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
 *
 * **AND THE WHOLE PIPELINE IS NOW MEASURED AGAINST UNICODE'S OWN CASES: `BidiCharacterTest.txt` (Unicode 15.1.0),
 * 91,707 single-paragraph sequences each with its resolved paragraph level, its resolved level per character and its
 * visual order — 91,691 agree and 16 do not.** The suite's `case_bidi_conformance` runs a committed table (the
 * shortest case per feature) on every build, and the WHOLE file whenever `BIDI_TEST` names the download;
 * `tools/gen-bidi-cases.py` produces the table and records the URL. The file is not in this repository, the same
 * standing the UCD's other files have.
 *
 * **AND THE RUN EARNED ITS KEEP ON THE FIRST DAY, WHICH IS WHY IT IS HERE RATHER THAN ON A WISH LIST.** It found two
 * rules this file had read past: X5a/X6a's last sentence — an isolate initiator and its PDI take the OVERRIDE's class
 * when one is in effect, not the neutral they are born with — and N0's last sentence — combining marks that follow a
 * paired bracket take the bracket's new direction, and finding them needs the ORIGINAL class because W1 has already
 * rewritten it. Both are fixed and named where they are implemented.
 *
 * **AND THE 16 THAT REMAIN ARE ONE DEFECT, KNOWN AND NOT HIDDEN.** X9 removes embedding, override and BN characters
 * from the text the rules read; `fn_sequences` builds X10's level runs over the raw arrays, so a removed character
 * whose level differs from its neighbours SPLITS A RUN THAT SHOULD NOT HAVE A BOUNDARY THERE. The 16 are what that
 * costs: brackets that span an embedding, and a Latin word followed by an embedding and then digits, where W7's
 * backward search never reaches the word. Making the removed characters transparent was tried twice and MEASURED
 * WORSE (85,842 and 85,847 of 91,707), so the honest fix is a sequence pass over a COMPACTED index space — a cut of
 * its own, named here and in `fn_sequences` where the walk is.
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

/* **UAX #9 W1-W7: THE WEAK TYPES, over the ISOLATING RUN SEQUENCES of X10.** A sequence is a chain of level runs
 * joined through an isolate, with its own `sos`/`eos`; the rules run over each sequence in turn. `classes` is edited
 * in place and `levels` is read. */
void ct_bidi_resolve_weak(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel);

/* **UAX #9 N1 AND N2: THE NEUTRAL TYPES**, over the ISOLATING RUN SEQUENCES (X10) like the weak rules — a run of
 * neutrals between two strong types of the SAME direction takes that direction (a NUMBER counting as R), and any
 * other run of neutrals takes the EMBEDDING direction of its sequence. N0, the bracket pairs, runs BEFORE this (see
 * `ct_bidi_resolve_brackets`): it settles what it can, and what it cannot it leaves neutral for these rules. */
void ct_bidi_resolve_neutral(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel);

/* **UAX #9 I1 AND I2: THE IMPLICIT LEVELS.** The LAST level rule, and the one that puts a right-to-left character
 * inside a left-to-right paragraph at an ODD level — which is what L2 later reorders. At an EVEN level a strong R
 * goes UP one and a NUMBER up two (I1); at an ODD level L and a number go up one and R stays (I2). `levels` is edited
 * in place and `classes` is read: every class is L, R, EN or AN by now, because W and N have resolved the rest. */
void ct_bidi_resolve_implicit(const CTBidiClass *classes, uint8_t *levels, CFIndex count);

/* THE BRACKET PAIRS, generated from the UCD's `BidiBrackets.txt` by `tools/gen-bidi-brackets.py`: one row per bracket,
 * with the code point it PAIRS with and whether it OPENS. BD16 pushes an opener and matches a closer against it, so
 * both directions are in the table. */
struct ct_bidi_bracket {
	uint32_t bracket;
	uint32_t paired;
	bool isOpen;
};

extern const struct ct_bidi_bracket ct_bidi_brackets[];
extern const size_t ct_bidi_bracket_count;

/* **UAX #9 N0: THE BRACKET PAIRS.** A matched pair of brackets takes ONE direction: the embedding direction when the
 * text inside or the context before agrees with it, and the OPPOSITE direction when the inside and the context before
 * BOTH disagree with the embedding direction. Runs BEFORE the neutral rules, which settle whatever is left. Needs the
 * TEXT because a class says only "OTHER NEUTRAL" — whether a character is a bracket is a code-point fact. */
void ct_bidi_resolve_brackets(const UniChar *text, CTBidiClass *classes, const uint8_t *levels, CFIndex count,
			      int paragraphLevel);

/* **UAX #9 L1: THE LEVEL RESET.** A segment or paragraph separator, the whitespace/isolate formatting BEFORE it, and
 * the whitespace/isolate formatting at the END of the line all take the PARAGRAPH's level — so a trailing space does
 * not sit at an odd level and get reordered as though it were text. Reads the ORIGINAL classes (from `ct_bidi_class`
 * over the text), because the resolved ones no longer say which characters were whitespace. */
void ct_bidi_reset_levels(const UniChar *text, uint8_t *levels, CFIndex count, int paragraphLevel);

/* **UAX #9 L2: THE VISUAL ORDER.** Fills `visualOrder[k]` with the LOGICAL index that sits at visual position `k`: from
 * the highest level present down to the lowest ODD level, every maximal run of characters at that level OR HIGHER is
 * REVERSED. All-even levels mean no reversal at all, which is why a left-to-right line's order is the identity. */
void ct_bidi_reorder(const uint8_t *levels, CFIndex count, CFIndex *visualOrder);

/* THE MIRRORED GLYPH of a character, generated from the UCD's `BidiMirroring.txt` by `tools/gen-bidi-mirroring.py`:
 * one row per character that has a mirror, and the character it becomes. */
struct ct_bidi_mirroring {
	uint32_t code;
	uint32_t mirrored;
};

extern const struct ct_bidi_mirroring ct_bidi_mirrorings[];
extern const size_t ct_bidi_mirroring_count;

/* **UAX #9 L4: THE MIRRORED GLYPH.** A character whose RESOLVED level is ODD — right to left — is depicted by its
 * MIRRORED glyph, so `(` shows as `)` in Arabic text and `<` as `>`. The characters are rewritten IN PLACE, before
 * the glyphs are looked up, so the engine never sees the unmirrored one. A character the table does not name — and
 * one whose mirror is outside the BMP — is left alone, which is the honest answer for a table that ends where it
 * does. */
void ct_bidi_mirror(UniChar *chars, CFIndex count, const uint8_t *levels);

#endif /* CORE_TEXT_CTBIDI_INTERNAL_H */
