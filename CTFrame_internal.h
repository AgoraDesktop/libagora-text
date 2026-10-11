/*
 * CTFrame_internal — the frame's class, which the FRAMESETTER fills in.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE SPLIT IS THE SAME ONE EVERY OTHER PAIR IN THIS LIBRARY USES: the class's ivars are visible to the
 * file that BUILDS the object (`CTFramesetter.m` lays the paragraph out and fills a frame in), and the
 * accessors in `CTFrame.m` read them. Apple's own split is the same shape — a typesetter creates frames —
 * and it is why the two classes are two files rather than one.
 *
 * THE ORIGINS ARE A `malloc`ED ARRAY RATHER THAN A FOUNDATION CONTAINER, for the reason the run's arrays
 * are: `CTFrameGetLineOrigins` hands out a COPY into a caller's buffer, and the array it copies from is
 * written once, in layout order, by the framesetter.
 */
#ifndef CORE_TEXT_CTFRAME_INTERNAL_H
#define CORE_TEXT_CTFRAME_INTERNAL_H

#import <Foundation/Foundation.h>

#include <CoreText/CTFrame.h>
#include <CoreGraphics/CGGeometry.h>

@interface CTFrame : NSObject {
	@public
	NSArray *lines;		/* CTLine objects, in layout order, RETAINED by the array */
	CGPoint *origins;	/* malloc'd, one per line: relative to the frame's path (see CTFrame.h) */
	long line_count;
	/* THE RANGE ASKED FOR AND THE RANGE THAT FIT — the same value unless the shape was too small, which is
	 * the case `CTFrameGetVisibleStringRange` exists to answer. */
	/* **THE SHAPE AND THE ATTRIBUTES THE FRAME WAS MADE WITH** (M6p): "the path the frame was created with"
	 * and "the frame attributes", which `CTFramesetterCreateFrame` takes as its third and fourth arguments and
	 * which `CTFrameGetPath`/`CTFrameGetFrameAttributes` have to answer. Both RETAINED. */
	CGPathRef path;
	NSDictionary *frame_attributes;
	NSRange string_range;
	NSRange visible_range;
	/* AND THE BOX THE LAYOUT USED, kept because `CTFrameDraw` places every line relative to it and a caller
	 * has no other way to know what the path became. */
	CGRect box;
}
@end

#endif /* CORE_TEXT_CTFRAME_INTERNAL_H */
