/*
 * CTTypesetter.h — the typesetter: the thing that decides where a line ENDS and the next one begins.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), whose only include is `CTLine.h` — a typesetter's whole output
 * is lines, and the header says so by including no more than that.
 *
 * IT IS THE FIRST THING IN THIS LIBRARY THAT MAKES A DECISION RATHER THAN MEASURING ONE. Everything below it answers
 * a question that was asked; a typesetter answers "how much of this text fits", which is a CHOICE — and Apple prints
 * the choice's shape in its own words: "The line break can be triggered either by a HARD BREAK CHARACTER in the
 * stream or by FILLING THE SPECIFIED WIDTH with characters." That sentence is the whole of what this tier implements,
 * and it is implemented honestly: a hard break ends the line wherever it is, and otherwise characters are taken
 * until the width is full.
 *
 * WHAT IS NOT ATTEMPTED, SAID WHERE A CALLER READS IT — AND WHAT WAS MISSING IS HERE NOW. **THE UNICODE
 * LINE-BREAKING RULES (UAX #14) ARE IMPLEMENTED AND CALLED** (`CTLineBreak.c`, over the UCD's `Line_Break` property,
 * at the UCD's own conformance score: 10,274 of 10,274 of `LineBreakTest.txt`), so the boundary a wrapped line backs
 * off to is the last one the RULES allow rather than the last SPACE — MEASURED, `abc-defghij` now breaks after the
 * hyphen instead of at the overflow. **AND HYPHENATION IS IMPLEMENTED TOO, OPT-IN**: `kCTTypesetterOptionHyphenationFactor`
 * (this tier's own key, declared below, because the factor is AppKit's property) lets the breaker take a hyphenation
 * point where UAX #14 has no opportunity or where the legal break would leave the line too empty — and a caller that
 * passes no options gets exactly the answer it always got, which is the point. MEASURED: `hyphenation` at a
 * four-character width breaks at the overflow **4** with no factor and at the hyphenation point **2** with one.
 * **AND THE SCRIPTS UAX #14 ITSELF SENDS TO A
 * DICTIONARY ARE NOT WRAPPED**: `SA` — Thai, Lao, Khmer and friends, "require morphological analysis to determine
 * break opportunities" — resolves to `AL`, whose own rule forbids a break inside an alphabetic run, so such a run is
 * unbreakable here and the answer is the word-doesn't-fit fallback. That is the specification's answer rather than
 * this tier's gap, and it is said here because "UAX #14 is implemented" invites the opposite assumption. **AND HYPHENATION IS NOT MERELY
 * ABSENT HERE, IT IS SOMEWHERE ELSE — WHICH IS A FACT WORTH STATING WHERE A CALLER LOOKS FOR IT.** Core Text's
 * typesetter does not hyphenate either, on the platform this follows; the resource is Core Foundation's, and this tree
 * now answers it (`libagora-core/CFHyphenation.c`: `CFStringGetHyphenationLocationBeforeIndex`). The door for a caller
 * that wants to do the breaking itself is `CTTypesetterSuggestClusterBreak`, named below. Word wrapping here backs off
 * to the last UAX #14 opportunity that fits, which is what Apple's own description of that mode says ("Wrapping occurs at word boundaries,
 * unless the word itself doesn't fit on a single line") and no more than that. A caller typesetting Thai, or expecting
 * hyphenation opportunities, gets the character-level answer instead — and `CTTypesetterSuggestClusterBreak` is the
 * door for a caller that wants the character-level answer ON PURPOSE.
 *
 * AND BIDI IS THE LINE'S, NOT THE TYPESETTER'S — WHICH IS WHY THE OPTIONS CROSS A SEAM RATHER THAN DOING WORK HERE.
 * Apple's default is to apply the Unicode Bidirectional Algorithm, with an option to switch it off, and the algorithm
 * runs in the LINE (see `CTLine.c`). **BOTH OPTIONS ARE OBEYED NOW**, by being read into a `CTLineBidiDirective` and
 * handed to `CTLineCreateWithAttributedStringAndRangeWithBidi` — so the typesetter consults neither while BREAKING
 * (bidi changes no advance, which is why it costs nothing to keep the breaker ignorant of it) and both change the line
 * this typesetter makes. A caller who needs the LEVELS is still owed that door; what is here is the layout, and a line
 * whose glyphs were reordered says so with `kCTRunStatusNonMonotonic`.
 */
#ifndef CORE_TEXT_CTTYPESETTER_H
#define CORE_TEXT_CTTYPESETTER_H

#include <CoreText/CTLine.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE TYPESETTER. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTTypesetterGetTypeID()`. */
typedef const struct __CTTypesetter *CTTypesetterRef;

CFTypeID CTTypesetterGetTypeID(void);

/* THE TWO OPTIONS, AND NEITHER VALUE IS PUBLISHED BY APPLE — both are recorded as the inferences the other
 * unpublished keys in this library are, taking the constant's own name with the leading `k` dropped. Their MEANING
 * is Apple's: the first turns bidi processing off (its value must be a `CFBooleanRef`, default false) and the second
 * forces an embedding level (a `CFNumberRef`, default unset). */
extern const CFStringRef kCTTypesetterOptionDisableBidiProcessing;
extern const CFStringRef kCTTypesetterOptionForcedEmbeddingLevel;

/* **AND A THIRD KEY, WHICH IS THIS TIER'S OWN — RECORDED HERE BECAUSE APPLE DECLARES TWO AND THIS IS NOT ONE OF
 * THEM.** Its value is a `CFNumber`: **0.0 means no hyphenation**, and the number is the share of the width a line
 * must be able to fill before hyphenating is worth it — the hyphenation ZONE, which is how `NSLayoutManager` reads
 * `NSParagraphStyle -hyphenationFactor`, the property this key carries into the engine.
 *
 * WHY IT HAS TO EXIST. The setting is AppKit's: `NSParagraphStyle -hyphenationFactor` is a paragraph property,
 * `CTParagraphStyle` has no specifier for it in any version, and on the platform this library follows the layout
 * manager is what reads the factor and asks `CFStringGetHyphenationLocationBeforeIndex`. A TYPESETTER that can
 * hyphenate therefore needs a channel for that setting, Core Text provides none, and the options dictionary is the
 * surface built for it — a dictionary, with Apple's own two keys as precedent.
 *
 * **AND THE ALTERNATIVE WAS REFUSED RATHER THAN OVERLOOKED: THIS DOOR MUST NOT ALWAYS HYPHENATE.** Two reasons, both
 * properties of a published door. Its answer would depend on the MACHINE — a dictionary present here and absent
 * there would break the same string at a different character — and it would change the meaning of a door Apple
 * documents, for callers that asked for nothing. **A DOOR WHOSE ANSWER CHANGES WITH THE MACHINE IS THE ONE THING A
 * LAYOUT ENGINE MUST NOT HAVE**, so hyphenation is opted into, and the factor is the opt-in. */
extern const CFStringRef kCTTypesetterOptionHyphenationFactor;

/* ---------------------------------------------------------------------------------------
 * CREATION. Both doors take the SAME string and differ only in whether the caller has options to give.
 * ------------------------------------------------------------------------------------- */
CTTypesetterRef CTTypesetterCreateWithAttributedString(CFAttributedStringRef string);

/* **THE TWO OPTIONS ARE OBEYED, AND WHAT THEY ARE OBEYED BY IS THE LINE.** Both are about BIDIRECTIONAL PROCESSING,
 * which is the LINE's (see `CTLine.c`) and not this door's, so `CTTypesetterCreateLine` reads them into a
 * `CTLineBidiDirective` and passes it across the seam. `CTTypesetter.c` records the three readings that are this
 * tier's rather than Apple's: a value of the WRONG TYPE and a forced level that is NOT 0 OR 1 are both IGNORED rather
 * than refused, and `DisableBidiProcessing` WINS over a forced level, because a forced level is an input to an
 * algorithm that is not going to run. */
CTTypesetterRef CTTypesetterCreateWithAttributedStringAndOptions(CFAttributedStringRef string,
								 CFDictionaryRef options);

/* ---------------------------------------------------------------------------------------
 * A LINE FROM IT, OVER A RANGE.
 * ------------------------------------------------------------------------------------- */
/* "If the length portion of range is set to 0, then the typesetter will continue to add glyphs to the line until it
 * runs out of characters in the string. The location and length of the range must be within the bounds of the string,
 * OTHERWISE THE CALL WILL FAIL" — Apple's own sentence, and both halves of it are honoured: a zero length means the
 * rest of the string, and a range that does not lie inside it answers NULL rather than a truncated line. */
CTLineRef CTTypesetterCreateLine(CTTypesetterRef typesetter, CFRange stringRange);

/* ---------------------------------------------------------------------------------------
 * WHERE THE NEXT LINE STARTS.
 * ------------------------------------------------------------------------------------- */
/* THE COUNT OF CHARACTERS FROM `startIndex` THAT WOULD CAUSE THE LINE BREAK, which is what a caller feeds straight
 * back into `CTTypesetterCreateLine` as a range's length. The breaking is the contextual one — the paragraph
 * style's line break mode is read and obeyed for the two WRAPPING modes — and the number returned is never less
 * than 1 unless the caller is already at the end of the string, because a line that holds nothing would loop
 * forever in the caller's own loop over the text. */
CFIndex CTTypesetterSuggestLineBreak(CTTypesetterRef typesetter, CFIndex startIndex, double width);

/* THE SAME QUESTION, ASKED OF A TYPOGRAPHIC CLUSTER RATHER THAN A LINE. Apple's own description of the difference is
 * exact: this "is similar to a character break, except that it will not break apart linguistic clusters. NO OTHER
 * CONTEXTUAL ANALYSIS WILL BE DONE" — so THIS TIER'S TWO DOORS DIFFER IN EXACTLY ONE WAY, and it is the way Apple
 * names: this one never backs off to a word boundary, and `SuggestLineBreak` does. A caller implementing hyphenation
 * is the caller this door is for. */
CFIndex CTTypesetterSuggestClusterBreak(CTTypesetterRef typesetter, CFIndex startIndex, double width);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTTYPESETTER_H */
