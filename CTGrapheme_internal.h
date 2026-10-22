/*
 * CTGrapheme_internal.h — UAX #29's extended grapheme clusters: where one cluster ends and the next begins.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NOT PART OF CORE TEXT'S SURFACE. Apple publishes no door that says "this is a grapheme cluster boundary"; what it
 * publishes is `CFStringGetRangeOfComposedCharactersAtIndex`, which answers a DIFFERENT question (see below), and the
 * caret doors, which move by clusters without ever naming the algorithm. This header is therefore internal, the same
 * standing `CTBidi_internal.h`, `CTLineBreak_internal.h` and `CTShaping_internal.h` have: the tier's own layers must
 * agree about where a cluster ends, and they agree here.
 *
 * **AND WHY A TEXT LAYER NEEDS THIS AT ALL: A CARET MOVES BY CLUSTER, NOT BY CHARACTER.** `CTLineGetOffsetForStringIndex`
 * takes a string index, and a user pressing the left arrow from after `e` + U+0301 must land BEFORE the `e`, not
 * between the letter and its accent — otherwise the accent is drawn on nothing halfway through the move. The same fact
 * decides what a deletion removes and what a selection highlights.
 *
 * **AND THE ALGORITHM IS A LIST OF RULES OVER THREE PROPERTIES, WHICH IS WHY IT IS HERE RATHER THAN BORROWED.**
 * `Grapheme_Cluster_Break` (GCB) is one property; `Extended_Pictographic` is ANOTHER — `©` is `Other` and a
 * pictograph — and GB11 needs both at once; and `Indic_Conjunct_Break` is a THIRD, which GB9c needs and which is why
 * the generator emits three tables instead of folding any of them together. The rules are UAX #29's §3.1: GB3
 * (CR × LF), GB4/GB5 (a Control/CR/LF breaks either way), GB6-GB8 (Hangul composes L+V+T arithmetically), GB9
 * (Extend and ZWJ attach), GB9a (SpacingMark attaches), GB9b (Prepend leads), **GB9c (a consonant, extenders and a
 * LINKER, then a consonant, is one aksara cluster)**, GB11 (an emoji ZWJ sequence is one cluster), GB12/GB13
 * (regional indicators pair, and only in pairs), and GB999 (anything else breaks).
 *
 * **AND GB9c IS THE RULE THIS SLICE DID NOT KNOW IT WAS MISSING — TWELVE RULES, NOT ELEVEN.** The engine was written
 * from the rules as they read before Unicode 15.1 and scored 1,180 of 1,187; the seven that failed were every case
 * the data marks rule 9.3, and they are all one shape: `क` + virama + `त`. Unicode 15.1 added GB9c for exactly that,
 * and it needs a property that is in neither of the other two tables. Found by the conformance run, not by review.
 *
 * **MEASURED: THE WHOLE OF `GraphemeBreakTest.txt` (Unicode 15.1.0), 1,187 strings — see `tests/text.c`'s
 * `case_grapheme_conformance`.** The suite runs a committed table of one case per rule on every build and the whole
 * file whenever `GRABREAK_TEST` names the download; `tools/gen-grapheme-cases.py` produces the table and records the
 * URL, and `tools/gen-grapheme-data.py` produces the two property tables.
 *
 * **AND IT IS NOT `CFStringGetRangeOfComposedCharactersAtIndex`, WHICH IS A DIFFERENT NOTION.** Core Foundation's
 * "composed character sequence" is base-plus-combining-marks, which is roughly a grapheme cluster but not UAX #29's:
 * it does not know about emoji ZWJ sequences, regional indicator pairs, or Hangul's arithmetic, and the two disagree
 * on exactly the cases this file exists to get right. Nothing here reinterprets that door — what the tier did instead
 * was move the CARET onto this algorithm.
 *
 * **AND IT IS WIRED INTO `CTLine` NOW, AS A SECOND MAP RATHER THAN AS A REPLACEMENT.** A line keeps `clusters[g]`
 * (which character a glyph stands for — the SHAPER's answer, and what `CTRunGetStringIndices` must report) and
 * `graphemes[i]` (this file's answer, one entry per character). `CTLineGetOffsetForStringIndex` and
 * `CTLineGetStringIndexForPosition` snap a character index to its grapheme start before answering, so no caret lands
 * between a base and its mark, inside an emoji ZWJ sequence or inside a flag — while the run's string indices keep
 * reporting what the shaper actually did. **MEASURED, and the disagreement is the test**: an emoji ZWJ sequence is
 * 3 glyphs over 3 runs with 3 distinct shaper string indices and ONE caret position, and a Devanagari aksara is 4
 * glyphs over 3 runs with 3 indices and one caret; a flag and a composed mark are shapes the shaper already agreed
 * about, and the case says so rather than asserting a disagreement that is not there.
 */
#ifndef CORE_TEXT_CTGRAPHEME_INTERNAL_H
#define CORE_TEXT_CTGRAPHEME_INTERNAL_H

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* **THE CLASSES, IN THE ORDER THE GENERATED TABLE'S VALUES USE.** The names are the UCD's own property values, and
 * `kCTGcbOther` is the default — it is never stored, because a table cannot know whether a code point was LISTED. */
enum {
	kCTGcbOther = 0,
	kCTGcbCR,
	kCTGcbLF,
	kCTGcbControl,
	kCTGcbExtend,
	kCTGcbZWJ,
	kCTGcbRegionalIndicator,
	kCTGcbPrepend,
	kCTGcbSpacingMark,
	kCTGcbL,
	kCTGcbV,
	kCTGcbT,
	kCTGcbLV,
	kCTGcbLVT
};

struct ct_gcb_range {
	uint32_t first;
	uint32_t last;
	uint8_t gcb;
};

extern const struct ct_gcb_range ct_gcb_ranges[];
extern const size_t ct_gcb_range_count;

/* **AND THE PICTOGRAPHS ARE A SECOND TABLE, NOT A FOURTEENTH CLASS**, because `Extended_Pictographic` is not a GCB
 * value: a character has a GCB value and MAY be a pictograph, and GB11 needs both facts about the same character. */
struct ct_extpict_range {
	uint32_t first;
	uint32_t last;
};

extern const struct ct_extpict_range ct_extpict_ranges[];
extern const size_t ct_extpict_range_count;

/* **AND A THIRD PROPERTY, BECAUSE THE RULES NEED ONE THAT CAME LATER THAN THE ALGORITHM'S FIRST WRITE-UP.**
 * `Indic_Conjunct_Break` (`InCB`) is what GB9c — added in Unicode 15.1 — reads: a consonant, then extenders and at
 * least one LINKER, then a consonant, is ONE cluster, which is a Devanagari aksara and not a phi-losophical nicety:
 * at 15.0 this case was seven of the data's 1,187 strings and a cluster that broke in the middle of a letter. */
enum {
	kCTIncbNone = 0,
	kCTIncbConsonant,
	kCTIncbLinker,
	kCTIncbExtend
};

struct ct_incb_range {
	uint32_t first;
	uint32_t last;
	uint8_t incb;
};

extern const struct ct_incb_range ct_incb_ranges[];
extern const size_t ct_incb_range_count;

uint8_t ct_incb_class(uint32_t codePoint);
uint8_t ct_gcb_class(uint32_t codePoint);
bool ct_grapheme_is_pictographic(uint32_t codePoint);

/* **THE DOOR: THE END OF THE CLUSTER THAT STARTS AT `start`.** `start` and the answer are UTF-16 UNIT indices into
 * `chars`, which is what a caller holding a `CFString` or a line's positions has; a surrogate pair is ONE code point
 * and so one unit of segmentation, which is the whole reason this decodes rather than indexes.
 *
 * The answer is always GREATER THAN `start` when `start` is in range — a cluster holds at least one code point, so a
 * caller that walks the door to the end of the string always advances. `start` itself comes back when it is out of
 * range, and `count` when the cluster runs to the end, so `start == count` is the way to say "done". */
CFIndex ct_grapheme_cluster_end(const UniChar *chars, CFIndex count, CFIndex start);

#endif /* CORE_TEXT_CT_GRAPHEME_INTERNAL_H */
