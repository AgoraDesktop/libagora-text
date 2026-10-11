/*
 * CTTypesetter.h — line breaking: which characters make a line, given a width.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A TYPESETTER IS THE THIRD THING CORE TEXT DOES**, after the font (M1) and the shaped line (M2a): it
 * takes a string and a WIDTH and answers WHERE THE LINE ENDS. Apple's model is that a line is created FOR
 * A RANGE — `CTTypesetterCreateLine` — and that the range is chosen by `CTTypesetterSuggestLineBreak`,
 * which is what makes a paragraph a loop rather than a data structure: the caller breaks, draws, advances
 * `startIndex`, and breaks again.
 *
 * **THE LINE-BREAKING ALGORITHM HERE IS THE GREEDY WORD WRAP, NAMED RATHER THAN IMPLIED.** Apple's own
 * documentation says the door "suggests" a break and that a caller may "implement a different line breaking
 * scheme, such as hyphenation" — i.e. the door is a SERVICE, not a specification of one algorithm. This
 * tier's service breaks at the last whitespace that fits, and breaks inside a word only when no whitespace
 * fits at all (the forced break an over-long word needs). **UAX #14's rules and the paragraph style's
 * hyphenation are the paragraph unit's** (`CTParagraphStyle`, M4) and are named there rather than
 * half-implemented here.
 */
#ifndef CORE_TEXT_CTTYPESETTER_H
#define CORE_TEXT_CTTYPESETTER_H

#include <CoreGraphics/CGBase.h>
#include <CoreText/CTLine.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTTypesetter;
#else
typedef struct objc_object CTTypesetter;
#endif
typedef CTTypesetter *CTTypesetterRef;

/* THE TYPESETTER HOLDS THE STRING EVERY DOOR BELOW MEASURES, which is why it is created from one rather
 * than from its pieces (Apple's door takes the attributed string and nothing else). **NO FONT, NO
 * TYPESETTER**, for the reason `CTLine.h` records: a string whose attributes name no font cannot be
 * measured, and this tier answers NULL rather than substituting one. */
CTTypesetterRef CTTypesetterCreateWithAttributedString(NSAttributedString *string);

/* **A LINE FOR A RANGE.** Apple's door is exactly this: the line covers the range, and the caller decides
 * where that range begins and ends (usually from the door below). */
CTLineRef CTTypesetterCreateLine(CTTypesetterRef typesetter, NSRange range);

/* **WHERE THE LINE THAT STARTS AT `start` SHOULD END**, given a width — the index just PAST the last
 * character that fits, which is where the next line begins. Three things about the answer are this door's
 * contract rather than its implementation: it is **at least one character** past `start` (so a caller
 * looping on it always advances), it is **at most the string's end**, and when a whitespace fits the break
 * lands JUST AFTER it — which is what makes the next line start on a word. */
long CTTypesetterSuggestLineBreak(CTTypesetterRef typesetter, long start, double width);

/* **M7d: THE OPTIONS, AND THE ONE THIS TIER CANNOT HONOUR.** Apple's door takes a dictionary of options, and
 * BOTH of its names are about the Unicode Bidirectional Algorithm (UAX #9):
 *
 *   * `kCTTypesetterOptionDisableBidiProcessing` — "if a typesetter is created with this option set to true,
 *     no directional reordering is performed and any directional control characters are ignored";
 *   * `kCTTypesetterOptionForcedEmbeddingLevel` — "specifies the embedding level", and "any directional
 *     control characters are ignored".
 *
 * **THIS TIER RUNS NO BIDI PASS AT ALL.** The shaper is handed HarfBuzz's own guess for the direction
 * (`CTLine.m` records that), and none of the reordering UAX #9 describes exists here. So the FIRST option is
 * SATISFIED BY CONSTRUCTION — there is no reordering to disable and no control character being honoured — and
 * the probe checks that the typesetter it answers is the SAME one the plain door answers, break for break.
 * THE SECOND IS NOT SATISFIABLE AND IS **REFUSED BY NAME**: a caller asking for a specific embedding level is
 * asking for a layout this tier cannot produce, and answering a typesetter that quietly ignored the level
 * would be a wrong paragraph rather than a missing one. Apple's own result for a creation that fails is NULL,
 * which is what the refusal returns.
 *
 * The VALUES are ours (Apple publishes no string for either key), which is what makes them safe: an option
 * key is only ever compared with the constant declared beside it. */
CTTypesetterRef CTTypesetterCreateWithAttributedStringAndOptions(NSAttributedString *string,
								NSDictionary *options);
extern NSString *const kCTTypesetterOptionDisableBidiProcessing;
extern NSString *const kCTTypesetterOptionForcedEmbeddingLevel;

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S — the pair every Core Text object carries, for
 * the reason `CTFont.h` gives. No ledger row is claimed for it. */
CTTypesetterRef CTTypesetterRetain(CTTypesetterRef typesetter);
void CTTypesetterRelease(CTTypesetterRef typesetter);

/* **AND THE ONE DOOR THAT IS NOT HERE, NAMED SO THAT A READER KNOWS IT WAS CONSIDERED.**
 * `CTTypesetterSuggestClusterBreak` is owed, and THIS unit's reconnaissance is what moved it: Apple specifies
 * that "a typographic cluster break can also be triggered by a HARD BREAK CHARACTER in the stream", and this
 * tier does NOT yet end a line at a hard break — the framesetter's loop lays a `\n` that fits inside the width
 * out as an ordinary glyph. So the cluster break and the paragraph's own newline handling want ONE
 * implementation of "a hard break ends a line", which is the paragraph unit's work rather than a second walk
 * here; writing it here alone would put the two doors into disagreement about what a break IS.
 *
 * **`CTTypesetterGetTypeID` IS DECLARED NOW** (CT-M7g): it returns a `CFTypeID`, which this tree SPELLS and
 * has been minting all along — `CTTypeID_internal.h` is the file that says so. */
CFTypeID CTTypesetterGetTypeID(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTTYPESETTER_H */
