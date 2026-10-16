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
 * WHAT IS NOT ATTEMPTED, SAID WHERE A CALLER READS IT. The Unicode line-breaking rules (UAX #14), hyphenation, and the
 * rules for scripts whose words are not separated by spaces are NOT implemented: word wrapping here backs off to the
 * last SPACE that fits, which is what Apple's own description of that mode says ("Wrapping occurs at word boundaries,
 * unless the word itself doesn't fit on a single line") and no more than that. A caller typesetting Thai, or expecting
 * hyphenation opportunities, gets the character-level answer instead — and `CTTypesetterSuggestClusterBreak` is the
 * door for a caller that wants the character-level answer ON PURPOSE.
 *
 * AND BIDI IS HALF-READ RATHER THAN PRETENDED. Apple's default is to apply the Unicode Bidirectional Algorithm, with
 * an option to switch it off — THIS TIER NEVER REORDERS ANYTHING YET, so the option is RECORDED and not obeyed, and a
 * caller who needs directionality is not answered by pretending. What the milestone has STARTED with is the base
 * writing direction: a LINE reads the paragraph's direction and its run reports it (`CTRunGetStatus`), while the
 * algorithm that would reorder the glyphs is the next slice. See `CTTypesetter.c` and `CTLine.c`.
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

/* ---------------------------------------------------------------------------------------
 * CREATION. Both doors take the SAME string and differ only in whether the caller has options to give.
 * ------------------------------------------------------------------------------------- */
CTTypesetterRef CTTypesetterCreateWithAttributedString(CFAttributedStringRef string);

/* THE OPTIONS ARE RECORDED AND NOT OBEYED, which is the difference between a door that cannot do something and one
 * that pretends it did: both of these options are about BIDIRECTIONAL PROCESSING, and this tier does none — so
 * neither `DisableBidiProcessing` nor a forced embedding level can change its answer, and a caller that needs the
 * algorithm is told so in `CTTypesetter.c` rather than given a plausible wrong layout. */
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
