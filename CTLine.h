/*
 * CTLine.h — the line: a string and its glyphs in an order, which is the first thing in this library that is
 * not a setting or a face but a RESULT.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). Its includes are Apple's and they are worth stating because
 * of what is MISSING from them: `CFArray.h`, `CFAttributedString.h` and `CGContext.h` — NOT `CTRun.h` and not
 * `CTFont.h`, even though a line is made of runs and set in a font. Apple's header is self-contained, and that is
 * a fact about the SURFACE rather than about the implementation.
 *
 * A LINE IS ONE PASS OVER THE STRING, AND THAT IS THE BOUNDARY THIS TIER DRAWS. Apple's own sentence for the
 * main creator is that "without a typesetter object, the line cannot be properly broken" — the line is the
 * SIMPLE case, and the breaking is the typesetter's. WHAT THIS TIER BUILDS IS ONE RUN: the characters mapped to
 * glyphs through the font the attributed string names (`CTFontGetGlyphsForCharacters` — the door this library
 * exists for), positioned by that font's own advances (`CTFontGetAdvancesForGlyphs`), measured with its
 * ascent, descent and leading. SO EVERY CHARACTER IN THE STRING IS IN THE LINE, the string range is the whole
 * string, and the width is the sum of the advances.
 *
 * WHAT IS THEREFORE NOT ATTEMPTED, SAID PLAINLY: MULTI-RUN LINES, BIDIRECTIONAL TEXT, AND SHAPING. A line with
 * two fonts, a right-to-left run inside a left-to-right one, or a script needing ligatures or contextual forms
 * is NOT laid out correctly here — every glyph is drawn in the order its character appeared. That is the
 * HarfBuzz milestone this project's Core Text plan already names, and it is not a signature's problem.
 *
 * AND THREE DOORS ARE OWED, EACH DECLARED AND NOT DEFINED, so a caller who reaches for one gets a link error
 * rather than an invented answer. `CTLine.c` carries the reason for each; the shortest of the three is the one
 * worth knowing before reading on: A LINE'S RUNS ARE `CTRunRef`s, AND `CTRun` DOES NOT EXIST YET — Apple's own
 * include order puts its header after this one, which is why this header never mentions it.
 */
#ifndef CORE_TEXT_CTLINE_H
#define CORE_TEXT_CTLINE_H

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFAttributedString.h>
#include <CoreGraphics/CGContext.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE LINE. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTLineGetTypeID()`. */
typedef const struct __CTLine *CTLineRef;

/* WHICH END OF A LINE A TRUNCATION EATS, for the door this tier owes. Apple's values are printed in its header
 * and are named here so that the day the door lands the numbering is already held. */
enum {
	kCTLineTruncationStart = 0,	/* the end stays visible */
	kCTLineTruncationEnd = 1,	/* the beginning stays visible */
	kCTLineTruncationMiddle = 2	/* both ends stay visible */
};
typedef uint32_t CTLineTruncationType;

CFTypeID CTLineGetTypeID(void);

/* ---------------------------------------------------------------------------------------
 * CREATION. One door is answered and two are owed.
 * ------------------------------------------------------------------------------------- */
/* THE WHOLE STRING, AS ONE RUN, IN THE FONT THE ATTRIBUTED STRING NAMES. NULL when there is no such font: a
 * line has no default face to fall back to, because Apple's contract is that the caller supplies one AS AN
 * ATTRIBUTE, and substituting a face the caller did not ask for is exactly the inventing this library refuses.
 * AND A STRING OF LENGTH ZERO ANSWERS NULL for the same reason: there is no character to carry the attribute, so
 * there is no font to be found. Recorded because a caller WILL hit it — an editor asks for the line of an empty
 * document — and "there is nothing to lay out" is what NULL says.
 *
 * THE ATTRIBUTE IS READ BY ITS NAME, and the name is a boundary this file's implementation records: the key is
 * `kCTFontAttributeName`, whose header is later in this library's order, so the value is written in `CTLine.c`
 * where the constant will go. */
CTLineRef CTLineCreateWithAttributedString(CFAttributedStringRef string);

/* ---------------------------------------------------------------------------------------
 * WHAT A LINE SAYS ABOUT ITSELF.
 * ------------------------------------------------------------------------------------- */
/* HOW MANY GLYPHS ARE IN IT, and the door has no NULL case to describe: an invalid line answers 0, which is
 * also a legal answer for an empty one. */
CFIndex CTLineGetGlyphCount(CTLineRef line);

/* THE CHARACTERS THAT SPAWNED THE GLYPHS. For this tier every character did, so the range is the whole string —
 * and Apple's failure answer is an EMPTY range, which is what an invalid line gives back. */
CFRange CTLineGetStringRange(CTLineRef line);

/* ---------------------------------------------------------------------------------------
 * MEASUREMENT.
 * ------------------------------------------------------------------------------------- */
/* THE LINE'S WIDTH IN POINTS, with the ascent, descent and leading written to the pointers that are not NULL.
 * THE THREE HEIGHTS COME FROM THE FONT rather than from the glyphs actually present, which is what Apple's
 * framesetter needs to place line after line without measuring glyph by glyph; the WIDTH comes from the glyphs,
 * because a font cannot know which of its faces a line uses. An invalid line answers 0 and writes nothing. */
double CTLineGetTypographicBounds(CTLineRef line, CGFloat *ascent, CGFloat *descent, CGFloat *leading);

/* THE WIDTH OF THE WHITESPACE AT THE LINE'S END, which Apple's own note explains the need for: a line measured
 * for a width can come out LONGER because of trailing whitespace, and this door says how much of it that is. */
double CTLineGetTrailingWhitespaceWidth(CTLineRef line);

/* THE RECTANGLE THAT TIGHTLY ENCLOSES THE LINE'S INK, for a given context — Apple requires the context because
 * its settings can change the answer. Returns CGRectNull for an invalid line, which is Apple's own answer. THE
 * CONTEXT'S TEXT POSITION IS APPLIED, because Apple's sentence says the rectangle is "translated by the supplied
 * context's text position". */
CGRect CTLineGetImageBounds(CTLineRef line, CGContextRef context);

/* THE PEN OFFSET THAT MAKES A LINE FLUSH: 0 or less is left, 1.0 or more is right, and between them are the
 * degrees of centre with 0.5 exactly centred. Pure arithmetic on the line's width, which is why it needs no
 * engine — and Apple's own failure answer for an invalid line is 0.0. */
double CTLineGetPenOffsetForFlush(CTLineRef line, CGFloat flushFactor, double flushWidth);

/* DRAW IT. Apple's note is that this "may leave the graphics context in any state and does not flush", so
 * nothing is saved or restored and nothing is flushed here either. */
void CTLineDraw(CTLineRef line, CGContextRef context);

/* ---------------------------------------------------------------------------------------
 * CARET POSITIONING, which is the inverse pair a text editor lives on.
 * ------------------------------------------------------------------------------------- */
/* THE STRING INDEX FOR A POINT relative to the line's origin, answered within the line's own range: no less
 * than its first index and no greater than one past its last. Apple's failure answer is `kCFNotFound`. */
CFIndex CTLineGetStringIndexForPosition(CTLineRef line, CGPoint position);

/* THE OFFSET ALONG THE BASELINE FOR A STRING INDEX, with the secondary offset written out when the caller wants
 * it — the same value as the primary when a single caret suffices, which is the whole of this tier's case
 * because a single run has one caret per boundary. Apple's failure answer is 0.0. */
CGFloat CTLineGetOffsetForStringIndex(CTLineRef line, CFIndex charIndex, CGFloat *secondaryOffset);

/* ---------------------------------------------------------------------------------------
 * OWED DOORS, EACH DECLARED HERE AND NOT DEFINED. `CTLine.c` carries the reasons.
 * ------------------------------------------------------------------------------------- */
CFArrayRef CTLineGetGlyphRuns(CTLineRef line);

CTLineRef CTLineCreateTruncatedLine(CTLineRef line, double width, CTLineTruncationType truncationType,
				    CTLineRef truncationToken);

CTLineRef CTLineCreateJustifiedLine(CTLineRef line, CGFloat justificationFactor, double justificationWidth);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTLINE_H */
