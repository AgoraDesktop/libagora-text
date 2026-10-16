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
#include <stdbool.h>

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
 * say otherwise). The EXPLICIT phase — the embeddings, overrides and isolates of X1-X8 — is `ct_bidi_explicit_levels`
 * below; the W/N/I rules that give every OTHER character a level, and the L1/L2 rules that reorder them, are the next
 * slices. */
int ct_bidi_paragraph_level(const UniChar *text, CFIndex count, int fallbackLevel);

/* **UAX #9 X1-X8: THE EXPLICIT LEVELS.** An embedding, an override or an isolate CHANGES the level for what follows,
 * so this walks the text with a STACK of directional statuses (level, override, isolate) and fills `levels[i]` with
 * the embedding level of every character and `classes[i]` with its class AFTER the overrides have been applied. THE
 * EXPLICIT FORMATTING CHARACTERS (RLE/LRE/RLO/LRO/PDF) ARE MARKED `kCTBidiBN`, which is X9's "remove them" — the
 * level stays, and the W/N/I rules that follow are to ignore them. `levels` and `classes` must each hold `count`
 * entries. Answers false when the text is longer than this tier will walk (never, in practice) — the depth is capped
 * at 125 by the standard itself and overflow is HANDLED rather than refused, per the rules. */
bool ct_bidi_explicit_levels(const UniChar *text, CFIndex count, int paragraphLevel,
			     uint8_t *levels, CTBidiClass *classes);

/* **UAX #9 W1-W7: THE WEAK TYPES.** Applied over the LEVEL RUNS of the text (maximal runs of one embedding level),
 * each with its own `sos`/`eos` computed from the higher of its level and its neighbour's — which is the start of
 * X10's machinery without the isolates: an ISOLATING RUN SEQUENCE may join several level runs through an isolate,
 * and that join is the next slice. `classes` is edited in place and `levels` is read. */
void ct_bidi_resolve_weak(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel);

/* **UAX #9 N1 AND N2: THE NEUTRAL TYPES.** Over the LEVEL RUNS, like the weak rules — a run of neutrals between two
 * strong types of the SAME direction takes that direction (a NUMBER counting as R), and any other run of neutrals
 * takes the EMBEDDING direction of its run. WHAT IS NOT HERE IS TWO THINGS, BOTH NAMED: **X10**, the isolating run
 * sequences, which JOIN level runs through an isolate so the neutrals AROUND an isolate see each other; and **N0**,
 * the bracket pairs, which need Unicode's `BidiBrackets` data. Each is the next slice. */
void ct_bidi_resolve_neutral(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel);

#endif /* CORE_TEXT_CTBIDI_INTERNAL_H */
