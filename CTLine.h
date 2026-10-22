/*
 * CTLine.h — the line: a string and its glyphs in an order, which is the first thing in this library that is
 * not a setting or a face but a RESULT.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). Its includes are Apple's and they are worth stating because
 * of what is MISSING from them: `CFArray.h`, `CFAttributedString.h` and `CGContext.h` — NOT `CTRun.h` and not
 * `CTFont.h`, even though a line is made of runs and set in a font. Apple's header is self-contained, and that is
 * a fact about the SURFACE rather than about the implementation.
 *
 * A LINE IS ONE PASS OVER THE STRING, AND THAT IS THE BOUNDARY THIS TIER DRAWS. Apple's own sentence for the
 * main creator is that "without a typesetter object, the line cannot be properly broken" — the line is the
 * SIMPLE case, and the breaking is the typesetter's. WHAT THIS TIER BUILDS IS ONE GLYPH PER CHARACTER, MAPPED
 * THROUGH THE FACE THAT CHARACTER IS SET IN (`CTFontGetGlyphsForCharacters` — the door this library exists for),
 * positioned by that font's own advances (`CTFontGetAdvancesForGlyphs`), measured with its ascent, descent and
 * leading. SO EVERY CHARACTER IN THE STRING IS IN THE LINE, the string range is the whole string, and the width is
 * the sum of the advances.
 *
 * **AND THE LINE IS PARTITIONED INTO FONT RUNS, ONE PER FACE THE STRING ASKS FOR.** A string set entirely in one
 * face is ONE run, which is the common case and the one every other door was written against; a string that changes
 * face in the middle is TWO or more, each with its own font, its own slice of the string, and glyphs INDEXED IN THAT
 * FACE — because a glyph index means nothing without the face it came from. `CTLineGetGlyphRuns` is where the
 * partition is visible, and `CTLine.c` records that a run boundary is where the FONT changes and not where the
 * attribute dictionary does, so a run delegate or a tab in the middle does NOT split a run.
 *
 * WHAT THIS HEADER STILL DOES NOT PROMISE, SAID PLAINLY: SHAPING. A script needing ligatures or contextual forms is
 * NOT laid out correctly here, and neither is a line whose glyphs are not one per character — **BECAUSE THE SHAPING
 * ENGINE IS BUILT BUT NOT YET WIRED IN** (`CTShaping.c`, over HarfBuzz, measured against five faces; the line's
 * arrays, its caret doors and the AppKit's glyph ranges all assume one glyph per character, so connecting the two is
 * one change and is the next slice). **AND THE GLYPHS ARE REORDERED,
 * WHICH IS THE BIDI MILESTONE'S PAYOFF**: the line runs the whole pipeline over its own characters, PLACES each glyph
 * where the visual order says, and reports `kCTRunStatusNonMonotonic` — so a Hebrew letter in a Latin paragraph is
 * drawn to the right of the one before it. THE CARET DOORS SPEAK VISUAL ORDER, and an offset is the character's
 * LEADING edge — its LEFT one at an even level and its RIGHT one at an odd level. The HarfBuzz milestone this
 * project's Core Text plan names is therefore HALF DONE: the engine answers for ligatures, marks, conjuncts and
 * joined scripts, and what is left is the correspondence change that lets this type hold its answers.
 *
 * **A RUN DELEGATE, THOUGH, IS HONORED.** A character whose attributes carry `kCTRunDelegateAttributeName` is a BOX:
 * its advance is the delegate's width rather than the placeholder glyph's, the line's ascent and descent grow to cover
 * the delegate's, and no ink is drawn for it — which is how an attachment, an inline picture or a tab occupies space
 * without being a glyph. It changes the ADVANCE and not the FACE (see `CTRunDelegate.h`), so it does not split a run.
 *
 * **AND EVERY DOOR THIS HEADER DECLARES IS NOW IMPLEMENTED**: nothing here is left declared-and-not-defined, so a
 * caller cannot reach a LINK ERROR through this file. The doors that once waited — `CTLineGetGlyphRuns` and
 * `CTLineCreateTruncatedLine` among them — have all landed, and `CTLine.c` records the reason each one waited where
 * that reason is still worth knowing. THE ONE NAMING FACT TO KEEP: `CTRun` arrived WITH `CTLineGetGlyphRuns`, and this
 * header still names no run type, because Apple's include order puts `CTRun.h` AFTER this one — which is why the door
 * answers a `CFArrayRef` and its comment says so.
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
/* THE WHOLE STRING, IN THE FACES THE ATTRIBUTED STRING NAMES — ONE RUN PER FACE. NULL when there is no font at the
 * line's first character: a
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

/* THE RUNS THAT MAKE THE LINE UP, which is where this header meets `CTRun` — a type whose OWN header comes AFTER
 * this one in Apple's include order, and which is therefore not named here at all: what comes back is a
 * `CFArrayRef` of them. THE REFERENCE IS BORROWED, owned by the line — Apple's name for this door is `Get` and not
 * `Copy`, and its documentation says nothing about releasing. **THIS TIER'S LINES HAVE ONE RUN PER FACE THEY ARE SET
 * IN** (see this file's head): a string in one font is ONE run, and a string that changes face is TWO, each run
 * carrying its own font and the glyphs indexed in it. */
CFArrayRef CTLineGetGlyphRuns(CTLineRef line);

/* ---------------------------------------------------------------------------------------
 * THE TWO DOORS THAT TRUNCATE A LINE, ONE OF THEM ANSWERED NOW. THE TRUNCATION ENGINE IS THE FIRST DOOR OF THIS
 * HEADER TO HAVE BEEN BUILT FROM THE TYPESETTER'S ARRIVAL — its reason for waiting was "it waits for the typesetter,
 * whose job the breaking is" — AND `CTLineCreateTruncatedLine` LEFT THE OWED LIST IN THE SAME COMMIT, which the
 * `owed` test requires: it fails when an owed door acquires a definition. The three types are Apple's own descriptions
 * of which end stays visible; see `CTLine.c` for the two things its header does not print, one of which is refused.
 * ------------------------------------------------------------------------------------- */
CTLineRef CTLineCreateTruncatedLine(CTLineRef line, double width, CTLineTruncationType truncationType,
				    CTLineRef truncationToken);

/* ---------------------------------------------------------------------------------------
 * THE LAST DOOR OF THIS HEADER, AND WITH IT `CTLine` OWES NOTHING. `CTLineCreateJustifiedLine` left the owed list in
 * the commit that decided WHERE justification slack goes — the one thing its header does not print — and that decision
 * is written down in `CTLine.c` rather than presented as Apple's. The other two doors this header once owed left the
 * same way, each when the condition its reason named became false.
 * ------------------------------------------------------------------------------------- */
CTLineRef CTLineCreateJustifiedLine(CTLineRef line, CGFloat justificationFactor, double justificationWidth);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTLINE_H */
