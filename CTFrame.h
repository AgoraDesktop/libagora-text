/*
 * CTFrame.h — a paragraph wrapped into a shape: the lines, and where they sit.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THIS IS WHERE THE TYPESETTER'S LINE BREAKING BECOMES A PARAGRAPH.** A typesetter answers "where does
 * the line that starts at N end, given a width" (`CTTypesetter.h`); a FRAME asks that question over and
 * over until the shape is full, keeps every line it produced and where each one sits, and stops when the
 * next line would not fit. The lines it keeps ARE `CTLine` objects — the same objects `CTLineDraw` draws —
 * so a frame is a layout, not a second renderer, and there is exactly one drawing path in this tier.
 *
 * **THE ORIGINS ARE THE DESIGN'S ONE SUBTLE POINT, AND APPLE'S OWN DOCUMENTATION IS THE SPECIFICATION**:
 * `CTFrameGetLineOrigins` answers positions *relative to the frame's path*, with the first line's origin at
 * the TOP-LEFT of the shape's bounding box in CoreGraphics' own coordinate system (y up). This tier follows
 * that sentence exactly: `y` counts DOWN from the box's top as lines are added, and the x is the box's left
 * edge — so a caller that draws into the box's own coordinates needs no arithmetic at all, and one that
 * places the box somewhere translates the CTM (which is what `CTLineDraw`'s caller does too).
 *
 * **WHAT IS DELIBERATELY ABSENT**: the PATH's own generality — a frame here fills the PATH'S BOUNDING BOX
 * rather than following an arbitrary contour, which is a stated boundary rather than an unnoticed one. Text
 * that does not fill the box is still laid out; the lines that do not FIT are dropped, and
 * `CTFrameGetVisibleStringRange` is what says where the layout stopped.
 *
 * **AND TWO OF THE THREE THINGS THIS PARAGRAPH USED TO LIST ARE NO LONGER ABSENT**, which is worth saying
 * because a "deliberately absent" list is a CLAIM and two thirds of it had quietly stopped being true:
 * `CTFrameGetFrameAttributes` shipped in M6p, and `CTFrameGetTypeID` is declared with the other doors below
 * (CT-M7g, `CTTypeID_internal.h`).
 */
#ifndef CORE_TEXT_CTFRAME_H
#define CORE_TEXT_CTFRAME_H

#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreText/CTLine.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTFrame;
#else
typedef struct objc_object CTFrame;
#endif
typedef CTFrame *CTFrameRef;

/* THE LINES, IN LAYOUT ORDER, RETAINED (+1). One `CTLine` per line of the paragraph, each carrying its own
 * runs — so `CTFrameGetLines` and `CTFrameGetLineOrigins` are the whole of the layout's public shape. */
CFTypeID CTFrameGetTypeID(void);

NSArray *CTFrameGetLines(CTFrameRef frame) CG_RETURNS_RETAINED;

/* **AND WHERE EACH OF THEM SITS**, relative to the frame's path (see the file comment for the reading).
 * THE SAME EMPTY-RANGE RULE THE OTHER RANGED DOORS FOLLOW: a length of 0 means every line, and a range past
 * the end is clamped rather than refused (`CTRun.h` records the rule and the type). */
void CTFrameGetLineOrigins(CTFrameRef frame, NSRange range, CGPoint origins[]);

/* THE RANGE OF THE STRING THE FRAME WAS ASKED TO LAY OUT, and the part of it that FIT — which differ the
 * moment a shape is too small for its text, and are the same otherwise. */
NSRange CTFrameGetStringRange(CTFrameRef frame);
NSRange CTFrameGetVisibleStringRange(CTFrameRef frame);

/* DRAWING, AND IT IS THE SAME ONE CALL PER LINE THAT `CTLineDraw` IS — a frame draws its lines at their
 * origins and does nothing else. Apple's own note that "this call may leave the context in any state" is
 * observed: the text matrix and the font are whatever the last run set. */
void CTFrameDraw(CTFrameRef frame, CGContextRef context);

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S — the pair every Core Text object carries. No
 * ledger row is claimed for it. */
CTFrameRef CTFrameRetain(CTFrameRef frame);
void CTFrameRelease(CTFrameRef frame);

/* **M6p: THE FRAME'S OWN TWO DOORS** — the shape it was built in and the attributes it was built with, both
 * of which `CTFramesetterCreateFrame` received and the frame simply had not been keeping.
 *
 * **AND THEY ARE INSIDE THIS `extern "C"` BLOCK BECAUSE CT-M7d FOUND THEM OUTSIDE IT**: they were declared
 * after the block's closing brace, which hands a C++ translation unit a C++-MANGLED declaration of a function
 * the library defines with C linkage — a link error waiting for the first C++ consumer. Nothing in this header
 * is C++'s, so nothing in it belongs outside. */
CGPathRef CTFrameGetPath(CTFrameRef frame);
NSDictionary *CTFrameGetFrameAttributes(CTFrameRef frame);

/* **M7d: THE FRAME'S PROGRESSION — HOW THE LINES STACK.** Apple's sentence for the attribute: its value "must
 * be a CFNumberRef containing a CTFrameProgression", "Default is kCTFrameProgressionTopToBottom", and it
 * "determines the line stacking behavior for a frame and does not affect the appearance of the glyphs within
 * that frame".
 *
 * **THE TWO VALUES ARE NOT TWO SETTINGS.** Apple's own enum discussion says TopToBottom is "lines stacked top
 * to bottom FOR HORIZONTAL TEXT" and RightToLeft is "lines stacked right to left FOR VERTICAL TEXT", and adds
 * that a frame "created with a progression type specifying vertical text will ROTATE LINES 90 DEGREES
 * COUNTERCLOCKWISE when drawing". This tier lays out horizontal text left to right and says so in three other
 * headers (`CTLine.h`'s vertical refusal, `CTFont.h`'s orientation, `CTParagraphStyle.h`'s). So TopToBottom
 * **IS HONOURED** and RightToLeft **IS REFUSED BY NAME**: `CTFramesetterCreateFrame` answers NULL rather than
 * stacking the lines the wrong way — the same rule those three headers already state, that a wrong paragraph is
 * worse than a missing one. An attributes dictionary with no progression in it takes the default, TopToBottom.
 *
 * The VALUE of the key is ours (Apple publishes the name and its value's TYPE, never the string), which is what
 * makes it safe: it is only ever compared with the constant declared beside it. */
typedef enum {
	kCTFrameProgressionTopToBottom = 0,
	kCTFrameProgressionRightToLeft = 1
} CTFrameProgression;

extern NSString *const kCTFrameProgressionAttributeName;

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFRAME_H */
