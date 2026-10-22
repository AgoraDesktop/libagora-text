/*
 * CTFrame.h — the frame: the lines that fit inside a path.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). Its includes are Core Foundation's containers and strings, a
 * `CGContext` for drawing and — the one nothing else in this framework includes — a `CGPath`, because a frame is
 * defined BY the shape it fills.
 *
 * A FRAME HAS NO PUBLIC CREATOR, LIKE A RUN, AND FOR THE SAME REASON: Apple's sentence for the door that makes one is
 * that the framesetter "will continue to fill the frame until it either runs out of TEXT or it finds that text no
 * longer FITS" — which is a decision only a framesetter can make. So a caller reaches a frame through
 * `CTFramesetterCreateFrame`, and the seam between that door and this type is a header of its own
 * (`CTFrame_internal.h`) rather than a public creator Apple does not have.
 *
 * A FRAME IS WHERE THE WHOLE LAYER MEETS: a typesetter decides where each line ends, a line turns characters into
 * glyphs, and a frame PLACES the resulting lines one under another until the path is full — then reports which part
 * of the string it managed to show, which is why `CTFrameGetVisibleStringRange` exists beside `CTFrameGetStringRange`
 * and is not the same door.
 *
 * THE PATH IS READ AS ITS BOUNDING BOX HERE, WHICH IS THE ONE THING THIS TIER DOES NOT DO AS APPLE DOES, and it is
 * recorded where it is implemented rather than discovered: a frame fills the path's BOX, so a text frame shaped like
 * a circle — or like a second column beside an image's notch — is not reflowed around the shape. Every rectangle is
 * exact, which is what a page of text is; text flowing around an arbitrary outline waits for the tier that owns the
 * path-filling rules.
 */
#ifndef CORE_TEXT_CTFRAME_H
#define CORE_TEXT_CTFRAME_H

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGPath.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE FRAME. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTFrameGetTypeID()`. */
typedef const struct __CTFrame *CTFrameRef;

CFTypeID CTFrameGetTypeID(void);

/* WHICH WAY THE LINES MARCH. Apple's own note is that this is for VERTICAL text — a progression other than
 * top-to-bottom stacks lines in columns rather than rows — AND THIS TIER LAYS OUT HORIZONTALLY ONLY, so the two
 * other members are named here because the vocabulary is Apple's and RECORDED as not honoured in `CTFramesetter.c`
 * rather than quietly ignored. */
enum {
	kCTFrameProgressionTopToBottom = 0,
	kCTFrameProgressionRightToLeft = 1,
	kCTFrameProgressionLeftToRight = 2
};
typedef uint32_t CTFrameProgression;

/* AND THE FRAME ATTRIBUTE THAT NAMES IT, whose value Apple does not publish — an inference here, as the framework's
 * other unpublished keys are. The dictionary a caller hands `CTFramesetterCreateFrame` is kept, and this key is the
 * one member of it this tier reads. */
extern const CFStringRef kCTFrameProgressionAttributeName;

/* ---------------------------------------------------------------------------------------
 * WHAT THE FRAME CAME OUT AS. Two ranges, and the pair is the point: a frame asked for a whole document may have
 * shown one page, and only the SECOND door says how much.
 * ------------------------------------------------------------------------------------- */
/* THE RANGE THE FRAME WAS ASKED FOR, as it was given. */
CFRange CTFrameGetStringRange(CTFrameRef frame);

/* AND THE PART OF IT THAT FIT, which is a PREFIX of the range above — Apple's own word for the door is "visible".
 * It is the length of a prefix rather than a number of lines because a caller's next question is "where does the
 * next frame start", and the answer is this range's end. */
CFRange CTFrameGetVisibleStringRange(CTFrameRef frame);

/* THE SHAPE, RETAINED: what the frame was asked to fill, handed back rather than remembered by the caller. */
CGPathRef CTFrameGetPath(CTFrameRef frame);

/* AND THE ATTRIBUTES the frame was created with, or NULL when it was given none. NOT the string's attributes: this
 * is the dictionary `CTFramesetterCreateFrame` took as its own last parameter, which is where a progression and a
 * column count would live. */
CFDictionaryRef CTFrameGetFrameAttributes(CTFrameRef frame);

/* THE LINES, IN THE ORDER THEY ARE DRAWN, as a BORROWED array owned by the frame — Apple's name for this door is
 * `Get`, as it is for a line's runs. */
CFArrayRef CTFrameGetLines(CTFrameRef frame);

/* WHERE EACH LINE SITS, in the PATH'S coordinate space, which is the one place a caller needs coordinates rather than
 * a line: a frame's origin is the path's own, and its first line starts a line-height BELOW the top. APPLE'S RANGE
 * RULE APPLIES — a length of 0 continues to the end of the lines — and a range outside them writes nothing. */
void CTFrameGetLineOrigins(CTFrameRef frame, CFRange range, CGPoint origins[]);

/* DRAW EVERY LINE. Apple's note matches the line's own: this "may leave the graphics context in any state and does
 * not flush". Each line is drawn AT ITS ORIGIN, which is what makes the origins above a real API rather than a
 * convenience. */
void CTFrameDraw(CTFrameRef frame, CGContextRef context);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFRAME_H */
