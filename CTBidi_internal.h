/*
 * CTBidi_internal.h — the Bidi_Class of a code point, and the paragraph level UAX #9 P2/P3 reads from it.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * NOT PART OF CORE TEXT'S SURFACE. Apple's Core Text applies the Unicode Bidirectional Algorithm inside the
 * typesetter and publishes no door for it; this tier is building that algorithm in slices, and this header is the
 * first one — the DATA and the paragraph level — kept internal because nothing Apple declares corresponds to it.
 *
 * **THE CLASS IS DATA, AND IT IS GENERATED.** `ct_bidi_class` answers from `CTBidiData.c`, which
 * `tools/gen-bidi-data.py` emits from Python's `unicodedata` (the Unicode Character Database itself). The table
 * omits the default class (L), so anything it does not name answers L.
 */
#ifndef CORE_TEXT_CTBIDI_INTERNAL_H
#define CORE_TEXT_CTBIDI_INTERNAL_H

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

#include <stddef.h>
#include <stdint.h>

/* **THE CLASSES, IN THE ORDER THE TABLE'S INDICES USE.** The names are Unicode's own
 * (`DerivedBidiClass.txt`), and the isolate/embedding/override CONTROLS are here because P2 must step over an
 * isolate run rather than read it. The order is the table's, so an index in the data file IS one of these. */
enum {
	kCTBidiL = 0,
	kCTBidiR,
	kCTBidiAL,
	kCTBidiEN,
	kCTBidiES,
	kCTBidiET,
	kCTBidiAN,
	kCTBidiCS,
	kCTBidiNSM,
	kCTBidiBN,
	kCTBidiB,
	kCTBidiS,
	kCTBidiWS,
	kCTBidiON,
	kCTBidiLRE,
	kCTBidiRLE,
	kCTBidiLRO,
	kCTBidiRLO,
	kCTBidiPDF,
	kCTBidiLRI,
	kCTBidiRLI,
	kCTBidiFSI,
	kCTBidiPDI
};
typedef uint8_t CTBidiClass;

/* ONE RANGE OF THE TABLE, and the table itself: a binary search over it answers `ct_bidi_class`. */
struct ct_bidi_range {
	uint32_t first;
	uint32_t last;
	CTBidiClass bidi_class;
};

extern const struct ct_bidi_range ct_bidi_ranges[];
extern const size_t ct_bidi_range_count;

/* THE CLASS OF A CODE POINT: the table's answer, or L for anything it does not name. */
CTBidiClass ct_bidi_class(uint32_t codePoint);

/* **UAX #9 P2 AND P3: THE PARAGRAPH'S LEVEL, WHICH IS 0 (left to right) OR 1 (right to left).** P2 finds the FIRST
 * STRONG character — L, or R/AL — SKIPPING everything between an isolate initiator and its matching PDI; P3 says the
 * paragraph is LEFT TO RIGHT when there is none, which is what `fallbackLevel` is for (a higher-level protocol may
 * say otherwise). This is the whole of the algorithm that is built so far: the X/W/N/I rules that assign a level to
 * every OTHER character, and the L1/L2 rules that reorder them, are the next slices. */
int ct_bidi_paragraph_level(const UniChar *text, CFIndex count, int fallbackLevel);

#endif /* CORE_TEXT_CTBIDI_INTERNAL_H */
