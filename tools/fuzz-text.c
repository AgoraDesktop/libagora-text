/*
 * fuzz-text.c — the fuzzer for the text stack: random TEXT through every door, and random FONT BYTES through the
 * font parser.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * **WHY THIS EXISTS RATHER THAN A LINE IN THE SUITE.** The suite is 76 cases of strings somebody thought of; a fuzzer
 * is the same doors fed strings nobody thought of, and the two find different things. Everything this tree parses is
 * attacker-shaped — a font file is an arbitrary byte stream with offsets in it, and a document is arbitrary text that
 * reaches the bidi, line-breaking, shaping, normalization and grapheme engines — so the interesting question is not
 * whether `abc` lays out but whether 4,096 random UTF-16 units do.
 *
 * BUILD IT WITH SANITIZERS, or it is only a smoke test:
 *
 *     cmake -S libagora-text -B /tmp/asan-text \
 *         -DCMAKE_C_COMPILER=/usr/bin/clang-19 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-19 \
 *         -DCMAKE_BUILD_TYPE=Debug \
 *         -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1" \
 *         -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
 *         -DCMAKE_C_FLAGS:STRING="-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1" \
 *         -DCMAKE_PREFIX_PATH=$PWD/build-prefix
 *     cc -fsanitize=address,undefined -g -O1 -I build-prefix/include -I libagora-text \
 *        libagora-text/tools/fuzz-text.c -o /tmp/fuzz-text \
 *        -L /tmp/asan-text -lagora-text -L build-prefix/lib -lagora-core
 *
 * AND RUN IT AS:
 *
 *     LD_LIBRARY_PATH=/tmp/asan-text:build-prefix/lib ASAN_OPTIONS=detect_leaks=0 \
 *         /tmp/fuzz-text 3000 1 /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
 *
 * ARGUMENTS: iterations, seed, and an optional font file to mutate. The font half needs one because the parser's
 * input is a byte stream and the only honest way to make one is to damage a real one.
 *
 * **AND WHAT IT FOUND IS RECORDED IN `README.md`, not here**: the sweep's first finding was a heap-buffer-overflow in
 * `CTLineGetGlyphRuns` — a level array indexed by a character offset — which the 76-case suite could not see because
 * every line in it happened to have as many glyphs as characters.
 */
#include <CoreText/CoreText.h>
#include <CoreText/CTBidi_internal.h>
#include <CoreText/CTGrapheme_internal.h>
#include <CoreText/CTLineBreak_internal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* **AND A TRACE MODE, BECAUSE A HANG OR A CRASH IS ONLY USEFUL IF IT CAN BE REPRODUCED.** `FUZZ_TRACE=1` prints the
 * iteration, the string it built and each phase as it starts, so the failing input can be lifted straight into a test. */
static int g_trace;

static void fn_phase(const char *what, const UniChar *text, CFIndex length)
{
	if (!g_trace) {
		return;
	}
	fprintf(stderr, "  phase %s (", what);
	if (text != NULL) {
		CFIndex i;

		for (i = 0; i < length && i < 512; i++) {
			fprintf(stderr, "%s%04X", (i == 0) ? "" : " ", (unsigned)text[i]);
		}
	}
	fprintf(stderr, ")\n");
	fflush(stderr);
}

/* A xorshift, so a run is reproducible from its seed and no libc RNG version can change what "seed 1" means. */
static unsigned long g_state = 1;

static unsigned long fn_random(void)
{
	g_state ^= g_state << 13;
	g_state ^= g_state >> 7;
	g_state ^= g_state << 17;
	return g_state;
}

/* **TEXT FROM EVERY CORNER OF THE CODE SPACE ON PURPOSE**: ASCII, combining marks, Hebrew and Arabic, Devanagari with
 * its viramas, Hangul jamo, emoji, the explicit bidi controls, and the C0 controls that break lines. A generator that
 * only produced letters would never reach half these engines. */
static UniChar fn_random_unit(void)
{
	static const UniChar kInteresting[] = {
		0x0041, 0x0065, 0x0020, 0x0009, 0x000A, 0x000D, 0x0301, 0x0331, 0x05D0, 0x0627, 0x0661,
		0x0915, 0x094D, 0x0924, 0x1100, 0x1161, 0x11A8, 0xAC00, 0x200D, 0x200B, 0x200F, 0x2066,
		0x2069, 0x202A, 0x202C, 0x202E, 0x0028, 0x0029, 0x005B, 0x005D, 0x00AB, 0xD83D, 0xDC69,
		0x1F1E6 & 0xFFFF, 0x002D, 0x002C, 0x066B, 0x0025, 0x00A0, 0xFFFC, 0x000B, 0x0085
	};
	unsigned long choice = fn_random() % 8;

	if (choice < 5) {
		return kInteresting[fn_random() % (sizeof kInteresting / sizeof kInteresting[0])];
	}
	if (choice == 5) {
		return (UniChar)(0x20 + (fn_random() % 0x5F));
	}
	if (choice == 6) {
		return (UniChar)(0x0590 + (fn_random() % 0x70));
	}
	return (UniChar)(0xD800 + (fn_random() % 0x800));	/* a surrogate on its own, which must not crash */
}

/* ONE RANDOM LINE, THROUGH EVERY DOOR THAT TAKES ONE. Nothing here asserts anything: the ASSERTION IS ASAN, and a
 * door that writes past a buffer or reads freed memory says so by aborting with a stack trace. */
static void fn_fuzz_text(CTFontRef font, CTParagraphStyleRef style)
{
	static UniChar text[512];
	/* **LINES ARE SHORT ON PURPOSE, AND THE REASON IS A FINDING RATHER THAN A CONVENIENCE.** Each character the face
	 * cannot cover costs a font-fallback miss — MEASURED at 7 ms apiece even with the miss cached, because the search
	 * is re-enumerated — so a 512-character line of exotic characters takes SECONDS in file I/O rather than in the
	 * layout code this sweep exists to test. 48 units keeps every engine in play (a line that mixes scripts, controls
	 * and marks) while keeping a thousand iterations to seconds. The cost itself is recorded in `README.md`. */
	CFIndex length = (CFIndex)(fn_random() % 48);
	CFMutableDictionaryRef attrs;
	CFStringRef string;
	CFAttributedStringRef astr;
	CTLineRef line;
	CFIndex i;

	for (i = 0; i < length; i++) {
		text[i] = fn_random_unit();
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	CFDictionarySetValue(attrs, kCTParagraphStyleAttributeName, style);
	string = CFStringCreateWithCharacters(kCFAllocatorDefault, text, length);
	astr = (string != NULL) ? CFAttributedStringCreate(kCFAllocatorDefault, string, attrs) : NULL;

	/* THE ENGINES DIRECTLY, because a door may not reach every branch they have. */
	fn_phase("grapheme", text, length);
	(void)ct_grapheme_cluster_end(text, length, 0);
	if (length > 0) {
		fn_phase("linebreak", text, length);
		(void)ct_lb_break_before_utf16(text, length, 0, length);
	}
	if (length > 0 && length < 256) {
		uint8_t levels[256];
		CTBidiClass classes[256];

		fn_phase("bidi", text, length);
		if (ct_bidi_explicit_levels(text, length, (int)(fn_random() % 2), levels, classes)) {
			int paragraphLevel = (int)(fn_random() % 2);

			ct_bidi_resolve_weak(classes, levels, length, paragraphLevel);
			ct_bidi_resolve_brackets(text, classes, levels, length, paragraphLevel);
			ct_bidi_resolve_neutral(classes, levels, length, paragraphLevel);
			ct_bidi_resolve_implicit(classes, levels, length);
			ct_bidi_reset_levels(text, levels, length, paragraphLevel);
		}
	}
	fn_phase("normalize", text, length);
	if (string != NULL) {
		CFMutableStringRef normalized = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);

		if (normalized != NULL) {
			CFStringNormalizationForm forms[] = { kCFStringNormalizationFormC, kCFStringNormalizationFormD,
							      kCFStringNormalizationFormKC, kCFStringNormalizationFormKD };

			CFStringNormalize(normalized, forms[fn_random() % 4]);
			CFRelease(normalized);
		}
	}

	/* AND THE LAYOUT, WHICH IS WHERE THE TWO MAPS AND THE CARET ARITHMETIC MEET. */
	fn_phase("line", text, length);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	if (line != NULL) {
		CFArrayRef runs = CTLineGetGlyphRuns(line);
		CFIndex runCount = (runs != NULL) ? CFArrayGetCount(runs) : 0;
		CFIndex r;

		(void)CTLineGetTypographicBounds(line, NULL, NULL, NULL);
		for (i = -1; i <= length + 1; i++) {
			CGFloat secondary = 0.0;

			(void)CTLineGetOffsetForStringIndex(line, i, &secondary);
		}
		for (i = 0; i < 4; i++) {
			(void)CTLineGetStringIndexForPosition(line, CGPointMake((CGFloat)(fn_random() % 400) - 100.0, 0.0));
		}
		for (r = 0; r < runCount; r++) {
			CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);

			(void)CTRunGetGlyphsPtr(run);
			(void)CTRunGetStringIndicesPtr(run);
			(void)CTRunGetAdvancesPtr(run);
			(void)CTRunGetAttributes(run);
			(void)CTRunGetStatus(run);
		}
		if (length > 0) {
			CTLineRef justified = CTLineCreateJustifiedLine(line, 0.5, 200.0);
			CTLineRef truncated = CTLineCreateTruncatedLine(line, 100.0, kCTLineTruncationEnd, NULL);

			if (justified != NULL) {
				CFRelease(justified);
			}
			if (truncated != NULL) {
				CFRelease(truncated);
			}
		}
		CFRelease(line);
	}
	/* AND THE TYPESETTER, WHICH ASKS THE LINE-BREAKING ENGINE A DIFFERENT QUESTION. */
	fn_phase("typesetter", text, length);
	if (astr != NULL) {
		CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString(astr);

		if (typesetter != NULL) {
			CFIndex start = 0;

			while (start < length) {
				CFIndex count = CTTypesetterSuggestLineBreak(typesetter, start, 50.0);

				if (count <= 0) {
					count = 1;
				}
				start += count;
			}
			CFRelease(typesetter);
		}
	}
	if (astr != NULL) {
		CFRelease(astr);
	}
	if (string != NULL) {
		CFRelease(string);
	}
	CFRelease(attrs);
}

/* **AND A DAMAGED FONT, THROUGH THE PARSER.** A font file is an arbitrary byte stream full of offsets, so the only
 * honest input is a real one with bytes changed. Every door that reads a table is asked afterwards, because a table
 * read is where an offset from the file becomes a pointer. */
static void fn_fuzz_font(const unsigned char *bytes, long size, int round)
{
	unsigned char *damaged;
	CGDataProviderRef provider;
	CGFontRef graphics;
	CTFontRef font;
	int flips = 1 + (int)(fn_random() % 8);
	int i;

	damaged = (unsigned char *)malloc((size_t)size);
	if (damaged == NULL) {
		return;
	}
	memcpy(damaged, bytes, (size_t)size);
	for (i = 0; i < flips; i++) {
		/* THE HEADER FIRST — the table directory is the most delicate thing in the file — and then anywhere. */
		long at = (i == 0) ? (long)(fn_random() % 64) : (long)(fn_random() % (unsigned long)size);

		damaged[at] = (unsigned char)fn_random();
	}
	provider = CGDataProviderCreateWithData(NULL, damaged, (size_t)size, NULL);
	if (provider == NULL) {
		free(damaged);
		return;
	}
	graphics = CGFontCreateWithDataProvider(provider);
	CGDataProviderRelease(provider);
	if (graphics == NULL) {
		return;			/* a refusal is a correct answer, not a crash */
	}
	font = CTFontCreateWithGraphicsFont(graphics, 12.0, NULL, NULL);
	if (font != NULL) {
		static const CTFontTableTag tags[] = { kCTFontTableGlyf, kCTFontTableLoca, kCTFontTableHmtx,
						       kCTFontTableCmap, kCTFontTableHead, kCTFontTableMaxp, kCTFontTableKern,
						       kCTFontTableFeat };
		UniChar sample[8] = { 'A', 'b', 0x05D0, 0x0301, 0x0915, 0xFFFD, 0xD83D, 0xDC69 };
		CGGlyph glyphs[8];
		CGRect rects[8];
		CGSize advances[8];
		size_t t;

		for (t = 0; t < sizeof tags / sizeof tags[0]; t++) {
			CFDataRef table = CTFontCopyTable(font, tags[t], 0);

			if (table != NULL) {
				CFRelease(table);
			}
		}
		(void)CTFontGetGlyphsForCharacters(font, sample, glyphs, 8);
		(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, glyphs, advances, 8);
		(void)CTFontGetBoundingRectsForGlyphs(font, kCTFontHorizontalOrientation, glyphs, rects, 8);
		(void)CTFontGetAscent(font);
		(void)CTFontGetDescent(font);
		(void)CTFontGetLeading(font);
		(void)CTFontGetUnitsPerEm(font);
		(void)CTFontGetGlyphCount(font);
		if (round % 16 == 0) {
			CFStringRef name = CTFontCopyPostScriptName(font);

			if (name != NULL) {
				CFRelease(name);
			}
		}
		CFRelease(font);
	}
	CGFontRelease(graphics);
	free(damaged);
}

int main(int argc, char **argv)
{
	long iterations = (argc > 1) ? strtol(argv[1], NULL, 10) : 1000;
	CTFontRef font;
	CTParagraphStyleRef style;
	long i;

	const bool fontOnly = (getenv("FUZZ_FONT_ONLY") != NULL);

	g_trace = (getenv("FUZZ_TRACE") != NULL);
	g_state = (argc > 2) ? (unsigned long)strtoul(argv[2], NULL, 10) : 1;
	if (g_state == 0) {
		g_state = 1;
	}
	font = CTFontCreateWithName(CFSTR("DejaVuSans"), 13.0, NULL);
	if (font == NULL) {
		fprintf(stderr, "fuzz-text: no font to build lines with\n");
		return 1;
	}
	style = CTParagraphStyleCreate(NULL, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	/* **THE TWO HALVES ARE INDEPENDENT, AND `FUZZ_FONT_ONLY` EXISTS BECAUSE THEY COST WILDLY DIFFERENT AMOUNTS.** The
	 * text half pays a font-fallback miss per uncovered character (7 ms, see the note in `fn_fuzz_text`) and the font
	 * half pays one parse of one file, so a sweep of the PARSER wants the text loop out of the way. */
	for (i = 0; i < iterations && !fontOnly; i++) {
		if (g_trace) {
			fprintf(stderr, "iteration %ld\n", (long)i);
		}
		fn_fuzz_text(font, style);
		if ((i + 1) % 250 == 0) {
			printf("fuzz-text: %ld text iterations\n", (long)(i + 1));
		}
	}
	CFRelease(style);
	CFRelease(font);

	/* AND THE FONT HALF, WHEN A FILE IS NAMED. */
	if (argc > 3) {
		FILE *file = fopen(argv[3], "rb");
		long size;

		if (file != NULL && fseek(file, 0, SEEK_END) == 0 && (size = ftell(file)) > 0
		    && fseek(file, 0, SEEK_SET) == 0) {
			unsigned char *bytes = (unsigned char *)malloc((size_t)size);

			if (bytes != NULL && fread(bytes, (size_t)size, 1, file) == 1) {
				for (i = 0; i < iterations; i++) {
					fn_fuzz_font(bytes, size, (int)i);
					if ((i + 1) % 250 == 0) {
						printf("fuzz-text: %ld font iterations\n", (long)(i + 1));
					}
				}
			}
			free(bytes);
		}
		if (file != NULL) {
			fclose(file);
		}
	}
	printf("fuzz-text: %ld iterations, no failure\n", iterations);
	return 0;
}
