/*
 * CTBidi.c — the two rules built so far: which class a code point is, and which way the paragraph runs.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE TABLE ANSWERS THE CLASS; P2 AND P3 ANSWER THE PARAGRAPH.** A binary search over `CTBidiData.c` is the whole
 * of the first, and the second is the fourteen lines below it: find the first strong character while stepping over
 * isolate runs. Everything else UAX #9 does — the X, W, N and I rules that give every OTHER character a level, and
 * the L1/L2 rules that put them in visual order — is named as the next slice in `CTBidi_internal.h` rather than
 * half-built here.
 */
#include <CoreText/CTBidi_internal.h>

/* **THE CLASS OF A CODE POINT.** The table holds only the ranges whose class is NOT the default, so a miss is L —
 * which is also the answer for the unassigned code points the table deliberately does not carry. */
CTBidiClass ct_bidi_class(uint32_t codePoint)
{
	size_t lo = 0;
	size_t hi = ct_bidi_range_count;

	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		const struct ct_bidi_range *range = &ct_bidi_ranges[mid];

		if (codePoint < range->first) {
			hi = mid;
		} else if (codePoint > range->last) {
			lo = mid + 1;
		} else {
			return range->bidi_class;
		}
	}
	return kCTBidiL;
}

/* **UAX #9 P2 AND P3, WHICH ARE THE WHOLE OF WHAT IS BUILT.** P2 takes the first STRONG character and P3 turns that
 * into a level: 0 (left to right) for L, 1 (right to left) for R or AL, and the caller's `fallbackLevel` when there
 * is no strong character at all. */
int ct_bidi_paragraph_level(const UniChar *text, CFIndex count, int fallbackLevel)
{
	CFIndex i;
	int isolateDepth = 0;

	if (text == NULL) {
		return fallbackLevel;
	}
	for (i = 0; i < count; i++) {
		CTBidiClass bidiClass = ct_bidi_class(text[i]);

		/* **P2 STEPS OVER AN ISOLATE RUN RATHER THAN READING IT.** An initiator opens the run, everything between it
		 * and its matching PDI is skipped, and the PDI closes it; a nested initiator deepens the run, and an
		 * UNMATCHED PDI (depth already zero) is skipped by the same line that closes a matched one. */
		if (bidiClass == kCTBidiLRI || bidiClass == kCTBidiRLI || bidiClass == kCTBidiFSI) {
			isolateDepth++;
			continue;
		}
		if (bidiClass == kCTBidiPDI) {
			if (isolateDepth > 0) {
				isolateDepth--;
			}
			continue;
		}
		if (isolateDepth > 0) {
			continue;
		}
		/* THE FIRST STRONG CHARACTER DECIDES. */
		if (bidiClass == kCTBidiL) {
			return 0;
		}
		if (bidiClass == kCTBidiR || bidiClass == kCTBidiAL) {
			return 1;
		}
	}
	/* P3: WITH NO STRONG CHARACTER THE PARAGRAPH IS LEFT TO RIGHT, unless the higher-level protocol said otherwise. */
	return fallbackLevel;
}
