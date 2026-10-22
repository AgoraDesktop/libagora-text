/*
 * CTLineBreak_internal.h — UAX #14: where a line may be broken, which is the one thing a TYPESETTER needs that no
 * font can answer.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * NOT PART OF CORE TEXT'S SURFACE. Apple applies the Unicode Line Breaking Algorithm inside the typesetter and
 * publishes no door for it — `CTTypesetterSuggestLineBreak` is the whole of what a caller sees — so this header is
 * internal, the same standing `CTBidi_internal.h` and `CTShaping_internal.h` have and for the same reason: this
 * tier's typesetter and this tier's rules are two things in two files that must agree, and they agree here.
 *
 * **THE ALGORITHM IS A LIST OF RULES AND THE LIST IS THE SPECIFICATION.** UAX #14 states 31 numbered rules over the
 * Line_Break property of adjacent characters, and every one of them is either "never break here", "always break
 * here", or "break here unless the characters either side are in these classes". There is no weighting and no
 * scoring: a line breaker either obeys the rule list or it does not, which is why this file's engine is a
 * transcription of the list and why its measurement is a CONFORMANCE RUN rather than a set of hand-picked
 * examples — the Unicode Character Database ships `LineBreakTest.txt`, character sequences with Unicode's own
 * answer for every position and the rule that decided it.
 *
 * **MEASURED: 10,274 of 10,274 CONFORMANCE CASES, NO FAILURES** (Unicode 15.1.0's `auxiliary/LineBreakTest.txt`,
 * which is not in this repository — `tools/gen-linebreak-cases.py` records how to fetch it and produces the table
 * the suite commits). **AND THE RUN IS WHAT FOUND EVERY HARD PART**: what LB9's fold loses (a base and its mark
 * are one unit, not two characters that happen to share a class), that Unicode 15.1 REPLACED LB15 with LB15a and
 * LB15b (which need `General_Category` being `Pi` or `Pf`, a property the Line_Break class cannot express), that
 * §8.2's Example 7 tailors LB13 BESIDE LB25, and that LB28a has four clauses and reads the BASE of a syllable
 * rather than the character next to the position. None of those is reachable from hand-picked examples.
 *
 * **AND THE CLASSES ARE DATA.** `ct_lb_class` answers from `CTLineBreakData.c`, which
 * `tools/gen-linebreak-data.py` emits from `LineBreak.txt` and `EastAsianWidth.txt` (Unicode 15.1.0). The table
 * omits the default class (`XX`), and the file's own defaults for unlisted code points — `ID` in the CJK blocks
 * and the unassigned planes, `PR` in Currency Symbols — are baked into it, because a lookup cannot know whether a
 * code point was LISTED.
 */
#ifndef CORE_TEXT_CTLINEBREAK_INTERNAL_H
#define CORE_TEXT_CTLINEBREAK_INTERNAL_H

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

#include <stdbool.h>
#include <stdint.h>

/* **THE CLASSES, IN THE ORDER THE GENERATED TABLE'S INDICES USE.** The names are Unicode's own
 * (`LineBreak.txt`), and the order is this enum's — `tools/gen-linebreak-data.py` carries the same list, so an
 * index in the data file IS one of these constants. `XX` is FIRST because it is the default the table omits. */
enum {
	kCTLBXX = 0,	/* unknown, and what the table answers for anything it does not name */
	kCTLBAI,	/* ambiguous (alphabetic or ideograph), resolved by LB1 */
	kCTLBAK,
	kCTLBAL,
	kCTLBAP,
	kCTLBAS,
	kCTLBB2,
	kCTLBBA,
	kCTLBBB,
	kCTLBBK,
	kCTLBCB,
	kCTLBCJ,
	kCTLBCL,
	kCTLBCM,
	kCTLBCP,
	kCTLBCR,
	kCTLBEB,
	kCTLBEM,
	kCTLBEX,
	kCTLBGL,
	kCTLBH2,
	kCTLBH3,
	kCTLBHL,
	kCTLBHY,
	kCTLBID,
	kCTLBIN,
	kCTLBIS,
	kCTLBJL,
	kCTLBJT,
	kCTLBJV,
	kCTLBLF,
	kCTLBNL,
	kCTLBNS,
	kCTLBNU,
	kCTLBOP,
	kCTLBPO,
	kCTLBPR,
	kCTLBQU,
	kCTLBRI,
	kCTLBSA,
	kCTLBSG,
	kCTLBSP,
	kCTLBSY,
	kCTLBVF,
	kCTLBVI,
	kCTLBWJ,
	kCTLBZW,
	kCTLBZWJ
};

/* ONE RANGE OF CODE POINTS SHARING A VALUE, which is how the property is stored: 3,500 of them rather than a
 * 1.1-million-entry table. `value` is a `kCTLB*` class in the first table and unused in the second. */
struct ct_lb_range {
	uint32_t first;
	uint32_t last;
	int32_t value;
};

extern const struct ct_lb_range ct_lb_ranges[];
extern const CFIndex ct_lb_range_count;
extern const struct ct_lb_range ct_lb_wide_brackets[];
extern const CFIndex ct_lb_wide_bracket_count;
extern const struct ct_lb_range ct_lb_pi_qu[];
extern const CFIndex ct_lb_pi_qu_count;
extern const struct ct_lb_range ct_lb_pf_qu[];
extern const CFIndex ct_lb_pf_qu_count;
extern const struct ct_lb_range ct_lb_unassigned_pictographic[];
extern const CFIndex ct_lb_unassigned_pictographic_count;

/* THE LINE_BREAK CLASS OF A CODE POINT, by binary search over the ranges. **IT IS THE RAW PROPERTY AND NOT LB1's
 * ANSWER**: `AI`, `SA`, `SG`, `CJ` and `XX` are resolved inside the engine, where the resolution is part of a rule
 * list a reader can follow rather than a surprise in a lookup. */
int ct_lb_class(UTF32Char c);

/* **AND WHETHER A `CP`/`OP` IS EAST ASIAN, WHICH IS LB30'S OWN QUESTION.** True when the code point is a bracket
 * whose `East_Asian_Width` is F, W or H — the widths for which LB30 refuses to make the pair a break
 * opportunity. */
bool ct_lb_wide_bracket(UTF32Char c);

/* **AND THREE MORE PROPERTIES, EACH BECAUSE ONE RULE ASKS A QUESTION THE CLASS CANNOT ANSWER.**
 *
 * `ct_lb_is_pi_qu` / `ct_lb_is_pf_qu`: a `QU` is INITIAL or FINAL punctuation — `General_Category` being Pi or Pf —
 * and **Unicode 15.1 REPLACED LB15 WITH TWO RULES THAT TURN ON IT** (LB15a and LB15b). A quote that OPENS
 * something does not break after it; one that CLOSES something does not break before it; and the two are the same
 * Line_Break class, so the class alone cannot say which.
 *
 * `ct_lb_is_unassigned_pictographic`: LB30b's second clause is `[\p{Extended_Pictographic}&\p{Cn}] × EM` — an
 * UNASSIGNED code point that is nonetheless an emoji — which is the emoji property minus every assigned code point:
 * two UCD files and a set difference, computed when the table was generated. */
bool ct_lb_is_pi_qu(UTF32Char c);
bool ct_lb_is_pf_qu(UTF32Char c);
bool ct_lb_is_unassigned_pictographic(UTF32Char c);

/* =================================================================================================
 * THE DOOR.
 * =============================================================================================== */
/* **WHETHER A LINE MAY BE BROKEN BEFORE THE CHARACTER AT `index`**, over `chars[0..count)`. The positions UAX #14
 * has an opinion about are the boundaries, and there are `count + 1` of them: `index` 0 is the START of the text
 * (LB2: no break) and `index == count` is its END (LB3: always a break). A `NULL` or empty run answers false for 0
 * and true for `count`.
 *
 * THE RULES NEED THE WHOLE RUN AND NOT THE TWO CHARACTERS BESIDE THE POSITION, which is why this takes the array:
 * LB9 attaches a combining mark to the character it follows, LB14/15/16/17 look back over a run of spaces, LB25
 * reads a NUMBER across several characters, and LB30a counts regional indicators from the start of the run. */
bool ct_lb_break_at(const UTF32Char *chars, CFIndex count, CFIndex index);

/* **AND THE LAST OPPORTUNITY BEFORE A POINT, WHICH IS THE QUESTION A TYPESETTER ACTUALLY ASKS.** A line that fills
 * its width at `limit` must break at the greatest `i` in `(start, limit]` where `ct_lb_break_at` says yes — and if
 * there is none, the caller has a word wider than the line and decides for itself. Answers `start` when nothing in
 * the range is an opportunity, so a caller cannot mistake "no break" for an index that was never searched. */
CFIndex ct_lb_break_before(const UTF32Char *chars, CFIndex count, CFIndex start, CFIndex limit);

/* **AND THE SAME QUESTION OVER UTF-16, WHICH IS WHERE THIS LIBRARY LIVES.** UAX #14 is written over CODE POINTS and
 * this door is written over `UniChar`, and the two index spaces are not the same one: a surrogate pair is two units
 * and one code point, so a typesetter working in UTF-16 cannot ask the engine a question in its own indices. Rather
 * than make every caller do the conversion, the engine does it — the run is decoded once per call, the answer is
 * mapped back to the UNIT index the caller asked in, and a caller never sees a code point index at all.
 *
 * `start` and `limit` are UNIT indices and the answer is one too; `start` comes back when the range holds no
 * opportunity, exactly as above. */
CFIndex ct_lb_break_before_utf16(const UniChar *chars, CFIndex count, CFIndex start, CFIndex limit);

#endif /* CORE_TEXT_CTLINEBREAK_INTERNAL_H */
