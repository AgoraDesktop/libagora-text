/*
 * CTFrame.m — the frame's accessors, and the one drawing call every text system needs.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE FRAME OWNS TWO THINGS — an array of `CTLine` objects and a `malloc`ed array of their origins — and
 * `-dealloc` is the one place they go back. Everything else here is a copy out of those two arrays, which is
 * why this file is short: **the layout is the framesetter's, and the drawing is the LINE's.**
 */
#import <Foundation/Foundation.h>

#include "CTFrame.h"
#include "CTFrame_internal.h"
#include <CoreGraphics/CGPath.h>   /* for CGPathRef, which this file now names */
#include <CoreText/CTLine.h>
#include <CoreGraphics/CGContext.h>

#include <stdlib.h>

@implementation CTFrame

- (void)dealloc
{
	/* **AND THE TWO THINGS M6p ADDED**, which the existing dealloc could not know about: the shape and the
	 * attributes the frame was made with are the frame's own copies. */
	if (path != NULL) {		/* THE IVARS ARE BARE HERE: a method's receiver is `self`, and the file
					 * next door to this one is where `->` names a frame. */
		CGPathRelease(path);
	}
	[frame_attributes release];
	free(origins);
	[lines release];
	[super dealloc];
}

@end

NSArray *CTFrameGetLines(CTFrameRef frame)
{
	if (frame == NULL) {
		return nil;
	}
	return [(NSArray *)frame->lines retain];
}

void CTFrameGetLineOrigins(CTFrameRef frame, NSRange range, CGPoint origins[])
{
	NSRange r;
	NSUInteger i;
	long n;

	if (frame == NULL || origins == NULL) {
		return;
	}
	n = frame->line_count;
	/* **THE SAME EMPTY-RANGE RULE THE OTHER RANGED DOORS FOLLOW** (a length of 0 means every line, and a
	 * range past the end is clamped): `CTRun.h` records the rule and where the range type comes from, and a
	 * layout door that disagreed with the glyph doors about it would be the one place they diverged. */
	r = (range.length == 0) ? NSMakeRange(0, (NSUInteger)n) : range;
	if (n <= 0 || r.location >= (NSUInteger)n || r.length == 0) {
		return;
	}
	if (r.location + r.length > (NSUInteger)n) {
		r.length = (NSUInteger)n - r.location;
	}
	for (i = 0; i < r.length; i++) {
		origins[i] = frame->origins[r.location + i];
	}
}

NSRange CTFrameGetStringRange(CTFrameRef frame)
{
	return (frame != NULL) ? frame->string_range : NSMakeRange(0, 0);
}

NSRange CTFrameGetVisibleStringRange(CTFrameRef frame)
{
	return (frame != NULL) ? frame->visible_range : NSMakeRange(0, 0);
}

void CTFrameDraw(CTFrameRef frame, CGContextRef context)
{
	NSUInteger i;

	if (frame == NULL || context == NULL) {
		return;
	}
	for (i = 0; i < (NSUInteger)frame->line_count; i++) {
		/* **EACH LINE IS DRAWN BY TRANSLATING THE CTM TO ITS ORIGIN AND CALLING `CTLineDraw`** — the caller
		 * pattern `CTLineDraw` documents, done once per line and then UNDONE, so the context comes back as
		 * the caller had it (apart from the text state the runs set, which `CTFrame.h` names). There is no
		 * second drawing path in this tier, which is the point. */
		CGContextSaveGState(context);
		CGContextTranslateCTM(context, frame->origins[i].x, frame->origins[i].y);
		CTLineDraw((CTLineRef)[frame->lines objectAtIndex:i], context);
		CGContextRestoreGState(context);
	}
}

CTFrameRef CTFrameRetain(CTFrameRef frame)
{
	return (CTFrameRef)[(id)(uintptr_t)frame retain];
}

void CTFrameRelease(CTFrameRef frame)
{
	[(id)(uintptr_t)frame release];
}

CGPathRef CTFrameGetPath(CTFrameRef frame)
{
	return (frame == NULL) ? NULL : frame->path;
}

NSDictionary *CTFrameGetFrameAttributes(CTFrameRef frame)
{
	return (frame == NULL) ? nil : frame->frame_attributes;
}

/* **M7d: THE PROGRESSION KEY.** Apple declares it in `CTFrame.h` (its own home, and where it is declared here),
 * and the value is ours — Apple publishes the attribute's NAME and the TYPE of its value, never the string,
 * which is what makes an unpublished value safe: it is only ever compared with the constant beside it. */
NSString *const kCTFrameProgressionAttributeName = @"CTFrameProgression";
