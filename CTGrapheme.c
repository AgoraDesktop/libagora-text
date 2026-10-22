/*
 * CTGrapheme.c — UAX #29 extended grapheme cluster segmentation: twelve rules over three generated property tables.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * **THE RULES ARE A LIST AND THE LIST IS THE SPECIFICATION**, the same way `CTLineBreak.c` is a transcription of UAX
 * #14: there is no weighting and no scoring, and every rule is "never break here", "always break here", or "break
 * here unless the characters either side are these". So the implementation below is the list, in the standard's own
 * order, and its measurement is a CONFORMANCE RUN rather than a set of hand-picked examples.
 *
 * **THE ORDER IS PART OF THE ALGORITHM, NOT A STYLE CHOICE.** GB3 (CR × LF) is a NO-BREAK that must be asked before
 * GB4's "CR breaks after itself", and GB4/GB5 (hard breaks around Control, CR and LF) must be asked before GB9's
 * "Extend attaches", or a mark after a control would attach to it. The walk below asks them in §3.1's order and says
 * so at each one, because a reordering here is a silent behavioural change.
 *
 * **AND STATE IS CARRIED ACROSS THE RULES, WHICH IS WHY THIS IS A WALK AND NOT A PAIRWISE TEST.** GB11 needs "was
 * there an Extended_Pictographic, then only Extend characters, then a ZWJ" — a fact about an arbitrarily long run
 * behind the position — and GB12/GB13 need the PARITY of the run of regional indicators before it. Both are kept as
 * two variables beside the loop rather than recomputed per position, and both are updated in ONE place
 * (`fn_advance`) so a rule cannot see a state its own predecessor did not produce.
 */
#include <CoreText/CTGrapheme_internal.h>

/* =================================================================================================
 * THE PROPERTY LOOKUPS. Two binary searches over two generated tables; both answer "the default" for a code point no
 * range covers, which is what the UCD says the default is.
 * =============================================================================================== */
uint8_t ct_gcb_class(uint32_t codePoint)
{
	size_t low = 0;
	size_t high = ct_gcb_range_count;

	while (low < high) {
		size_t middle = low + (high - low) / 2;
		const struct ct_gcb_range *range = &ct_gcb_ranges[middle];

		if (codePoint < range->first) {
			high = middle;
		} else if (codePoint > range->last) {
			low = middle + 1;
		} else {
			return range->gcb;
		}
	}
	return kCTGcbOther;
}

bool ct_grapheme_is_pictographic(uint32_t codePoint)
{
	size_t low = 0;
	size_t high = ct_extpict_range_count;

	while (low < high) {
		size_t middle = low + (high - low) / 2;
		const struct ct_extpict_range *range = &ct_extpict_ranges[middle];

		if (codePoint < range->first) {
			high = middle;
		} else if (codePoint > range->last) {
			low = middle + 1;
		} else {
			return true;
		}
	}
	return false;
}

uint8_t ct_incb_class(uint32_t codePoint)
{
	size_t low = 0;
	size_t high = ct_incb_range_count;

	while (low < high) {
		size_t middle = low + (high - low) / 2;
		const struct ct_incb_range *range = &ct_incb_ranges[middle];

		if (codePoint < range->first) {
			high = middle;
		} else if (codePoint > range->last) {
			low = middle + 1;
		} else {
			return range->incb;
		}
	}
	return kCTIncbNone;
}

/* ONE CODE POINT AT `*at`, ADVANCING IT BY ONE OR TWO UNITS. A lone surrogate is passed through as itself, which is
 * the only thing UTF-16 can mean by it, and it then behaves as whatever class the table gives it (nothing, so
 * `Other`) rather than being silently merged with a neighbour. */
static uint32_t fn_next_code_point(const UniChar *chars, CFIndex count, CFIndex *at)
{
	UniChar unit = chars[*at];
	uint32_t codePoint = unit;

	(*at)++;
	if (unit >= 0xD800 && unit <= 0xDBFF && *at < count) {
		UniChar low = chars[*at];

		if (low >= 0xDC00 && low <= 0xDFFF) {
			codePoint = 0x10000 + (((uint32_t)unit - 0xD800) << 10) + ((uint32_t)low - 0xDC00);
			(*at)++;
		}
	}
	return codePoint;
}

/* **THE TWO FACTS THAT OUTLIVE A SINGLE PAIR.** `riRun` is the length of the run of regional indicators ending at the
 * previous character (GB12/GB13 care only about its PARITY); `pictState` is how far into `Extended_Pictographic
 * Extend* ZWJ` the text is (GB11). Both are set in one place, and only ever from the character that was just
 * consumed, so the pair seen by the rules is always the pair the text produced. */
enum {
	kPictNone = 0,
	kPictAfterPictograph,	/* Extended_Pictographic, then zero or more Extend */
	kPictAfterJoiner	/* ... and then a ZWJ */
};

/* **AND GB9c's OWN STATE, which is a two-bit memory of `Consonant (Extend|Linker)* Linker (Extend|Linker)*`**:
 * `afterConsonant` says the run began with a consonant, and `linkerSeen` says a LINKER has since appeared — the rule
 * needs AT LEAST ONE, which is what separates a conjunct from a consonant followed by marks. */
static void fn_advance_incb(uint32_t codePoint, bool *afterConsonant, bool *linkerSeen)
{
	uint8_t incb = ct_incb_class(codePoint);

	if (incb == kCTIncbConsonant) {
		*afterConsonant = true;
		*linkerSeen = false;
	} else if (incb == kCTIncbLinker) {
		if (*afterConsonant) {
			*linkerSeen = true;
		}
	} else if (incb == kCTIncbExtend) {
		/* the run continues; whether a linker was seen so far is unchanged */
	} else {
		*afterConsonant = false;
		*linkerSeen = false;
	}
}

static void fn_advance(uint32_t codePoint, uint8_t gcb, CFIndex *riRun, int *pictState)
{
	if (gcb == kCTGcbRegionalIndicator) {
		(*riRun)++;
	} else {
		*riRun = 0;
	}
	/* A PICTOGRAPH RESTARTS THE SEQUENCE whether or not the state said one was running: `ExtPict ExtPict` is two
	 * starts, and GB11 asks about the ZWJ that follows the SECOND. */
	if (ct_grapheme_is_pictographic(codePoint)) {
		*pictState = kPictAfterPictograph;
	} else if (gcb == kCTGcbExtend && *pictState == kPictAfterPictograph) {
		/* still after the pictograph */
	} else if (gcb == kCTGcbZWJ && *pictState == kPictAfterPictograph) {
		*pictState = kPictAfterJoiner;
	} else {
		*pictState = kPictNone;
	}
}

/* **THE TWELVE RULES, IN §3.1's ORDER.** True means "a cluster boundary falls between these two characters". */
static bool fn_break_between(uint8_t previous, uint8_t current, bool currentIsPictographic, CFIndex riRun,
			     int pictState, bool incbAfterConsonant, bool incbLinkerSeen, bool currentIsIncbConsonant)
{
	/* GB3: CR × LF — the ONE case where a hard break does not break. */
	if (previous == kCTGcbCR && current == kCTGcbLF) {
		return false;
	}
	/* GB4: (Control | CR | LF) ÷ — nothing attaches to a hard break. */
	if (previous == kCTGcbControl || previous == kCTGcbCR || previous == kCTGcbLF) {
		return true;
	}
	/* GB5: ÷ (Control | CR | LF) — and nothing attaches after one either. Asked BEFORE GB9, or a mark would. */
	if (current == kCTGcbControl || current == kCTGcbCR || current == kCTGcbLF) {
		return true;
	}
	/* GB6: L × (L | V | LV | LVT) */
	if (previous == kCTGcbL
	    && (current == kCTGcbL || current == kCTGcbV || current == kCTGcbLV || current == kCTGcbLVT)) {
		return false;
	}
	/* GB7: (LV | V) × (V | T) */
	if ((previous == kCTGcbLV || previous == kCTGcbV) && (current == kCTGcbV || current == kCTGcbT)) {
		return false;
	}
	/* GB8: (LVT | T) × T */
	if ((previous == kCTGcbLVT || previous == kCTGcbT) && current == kCTGcbT) {
		return false;
	}
	/* GB9: × (Extend | ZWJ) */
	if (current == kCTGcbExtend || current == kCTGcbZWJ) {
		return false;
	}
	/* GB9a: × SpacingMark */
	if (current == kCTGcbSpacingMark) {
		return false;
	}
	/* GB9b: Prepend × */
	if (previous == kCTGcbPrepend) {
		return false;
	}
	/* **GB9c: Consonant [Extend Linker]* Linker [Extend Linker]* × Consonant** — Unicode 15.1's addition, and the one
	 * rule that was not in the first eleven. A Devanagari `क` + virama + `त` is ONE cluster, an aksara; without this
	 * the caret could stand between the two consonants of a conjunct, and seven of the data's 1,187 strings said so. */
	if (incbAfterConsonant && incbLinkerSeen && currentIsIncbConsonant) {
		return false;
	}
	/* GB11: ExtPict Extend* ZWJ × ExtPict — one emoji even when it is drawn by several characters. */
	if (pictState == kPictAfterJoiner && currentIsPictographic) {
		return false;
	}
	/* GB12/GB13: regional indicators pair, and ONLY in pairs — so a flag is one cluster and three indicators are a
	 * flag and a stray, which is what the standard's parity test says. */
	if (previous == kCTGcbRegionalIndicator && current == kCTGcbRegionalIndicator && (riRun % 2) == 1) {
		return false;
	}
	/* GB999: everything else. */
	return true;
}

CFIndex ct_grapheme_cluster_end(const UniChar *chars, CFIndex count, CFIndex start)
{
	CFIndex at;
	CFIndex riRun = 0;
	int pictState = kPictNone;
	bool incbAfterConsonant = false;
	bool incbLinkerSeen = false;
	uint32_t previousCodePoint;
	uint8_t previousClass;

	if (chars == NULL || start < 0 || start >= count) {
		return start;
	}
	at = start;
	previousCodePoint = fn_next_code_point(chars, count, &at);
	previousClass = ct_gcb_class(previousCodePoint);
	fn_advance(previousCodePoint, previousClass, &riRun, &pictState);
	fn_advance_incb(previousCodePoint, &incbAfterConsonant, &incbLinkerSeen);

	while (at < count) {
		CFIndex begin = at;
		uint32_t codePoint = fn_next_code_point(chars, count, &at);
		uint8_t gcb = ct_gcb_class(codePoint);
		bool isPictographic = ct_grapheme_is_pictographic(codePoint);

		if (fn_break_between(previousClass, gcb, isPictographic, riRun, pictState, incbAfterConsonant,
				     incbLinkerSeen, ct_incb_class(codePoint) == kCTIncbConsonant)) {
			return begin;	/* the cluster ended before this character */
		}
		fn_advance(codePoint, gcb, &riRun, &pictState);
		fn_advance_incb(codePoint, &incbAfterConsonant, &incbLinkerSeen);
		previousClass = gcb;
		previousCodePoint = codePoint;
	}
	return count;
}
