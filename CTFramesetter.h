/*
 * CTFramesetter.h — a paragraph, wrapped into a shape.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A FRAMESETTER IS A PARAGRAPH'S LAYOUT LOOP, AND IT IS THE LAST PIECE OF THE STACK**: the font (M1)
 * measures, the shaper (M2) positions, the typesetter (M2b-3) says where a line ends, and the framesetter
 * asks it that question over and over until the SHAPE is full — keeping every line it made and where each
 * one sits. Nothing is drawn here: it produces a `CTFrame`, and `CTFrameDraw` draws it through
 * `CTLineDraw`, which is the same one drawing path everything else uses.
 *
 * **THE SHAPE IS A PATH BECAUSE APPLE'S IS, AND THIS TIER FILLS ITS BOUNDING BOX** — a stated boundary
 * rather than an unnoticed one: an arbitrary contour (a circle, a shape with a hole) would need the line
 * break to ask about the shape's width at every line's height, which is a unit of its own. A RECT path —
 * which is what a text cell, a label and a title all have — is exact.
 *
 * WHAT IS DELIBERATELY ABSENT, each with where it belongs: `CTFramesetterCreateWithAttributedStringAndOptions`
 * (options whose values are types this tier cannot carry yet — M6's vocabulary). **TWO THINGS THAT USED TO BE
 * ON THIS LIST ARE NOT**: `CTFramesetterSuggestFrameSizeWithConstraints` landed in CT-M7d as the MEASURING
 * half of this class — the wrap run backwards, "how big would this text need to be", and the SAME loop
 * `CTFramesetterCreateFrame` runs with the lines not kept — and `CTFramesetterGetTypeID` is declared with the
 * doors below (CT-M7g, `CTTypeID_internal.h`).
 */
#ifndef CORE_TEXT_CTFRAMESETTER_H
#define CORE_TEXT_CTFRAMESETTER_H

#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGPath.h>
#include <CoreText/CTFrame.h>
#include <CoreText/CTTypesetter.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTFramesetter;
#else
typedef struct objc_object CTFramesetter;
#endif
typedef CTFramesetter *CTFramesetterRef;

/* THE STRING THE WHOLE LAYOUT IS MADE OF — Apple's door, and the only constructor: **NO FONT, NO
 * FRAMESETTER**, for the reason every other door in this library gives (a string whose attributes name no
 * font cannot be measured, so it answers NULL rather than substituting one). */
CTFramesetterRef CTFramesetterCreateWithAttributedString(NSAttributedString *string);

/* **AND THE FRAME ITSELF: THE PARAGRAPH LAID OUT.** `stringRange` is the part of the string to lay out;
 * `path` supplies the box (see the file comment); and **`frameAttributes` IS NOW READ FOR THE ONE KEY THIS TIER
 * SPELLS** — `kCTFrameProgressionAttributeName` (M7d). Apple's dictionary has more keys than that; the ones this
 * tier cannot name are accepted and not acted on, which is a named refusal in the shape the codebase uses rather
 * than a silent one. **THE PROGRESSION IS THE ONE THAT CANNOT SIMPLY BE IGNORED**: `RightToLeft` in it means a
 * VERTICAL layout this tier does not do, so the frame answers NULL rather than stacking its lines the wrong way
 * (`CTFrame.h` carries the argument). An absent key, or `TopToBottom`, lays out.
 *
 * **THE LINES THAT DO NOT FIT ARE DROPPED, NOT COMPRESSED**: the layout stops when the next line would
 * leave the box, which is what `CTFrameGetVisibleStringRange` reports. A frame whose first line does not fit
 * still keeps that one line, because a caller asking for text in a box too small for it wants the reason in
 * the answer rather than an empty frame. */
CTFrameRef CTFramesetterCreateFrame(CTFramesetterRef framesetter, NSRange stringRange, CGPathRef path,
				    NSDictionary *frameAttributes);

/* **AND ITS MEASURING HALF (M7d): HOW BIG WOULD THIS TEXT NEED TO BE.** Apple's result is "the actual
 * dimensions for the given string range and constraints", and `fitRange` is where "the range of the string that
 * actually fit in the constrained size" comes back — a `NULL` there means the caller does not want it. A
 * `CGFLOAT_MAX` in either dimension means that dimension is unconstrained.
 *
 * **IT IS THE SAME LOOP AS THE FRAME'S, WITH THE LINES NOT KEPT** — the same breaks, the same paragraph
 * numbers, the same line heights — which is what makes the two agree BY CONSTRUCTION rather than on the easy
 * cases, and the probe checks that agreement directly: a frame built over the range this door reports must fit
 * the size it reports. **ITS WIDTH IS THE WIDEST LINE ACTUALLY USED AND NOT THE CONSTRAINT**, because "the
 * actual dimensions" is what the era asks for, and a door that echoed its input back could not be checked at
 * all. It reads `frameAttributes` for the progression exactly as the frame does, and refuses the same one. */
CGSize CTFramesetterSuggestFrameSizeWithConstraints(CTFramesetterRef framesetter, NSRange stringRange,
						    NSDictionary *frameAttributes, CGSize constraints,
						    NSRange *fitRange);

/* THE TYPESETTER A CALLER MAY WANT FROM IT (Apple's door): a NEW typesetter over the same string, +1, since a
 * `CTTypesetterRef` is an object a caller owns. */
CTTypesetterRef CTFramesetterGetTypesetter(CTFramesetterRef framesetter) CG_RETURNS_RETAINED;

CFTypeID CTFramesetterGetTypeID(void);

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S. No ledger row is claimed for it. */
CTFramesetterRef CTFramesetterRetain(CTFramesetterRef framesetter);
void CTFramesetterRelease(CTFramesetterRef framesetter);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFRAMESETTER_H */
