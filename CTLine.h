/*
 * CTLine.h — a line of text: the shaped runs, and the numbers a caller lays it out with.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THIS IS WHERE CORE TEXT STARTS SHAPING, AND IT IS M2's FIRST DOOR.** A line is made from an
 * attributed string that carries a font (`kCTFontAttributeName`), it is shaped by HARFBUZZ with `hb-ft`
 * so that the shaper and the metrics read ONE face (`cg_font_face`, the seam the CoreGraphics font
 * already opened), and what comes out is the runs — glyphs, positions, advances and the cluster mapping.
 *
 * WHAT A LINE KNOWS HOW TO SAY: its glyph count, its runs, its typographic bounds (the width and the
 * ascent/descent/leading a caller lays it out with), and its trailing whitespace width. **APPLE'S
 * RANGED DOORS ARE NOT HERE AND ARE NAMED RATHER THAN MISSING** — `CTLineGetStringRange` returns a
 * `CFRange` and `CTLineGetImageBounds` needs a context, so the first waits on the range-type decision
 * (`CTRun.h` records it in full) and the second belongs to M3's drawing unit.
 *
 * THE MODEL IS ONE RUN PER LINE FOR NOW, and that is a stated boundary rather than a shortcut: a line
 * whose attributes change mid-string becomes several runs once the attribute-splitting half of M2 lands
 * (M2b), which is also where `CTTypesetter` (line breaking) and the explicit script/language door live.
 */
#ifndef CORE_TEXT_CTLINE_H
#define CORE_TEXT_CTLINE_H

#include <CoreGraphics/CGBase.h>
#include <CoreText/CTRun.h>
/* AND `CGContext.h` FOR ONE REASON: the drawing doors below take a `CGContextRef`, and that type is
 * declared THERE rather than in `CGBase.h` (measured) — a header that names a type includes the header that
 * declares it, and the alternative (`void *` or a second typedef of another header's type) would be a lie
 * about the signature. */
#include <CoreGraphics/CGContext.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __OBJC__
@class CTLine;
#else
typedef struct objc_object CTLine;
#endif
typedef CTLine *CTLineRef;

/* A LINE IS MADE FROM AN ATTRIBUTED STRING, and the font comes out of it: **NO FONT, NO LINE** — Apple's
 * door has no size parameter and no font parameter either, so a string with no `kCTFontAttributeName` is
 * a string this tier cannot shape, and it answers NULL rather than substituting one. */
CTLineRef CTLineCreateWithAttributedString(NSAttributedString *string);

long CTLineGetGlyphCount(CTLineRef line);

/* THE RUNS, RETAINED (+1). One array element per run, and one run per line until M2b. */
NSArray *CTLineGetGlyphRuns(CTLineRef line) CG_RETURNS_RETAINED;

/* **THE WIDTH, AND THE THREE METRICS A CALLER NEEDS TO PLACE THE LINE.** Apple's sentence: "the
 * typographic bounds ... the width of the line" with ascent/descent/leading written back if the caller
 * asks for them; the width is THE SUM OF THE RUNS' ADVANCES (the plan's own proof shape: "a line
 * measured as the sum of its runs"). */
double CTLineGetTypographicBounds(CTLineRef line, CGFloat *ascent, CGFloat *descent, CGFloat *leading);

/* "THE WIDTH OF THE TRAILING WHITESPACE IN THE LINE" — the advances of the trailing glyphs whose clusters
 * fall on whitespace characters, 0 when the line does not end in whitespace. */
double CTLineGetTrailingWhitespaceWidth(CTLineRef line);

/* THE RANGE OF THE STRING THE LINE WAS MADE FROM — "the range of characters that originally spawned the
 * glyphs in the line", in this tier's byte units (the cluster map's units, `CTRun.h` records why). It is
 * the union of its runs' ranges, so it needs no extra state and cannot disagree with them. */
NSRange CTLineGetStringRange(CTLineRef line);

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S — the same pair every Core Text object
 * carries, for the reason `CTFont.h` gives: Apple's line is a CFType and this tree has no CoreFoundation.
 * No ledger row is claimed for it. */
CTLineRef CTLineRetain(CTLineRef line);
void CTLineRelease(CTLineRef line);

/* **AND THE TYPE IDENTITY (CT-M7g)** — `CTLineGetTypeID`'s absence was never recorded in this header, so there
 * is no sentence to correct; the argument for all of them is in `CTTypeID_internal.h`. */
CFTypeID CTLineGetTypeID(void);

/* ------------------------------------------------------------------------------------------------
 * CT-M3: THE DRAWING DOORS, EXPRESSED OVER THE SURVIVING COREGRAPHICS GLYPH DOORS.
 *
 * That is the plan's rule 1 in as many words: the bodies reach `CGContextSetFont`,
 * `CGContextSetFontSize`, `CGContextSetTextMatrix` and `CGContextShowGlyphsAtPositions` — **the ones Apple
 * did NOT deprecate** — and never the doors it did, which `coregraphics-plan` §15 measured the rules of
 * (the mask route's device-coordinate placement, the pen rounding, the flip). The surface is the
 * CONTEXT'S, so a caller decides what is being drawn into.
 *
 * **`CTFontDrawGlyphs` IS NOT HERE, AND THAT IS A MEASUREMENT RATHER THAN AN OMISSION**: it is `struck` in
 * this plan's ledger — a door the 10.6 SDK does not mention at all — so the era-correct drawing surface is
 * the LINE and the RUN, which is what a text system draws through anyway.
 *
 * THE TEXT MATRIX IS SET FROM THE FONT'S OWN MATRIX before the glyphs go out (this tier's reading of what
 * a font's matrix means for a line); the context's own CTM is applied by the CoreGraphics door itself, as
 * its documentation says.
 * ---------------------------------------------------------------------------------------------- */
void CTLineDraw(CTLineRef line, CGContextRef context);

/* AND THE RUN'S, over a GLYPH range of it — Apple's signature, now spellable because the range type is
 * (`dec-5245eae215922f08`). A length of 0 means the whole run, the same empty-range rule the accessors
 * follow. */
void CTRunDraw(CTRunRef run, CGContextRef context, NSRange range);


/* **THE VALUES ARE OURS**: Apple publishes enum case NAMES and no values, and the tree's Core Text
 * vocabulary follows the same rule as the rest of it — the default or "none" case is 0, a MASK's cases are
 * powers of two, and everything else is the order the era's header declares them in. What is NOT ours is the
 * set of names: every one below is a name 10.6's own headers carry. */
/* the start/end/middle order the era declares (Apple's own home). */
typedef enum {
	kCTLineTruncationStart = 0,
	kCTLineTruncationEnd = 1,
	kCTLineTruncationMiddle = 2
} CTLineTruncationType;

/* **M7c: THE LINE RESHAPED TO A MEASURE — TRUNCATION AND JUSTIFICATION.** Both take a line and answer a NEW
 * line, and both are exact functions of the original's own runs rather than new shaping: the glyphs, their
 * advances and the cluster mapping come from the ORIGINAL runs, so a reshaped line cannot disagree with the
 * line it came from about what the text says.
 *
 * `CTLineCreateTruncatedLine` — Apple's contract in its own words: the line "will be truncated if its width is
 * greater than the width passed in"; the token "will be added to the point where truncation took place"
 * (the ellipsis character usually); "If this parameter is set to NULL, then no truncation token is used, and
 * the line is simply cut off"; and "if the width of the line specified in truncationToken is greater, this
 * function will return NULL if truncation is needed". **TWO THINGS THE ERA DOES NOT SETTLE ARE THIS TIER'S AND
 * ARE SAID HERE**: a line that already FITS is answered as ITSELF (retained) rather than as NULL, because
 * "this line, at most this wide" is what the caller asked for and needing no truncation is a successful
 * answer; and the cut falls on a GLYPH boundary, which is what a shaped line offers. The three types keep the
 * portion Apple's own doc block names — Start leaves the END visible, End leaves the START, Middle both.
 *
 * `CTLineCreateJustifiedLine` — a factor of 1.0 or greater is "full justification", below 1.0 "varying degrees
 * of partial justification", and 0 or less "no justification will be performed"; a `justificationWidth` under
 * the line's own width performs "negative justification (text squishing)". **AND THE MECHANISM IS OURS, SAID
 * RATHER THAN HIDDEN**: Apple publishes the contract and not where the slack goes, so this tier spreads it
 * EVENLY ACROSS THE GAPS BETWEEN CONSECUTIVE GLYPHS — the first glyph keeps the line's origin and the last
 * ends at the target width. A line of one glyph has no gap to spread across and is therefore unchanged, which
 * is the degenerate case stated rather than a special path.
 *
 * THE VERTICAL METRICS ARE THE ORIGINAL'S: neither operation moves a glyph in y, so ascent, descent and leading
 * are carried over instead of recomputed from nothing. The trailing whitespace is the NEW last run's, because
 * it is a per-run fact (`CTRun.h`).
 *
 * **AND THIS IS THE MECHANISM `CTParagraphStyle.h` REFUSED BY NAME**: `kCTJustifiedTextAlignment` is refused
 * because justification is per-glyph spacing rather than per-line placement, and the FRAME places whole lines.
 * The spacing half is here now; honouring the alignment in a frame is that unit's own work, and the paragraph
 * header says so rather than this one claiming it. */
CTLineRef CTLineCreateTruncatedLine(CTLineRef line, double width, CTLineTruncationType truncationType,
				    CTLineRef truncationToken) CG_RETURNS_RETAINED;
CTLineRef CTLineCreateJustifiedLine(CTLineRef line, CGFloat justificationFactor, double justificationWidth)
	CG_RETURNS_RETAINED;

/* **M6m: THE LINE'S HIT-TESTING AND PLACEMENT DOORS.** The indices here are BYTE offsets into the line's
 * UTF-8 text — this tier's own unit, the divergence `CTRun.h` records — not Apple's UTF-16 indices. */
double CTLineGetOffsetForStringIndex(CTLineRef line, long charIndex, CGFloat *secondaryOffset);
long CTLineGetStringIndexForPosition(CTLineRef line, CGPoint position);
double CTLineGetPenOffsetForFlush(CTLineRef line, CGFloat flushFactor, double flushWidth);
CGRect CTLineGetImageBounds(CTLineRef line, CGContextRef context);

#ifdef __cplusplus
}
#endif
#endif /* CORE_TEXT_CTLINE_H */
