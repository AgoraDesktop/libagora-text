/*
 * CTLineBreakThai.c — the dictionary half of the `SA` tailoring: a libdatrie double-array trie, read at runtime, and
 * a greedy longest-match walk over it.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * **THE FILE FORMAT, WHICH IS THE WHOLE OF WHAT THIS FILE KNOWS THAT IS NOT THE ALGORITHM.** `thbrk.tri` is libdatrie's
 * serialised trie, and it is three sections in order, every integer BIG-ENDIAN:
 *
 *   1. THE ALPHABET — `AlphaMap`. `int32 alphaMapSignature` (0xD9FCD9FC), `int32 rangeCount`, then `rangeCount` pairs of
 *      `int32 begin, int32 end` (inclusive). The trie's own character codes are assigned in RANGE ORDER, one-based,
 *      because 0 is libdatrie's key terminator: the first character of the first range is code 1. A character outside
 *      every range is not in the alphabet at all.
 *   2. THE DOUBLE ARRAY — `DArray`. `int32 daSignature` (0xDAFCDAFC), `int32 cellCount`, then `cellCount - 1` pairs of
 *      `int32 base, int32 check` — the cell 0 is not stored and is synthesised by the reader as
 *      `{ base = signature, check = cellCount }`, which is libdatrie's own way of making cell 0 unreachable. A walk
 *      from the root (state 2) takes `next = base[state] + code` and is legal when `check[next] == state`.
 *      **A CELL WHOSE `base` IS NEGATIVE IS A LEAF**, and the negated value minus one is the index of its tail block.
 *   3. THE TAILS — `Tail`. `int32 tailSignature` (0xDFFCDFFC), `int32 firstFree`, `int32 tailCount`, then `tailCount`
 *      blocks of `int32 nextFree`, `int32 data`, `int16 length`, `length` bytes of suffix. **THE FILE DOES NOT STORE
 *      THE SUFFIX'S TERMINATOR** — libdatrie allocates `length + 1` bytes and writes the terminator in memory — so this
 *      reader appends one, which is what makes an empty tail and a one-character tail tell themselves apart.
 *
 * A KEY IS WALKED AS `characters + terminator`: the array is walked until a leaf is reached, and the leaf's tail must
 * then equal the rest of the key. A word that ends exactly at a branch node is reached by walking the TERMINATOR
 * through the array to a leaf whose tail is empty, which is why the walk below tests the terminator branch at every
 * position rather than only at the end.
 *
 * PROVENANCE OF THE FORMAT: libdatrie 0.2.13's `alpha-map.c` (`alpha_map_fread_bin`, `alpha_map_recalc_work_area`),
 * `darray.c` (`da_fread`, `da_walk`, `da_get_root`), `tail.c` (`tail_fread`, `tail_get_suffix`, `tail_walk_char`) and
 * `fileutils.c` (big-endian `file_read_int32`). libdatrie is `LGPL-2.1-or-later`; NO CODE FROM IT IS HERE — what is
 * here is a fact about a file format, and the reader is this tier's own.
 */
#include <CoreText/CTLineBreakThai_internal.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* THE THREE SIGNATURES, which are also the first thing a malformed or foreign file trips over. */
#define kThaiAlphaSig 0xD9FCD9FCu
#define kThaiDaSig 0xDAFCDAFCu
#define kThaiTailSig 0xDFFCDFFCu

/* libdatrie's own constants: the root state, the key terminator, and "not in the alphabet". */
enum { kThaiRoot = 2, kThaiTerm = 0, kThaiNoChar = -1 };

struct ct_thai_range {
	uint32_t begin;
	uint32_t end;
};

struct ct_thai_tail {
	int32_t data;
	const uint8_t *suffix;		/* TERMINATOR-TERMINATED, because the file does not store it */
};

struct ct_thai {
	int32_t *base;			/* malloc'd, `cells` entries */
	int32_t *check;			/* malloc'd, `cells` entries */
	int32_t cells;
	struct ct_thai_tail *tails;	/* malloc'd, `tailCount` entries */
	int32_t tailCount;
	struct ct_thai_range *ranges;	/* malloc'd, `rangeCount` entries, in file order */
	int32_t rangeCount;
	uint8_t *suffixes;		/* malloc'd arena the tails' suffix pointers point into */
	size_t suffixesSize;
};

static struct ct_thai g_thai;
static bool g_thaiLoaded;			/* a successful load, which is what makes the door answer true */
static bool g_thaiTried;			/* ... and this is what makes it answer at all: once */
static pthread_mutex_t g_thaiLock = PTHREAD_MUTEX_INITIALIZER;

/* A CURSOR OVER THE WHOLE FILE, read once. Every field is bounds-checked as it is taken, so a truncated file is a
 * refusal rather than a read past the end. */
struct ct_thai_cursor {
	const uint8_t *bytes;
	size_t size;
	size_t at;
};

static bool ct_thai_u32(struct ct_thai_cursor *c, uint32_t *out)
{
	if (c->at + 4 > c->size) {
		return false;
	}
	*out = ((uint32_t)c->bytes[c->at] << 24) | ((uint32_t)c->bytes[c->at + 1] << 16)
	       | ((uint32_t)c->bytes[c->at + 2] << 8) | (uint32_t)c->bytes[c->at + 3];
	c->at += 4;
	return true;
}

static bool ct_thai_i16(struct ct_thai_cursor *c, int16_t *out)
{
	if (c->at + 2 > c->size) {
		return false;
	}
	*out = (int16_t)(((uint16_t)c->bytes[c->at] << 8) | (uint16_t)c->bytes[c->at + 1]);
	c->at += 2;
	return true;
}

static bool ct_thai_bytes(struct ct_thai_cursor *c, size_t count, const uint8_t **out)
{
	if (c->at + count > c->size) {
		return false;
	}
	*out = c->bytes + c->at;
	c->at += count;
	return true;
}

static void ct_thai_free(void)
{
	free(g_thai.base);
	free(g_thai.check);
	free(g_thai.tails);
	free(g_thai.ranges);
	free(g_thai.suffixes);
	memset(&g_thai, 0, sizeof g_thai);
}

/* **THE THREE SECTIONS, IN ORDER, EACH REFUSING A BAD SIGNATURE.** A file that fails anywhere leaves the tables freed
 * and the door answering false, so a partly-read trie can never be walked. */
static bool ct_thai_parse(const uint8_t *bytes, size_t size)
{
	struct ct_thai_cursor c = { bytes, size, 0 };
	uint32_t sig, count, i;
	uint8_t *suffixes;
	size_t used = 0;

	/* 1. THE ALPHABET. */
	if (!ct_thai_u32(&c, &sig) || (sig & 0xFFFFFFFFu) != kThaiAlphaSig || !ct_thai_u32(&c, &count)) {
		return false;
	}
	if (count == 0 || count > 1024) {
		return false;
	}
	g_thai.rangeCount = (int32_t)count;
	g_thai.ranges = (struct ct_thai_range *)calloc(count, sizeof(struct ct_thai_range));
	if (g_thai.ranges == NULL) {
		return false;
	}
	for (i = 0; i < count; i++) {
		uint32_t begin, end;

		if (!ct_thai_u32(&c, &begin) || !ct_thai_u32(&c, &end) || end < begin) {
			return false;
		}
		g_thai.ranges[i].begin = begin;
		g_thai.ranges[i].end = end;
	}

	/* 2. THE DOUBLE ARRAY. Cell 0 is not stored; libdatrie synthesises it and so does this. */
	if (!ct_thai_u32(&c, &sig) || (sig & 0xFFFFFFFFu) != kThaiDaSig || !ct_thai_u32(&c, &count)) {
		return false;
	}
	if (count < 3 || count > 0x00FFFFFFu) {
		return false;
	}
	g_thai.cells = (int32_t)count;
	g_thai.base = (int32_t *)calloc(count, sizeof(int32_t));
	g_thai.check = (int32_t *)calloc(count, sizeof(int32_t));
	if (g_thai.base == NULL || g_thai.check == NULL) {
		return false;
	}
	g_thai.base[0] = (int32_t)kThaiDaSig;
	g_thai.check[0] = (int32_t)count;
	for (i = 1; i < count; i++) {
		uint32_t base, check;

		if (!ct_thai_u32(&c, &base) || !ct_thai_u32(&c, &check)) {
			return false;
		}
		g_thai.base[i] = (int32_t)base;
		g_thai.check[i] = (int32_t)check;
	}

	/* 3. THE TAILS. The suffix bytes are copied into one arena with a terminator appended to each. */
	if (!ct_thai_u32(&c, &sig) || (sig & 0xFFFFFFFFu) != kThaiTailSig) {
		return false;
	}
	{
		uint32_t firstFree, tailCount;
		struct ct_thai_tail *tails;

		if (!ct_thai_u32(&c, &firstFree) || !ct_thai_u32(&c, &tailCount)) {
			return false;
		}
		(void)firstFree;
		if (tailCount > 0x00FFFFFFu) {
			return false;
		}
		tails = (struct ct_thai_tail *)calloc(tailCount > 0 ? tailCount : 1, sizeof(struct ct_thai_tail));
		if (tails == NULL) {
			return false;
		}
		g_thai.tails = tails;
		g_thai.tailCount = (int32_t)tailCount;

		/* The arena cannot be larger than the rest of the file plus one terminator per tail; size it once. */
		suffixes = (uint8_t *)malloc((c.size - c.at) + tailCount + 1);
		if (suffixes == NULL) {
			return false;
		}
		for (i = 0; i < tailCount; i++) {
			uint32_t nextFree, data;
			int16_t length;
			const uint8_t *raw;

			if (!ct_thai_u32(&c, &nextFree) || !ct_thai_u32(&c, &data) || !ct_thai_i16(&c, &length)
			    || length < 0) {
				free(suffixes);
				return false;
			}
			(void)nextFree;
			if (!ct_thai_bytes(&c, (size_t)length, &raw)) {
				free(suffixes);
				return false;
			}
			tails[i].data = (int32_t)data;
			tails[i].suffix = suffixes + used;
			if (length > 0) {
				memcpy(suffixes + used, raw, (size_t)length);
			}
			used += (size_t)length;
			suffixes[used++] = (uint8_t)kThaiTerm;	/* THE TERMINATOR THE FILE LEAVES OUT */
		}
	}
	g_thai.suffixes = suffixes;
	g_thai.suffixesSize = used;
	return true;
}

/* **THE THREE PLACES libthai LOOKS, IN ITS OWN ORDER**: the file named by `LIBTHAI_DICTDIR` if it is set, then the
 * install path Debian and Fedora both use. The environment variable is libthai's own — `th_brk_new(NULL)` documents it
 * — so pointing it somewhere else is how this door is TESTED with no dictionary, which is the degradation that matters:
 * a machine without the data must keep the rule list's answer rather than lose Thai text. */
static bool ct_thai_read(void)
{
	static const char *const dirs[] = { "/usr/share/libthai/", "/usr/share/thai/" };
	const char *envDir = getenv("LIBTHAI_DICTDIR");
	char path[512];
	size_t d;

	for (d = 0; d < sizeof dirs / sizeof dirs[0] + 1; d++) {
		const char *dir = (d == 0) ? envDir : dirs[d - 1];
		FILE *file;
		long length;
		uint8_t *bytes;
		bool ok;

		if (dir == NULL) {
			continue;
		}
		if (snprintf(path, sizeof path, "%s%s", dir, (dir[0] != '\0' && dir[strlen(dir) - 1] == '/')
			     ? "thbrk.tri" : "/thbrk.tri") >= (int)sizeof path) {
			continue;
		}
		file = fopen(path, "rb");
		if (file == NULL) {
			continue;
		}
		if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
			fclose(file);
			continue;
		}
		bytes = (uint8_t *)malloc((size_t)length);
		if (bytes == NULL) {
			fclose(file);
			continue;
		}
		ok = (fread(bytes, (size_t)length, 1, file) == 1);
		fclose(file);
		if (ok && length > 0) {
			ok = ct_thai_parse(bytes, (size_t)length);
		} else {
			ok = false;
		}
		free(bytes);			/* the parser copies what it keeps */
		if (ok) {
			return true;
		}
		ct_thai_free();
	}
	return false;
}

bool ct_thai_available(void)
{
	bool loaded;

	pthread_mutex_lock(&g_thaiLock);
	if (!g_thaiTried) {
		g_thaiTried = true;
		g_thaiLoaded = ct_thai_read();
	}
	loaded = g_thaiLoaded;
	pthread_mutex_unlock(&g_thaiLock);
	return loaded;
}

/* **A CODE POINT TO THE TRIE'S OWN CHARACTER CODE**, or -1 when the alphabet does not have it. One-based and in range
 * order, which is libdatrie's assignment and the reason this is a loop rather than a subtract. */
static int ct_thai_char(const struct ct_thai *t, UniChar c)
{
	int32_t code = kThaiTerm + 1;	/* code 0 is the terminator, so the alphabet starts at 1 */
	int32_t i;

	if (c == 0) {
		return kThaiNoChar;	/* a NUL character is not a character in the alphabet */
	}
	for (i = 0; i < t->rangeCount; i++) {
		if ((uint32_t)c >= t->ranges[i].begin && (uint32_t)c <= t->ranges[i].end) {
			return (int)(code + ((int32_t)c - (int32_t)t->ranges[i].begin));
		}
		code += (int32_t)(t->ranges[i].end - t->ranges[i].begin) + 1;
	}
	return kThaiNoChar;
}

/* **THE TAIL OF A LEAF AGAINST THE INPUT, WHICH IS THE SECOND HALF OF A KEY.** Returns the index one past the last
 * character the word consumes, or -1 when the tail does not match — including the case where the input ends in the
 * middle of the tail, which is a word that runs past the run being broken and so is not a word here. */
static CFIndex ct_thai_tail_match(const struct ct_thai *t, int32_t leaf, const UniChar *chars, CFIndex count,
				  CFIndex at)
{
	int32_t index = -t->base[leaf] - 1;
	const uint8_t *suffix;
	CFIndex p = at;

	if (index < 0 || index >= t->tailCount) {
		return -1;
	}
	suffix = t->tails[index].suffix;
	while (*suffix != (uint8_t)kThaiTerm) {
		int code;

		if (p >= count) {
			return -1;	/* the word is longer than the run: not a break opportunity here */
		}
		code = ct_thai_char(t, chars[p]);
		if (code < 0 || code != (int)*suffix) {
			return -1;
		}
		suffix++;
		p++;
	}
	return p;
}

/* WHETHER A LEAF'S TAIL IS EMPTY, which is what a word ending exactly at a branch node has. */
static bool ct_thai_tail_empty(const struct ct_thai *t, int32_t leaf)
{
	int32_t index = -t->base[leaf] - 1;

	return index >= 0 && index < t->tailCount && t->tails[index].suffix[0] == (uint8_t)kThaiTerm;
}

/* **THE LONGEST DICTIONARY WORD STARTING AT `at`, AS A LENGTH, OR 0.** The walk is the array's own walk: the
 * terminator is tested at EVERY position (a word can end at a branch node), a leaf ends the walk because a leaf has no
 * children, and its tail decides how much more of the input the word consumes. */
static CFIndex ct_thai_word(const struct ct_thai *t, const UniChar *chars, CFIndex count, CFIndex at)
{
	int32_t state = kThaiRoot;
	CFIndex p = at;
	CFIndex best = 0;

	for (;;) {
		int32_t term = t->base[state] + kThaiTerm;

		/* A WORD MAY END HERE, through the terminator and an empty tail. */
		if (term > 0 && term < t->cells && t->check[term] == state && t->base[term] < 0
		    && ct_thai_tail_empty(t, term)) {
			best = p - at;
		}
		/* A LEAF REACHED BY CONSUMING CHARACTERS finishes the word through its tail, and has no children. */
		if (state != kThaiRoot && t->base[state] < 0) {
			CFIndex end = ct_thai_tail_match(t, state, chars, count, p);

			if (end > best + at) {
				best = end - at;
			}
			break;
		}
		if (p >= count) {
			break;
		}
		{
			int code = ct_thai_char(t, chars[p]);
			int32_t next;

			if (code < 0) {
				break;
			}
			next = t->base[state] + code;
			if (next <= 0 || next >= t->cells || t->check[next] != state) {
				break;
			}
			state = next;
			p++;
		}
	}
	return best;
}

void ct_thai_breaks(const UniChar *chars, CFIndex count, uint8_t *breaks)
{
	const struct ct_thai *t = &g_thai;
	CFIndex at = 0;

	if (chars == NULL || breaks == NULL || count <= 1 || !g_thaiLoaded) {
		return;
	}
	while (at < count) {
		CFIndex length = ct_thai_word(t, chars, count, at);

		if (length <= 0) {
			at++;
			continue;
		}
		at += length;
		if (at < count) {
			breaks[at] = 1;
		}
	}
}
