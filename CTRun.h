/*
 * CTRun.h — the run: the piece of a line that shares one set of attributes, and the glyphs it is made of.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). Apple's includes for it are `CFDictionary.h` and
 * `CGContext.h` — a run is attributes, glyphs, numbers and a surface to draw on — and THIS IS THE HEADER
 * `CTLine.h` COULD NOT MENTION: Apple's include order puts CTRun.h AFTER CTLine.h, which is why `CTLineGetGlyphRuns`
 * was owed with "a line's runs are `CTRunRef`s and `CTRun` does not exist yet" as its reason. That door lands with
 * this type, in the same commit.
 *
 * A RUN IS INDEPENDENTLY RETAINABLE, WHICH DECIDES HOW IT IS BUILT. A caller can hold a run after releasing the
 * line it came from, so a run CANNOT be a view over its line's storage: it OWNS its glyphs, positions, advances
 * and string indices. That is why every glyph a line sets is carried TWICE — once in the line's own arrays and
 * once in the run that slices them, per run — and why that cost is worth paying rather than answering a retained
 * run with a dangling pointer.
 *
 * AND IT IS WHY THE "Ptr" DOORS HERE NEVER ANSWER NULL. Apple documents each of them as possibly returning NULL
 * "even if there are glyphs in the stream", with the caller obliged to fall back to the copying door; that is a
 * contract about a run that may keep its glyphs somewhere other than one contiguous block. THIS RUN ALWAYS KEEPS
 * THEM CONTIGUOUS AND OWNS THEM, so the pointer is always there — which is a stronger answer than Apple's, and is
 * recorded as the fact about this implementation that it is.
 *
 * **AND ONE OF THE TWO ANSWERS THIS TIER GIVES IS NOW READ RATHER THAN FIXED.** `CTRunGetStatus` reports the LINE's
 * base writing direction — `kCTRunStatusRightToLeft` when the paragraph runs that way, and `NoStatus` otherwise — AND
 * `kCTRunStatusNonMonotonic` when the LINE REORDERED its glyphs, which it does when the bidirectional pipeline says
 * the visual order is not the logical one, so a run's string indices are no longer monotonic. `CTRunGetTextMatrix`
 * is still the identity: the line reorders POSITIONS and sets no run matrix.
 */
#ifndef CORE_TEXT_CTRUN_H
#define CORE_TEXT_CTRUN_H

#include <CoreFoundation/CFDictionary.h>
#include <CoreGraphics/CGContext.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE RUN. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTRunGetTypeID()`. */
typedef const struct __CTRun *CTRunRef;

/* WHAT A RUN CAN SAY ABOUT ITSELF BEYOND ITS ATTRIBUTES, and Apple's own reason for the bits is that knowing them
 * "can aid in string index analysis" and "avoid expensive comparisons". NONE OF THEM IS SET HERE: a run this tier
 * makes is left-to-right, monotonic, and drawn under the identity matrix. */
enum {
	kCTRunStatusNoStatus = 0,
	kCTRunStatusRightToLeft = (1 << 0),		/* the run is right to left */
	kCTRunStatusNonMonotonic = (1 << 1),		/* reordered: string indices are no longer monotonic */
	kCTRunStatusHasNonIdentityMatrix = (1 << 2)	/* needs a specific text matrix to draw correctly */
};
typedef uint32_t CTRunStatus;

CFTypeID CTRunGetTypeID(void);

/* ---------------------------------------------------------------------------------------
 * WHAT A RUN IS MADE OF.
 * ------------------------------------------------------------------------------------- */
CFIndex CTRunGetGlyphCount(CTRunRef run);

/* THE ATTRIBUTES THE RUN WAS LAID OUT WITH. Apple's own sentence allows either "the same exact one that was set as
 * an attribute dictionary on the original attributed string" or one "manufactured by the layout engine" for
 * substitution or missing keys — THIS TIER HAS NOTHING TO MANUFACTURE, so what comes back is the attributed
 * string's own dictionary for the run's range. NULL for an invalid run, which is Apple's other answer. */
CFDictionaryRef CTRunGetAttributes(CTRunRef run);

CTRunStatus CTRunGetStatus(CTRunRef run);

/* THE FOUR ARRAYS, EACH WITH A POINTER DOOR AND A COPYING DOOR — and the pointer doors never answer NULL here
 * (see this file's head). The copying doors take a RANGE, and the rule for it is Apple's and is applied once:
 * "If the length of the range is set to 0, then the copy operation will continue from the range's start index to
 * the end of the run." */
const CGGlyph *CTRunGetGlyphsPtr(CTRunRef run);
void CTRunGetGlyphs(CTRunRef run, CFRange range, CGGlyph buffer[]);

/* POSITIONS ARE RELATIVE TO THE ORIGIN OF THE LINE THE RUN IS IN — Apple says so twice, once per door — which is
 * why a run cannot be drawn from its positions alone without knowing where the line put it. */
const CGPoint *CTRunGetPositionsPtr(CTRunRef run);
void CTRunGetPositions(CTRunRef run, CFRange range, CGPoint buffer[]);

/* AND THE ADVANCES ARE NOT A SUBSTITUTE FOR THE POSITIONS, which is Apple's own warning on this door: "advances
 * alone are not sufficient for correctly positioning glyphs … as a run may have a non-identity matrix or the
 * initial glyph in a line may have a non-zero origin". */
const CGSize *CTRunGetAdvancesPtr(CTRunRef run);
void CTRunGetAdvances(CTRunRef run, CFRange range, CGSize buffer[]);

/* THE CHARACTERS THAT SPAWNED EACH GLYPH, which is how a caller maps a glyph back into the string. */
const CFIndex *CTRunGetStringIndicesPtr(CTRunRef run);
void CTRunGetStringIndices(CTRunRef run, CFRange range, CFIndex buffer[]);

CFRange CTRunGetStringRange(CTRunRef run);

/* THE WIDTH OF A RANGE OF THE RUN, with the heights of its font. A range is measurable in part and not as a
 * whole, which is why these two doors take one — unlike the line's own bounds doors. */
double CTRunGetTypographicBounds(CTRunRef run, CFRange range, CGFloat *ascent, CGFloat *descent, CGFloat *leading);

/* THE INK OF A RANGE, "translated by the supplied context's text position and the positions of the individual
 * glyphs" — Apple's sentence, and the reason the context is a parameter rather than an accident. */
CGRect CTRunGetImageBounds(CTRunRef run, CGContextRef context, CFRange range);

/* THE MATRIX THE RUN MUST BE DRAWN UNDER, which Apple's note says the caller should fill in with the current text
 * position before using. THE IDENTITY HERE (see this file's head). */
CGAffineTransform CTRunGetTextMatrix(CTRunRef run);

/* DRAW A RANGE OF IT. Apple's note is that this "may leave the graphics context in any state and does not flush",
 * and that an UNDERLINE is not drawn by this door because its appearance depends on the rest of the line — this
 * tier draws no underline at all, so there is nothing to leave out. */
void CTRunDraw(CTRunRef run, CGContextRef context, CFRange range);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTRUN_H */
