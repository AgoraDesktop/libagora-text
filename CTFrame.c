/*
 * CTFrame.c — the frame: the lines it holds, where they sit, and drawing them.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * THIS FILE OWNS NOTHING BUT WHAT IT WAS HANDED, which is what makes it short: the lines come from the framesetter
 * (see CTFrame_internal.h), the origins come with them, and the path and the frame attributes are retained as they
 * arrived. So everything here is an accessor, plus the two doors that need the origins: `CTFrameGetLineOrigins` and
 * `CTFrameDraw`, which are the same array read twice.
 *
 * THE ORIGINS ARE IN THE PATH'S COORDINATE SPACE and are the reason this type exists as an object at all — a frame
 * is where a caller stops asking "how wide is this" and starts asking "where do I draw it".
 */
#include <CoreText/CTFrame_internal.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdlib.h>
#include <string.h>

/* THE ONE FRAME ATTRIBUTE THIS TIER READS, whose value Apple does not publish — an inference, like the framework's
 * other unpublished keys, taking the constant's own name with the leading `k` dropped. */
const CFStringRef kCTFrameProgressionAttributeName = CFSTR("CTFrameProgressionAttributeName");

struct __CTFrame {
	CFRuntimeBase base;
	CFArrayRef lines;		/* RETAINED: the lines, in the order they are drawn */
	CGPoint *origins;		/* malloc'd, lineCount entries, in the PATH's coordinates */
	CFIndex lineCount;
	CFRange stringRange;		/* what the frame was asked for */
	CFRange visibleRange;		/* and the prefix of it that fit */
	CGPathRef path;			/* RETAINED: the shape it was asked to fill */
	CFDictionaryRef attributes;	/* RETAINED: the frame attributes, or NULL */
};

static void ct_frame_finalize(CFTypeRef cf)
{
	struct __CTFrame *frame = (struct __CTFrame *)cf;

	if (frame->lines != NULL) {
		CFRelease(frame->lines);
	}
	if (frame->path != NULL) {
		CGPathRelease(frame->path);
	}
	if (frame->attributes != NULL) {
		CFRelease(frame->attributes);
	}
	free(frame->origins);
}

/* TWO FRAMES ARE EQUAL WHEN THEY COVER THE SAME PART OF THE SAME SHAPE WITH THE SAME LINES — the origins and the
 * visible range FOLLOW from those, so comparing them would compare one fact twice. */
static Boolean ct_frame_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTFrame *a = (const struct __CTFrame *)cf1;
	const struct __CTFrame *b = (const struct __CTFrame *)cf2;

	if (a->stringRange.location != b->stringRange.location ||
	    a->stringRange.length != b->stringRange.length || a->lineCount != b->lineCount) {
		return false;
	}
	if ((a->path == NULL) != (b->path == NULL)) {
		return false;
	}
	return !(a->path != NULL && !CGPathEqualToPath(a->path, b->path));
}

static CFHashCode ct_frame_hash(CFTypeRef cf)
{
	const struct __CTFrame *frame = (const struct __CTFrame *)cf;

	return (CFHashCode)((uintptr_t)frame->path) * 31u + (CFHashCode)frame->stringRange.length;
}

static const CFRuntimeClass ct_frame_class = {
	0,		/* version */
	"CTFrame",	/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_frame_finalize,	/* finalize */
	ct_frame_equal,	/* equal */
	ct_frame_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTFrameGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_frame_class);
	}
	return type_id;
}

/* THE SEAM `CTFrame_internal.h` DECLARES, and the only way a frame is ever made. The LINES AND THE ORIGINS ARE BOTH
 * COPIED — the array and the points — because the framesetter that built them is finished with them the moment this
 * returns, and a frame that borrowed them would be a frame whose answer changed when somebody else's stack moved. */
CTFrameRef CTFrameCreateWithLines(CFArrayRef lines, const CGPoint *origins, CFIndex lineCount, CFRange stringRange,
				  CFRange visibleRange, CGPathRef path, CFDictionaryRef frameAttributes)
{
	struct __CTFrame *frame;

	if (lineCount < 0) {
		return NULL;
	}
	frame = (struct __CTFrame *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTFrameGetTypeID(),
		(CFIndex)(sizeof(struct __CTFrame) - sizeof(CFRuntimeBase)), NULL);
	if (frame == NULL) {
		return NULL;
	}
	frame->lines = (lines == NULL) ? NULL : (CFArrayRef)CFRetain(lines);
	frame->lineCount = lineCount;
	frame->stringRange = stringRange;
	frame->visibleRange = visibleRange;
	frame->path = (path == NULL) ? NULL : CGPathRetain(path);
	frame->attributes = (frameAttributes == NULL) ? NULL : (CFDictionaryRef)CFRetain(frameAttributes);
	if (lineCount > 0) {
		frame->origins = (CGPoint *)malloc((size_t)lineCount * sizeof(CGPoint));
		if (frame->origins == NULL) {
			CFRelease((CFTypeRef)frame);
			return NULL;
		}
		memcpy(frame->origins, origins, (size_t)lineCount * sizeof(CGPoint));
	}
	return (CTFrameRef)frame;
}

CFRange CTFrameGetStringRange(CTFrameRef frame)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;

	return (f == NULL) ? CFRangeMake(0, 0) : f->stringRange;
}

CFRange CTFrameGetVisibleStringRange(CTFrameRef frame)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;

	return (f == NULL) ? CFRangeMake(0, 0) : f->visibleRange;
}

CGPathRef CTFrameGetPath(CTFrameRef frame)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;

	return (f == NULL) ? NULL : f->path;
}

CFDictionaryRef CTFrameGetFrameAttributes(CTFrameRef frame)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;

	return (f == NULL) ? NULL : f->attributes;
}

CFArrayRef CTFrameGetLines(CTFrameRef frame)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;

	return (f == NULL) ? NULL : f->lines;
}

/* APPLE'S RANGE RULE, WHICH IS THE FRAMEWORK'S OWN AND NOT A FRAME'S INVENTION: "If the length of the range is set to
 * 0, then the copy operation will continue from the range's start index to the end." (The same sentence is on the
 * run's array doors, in Apple's own words.) A range that reaches no line writes NOTHING rather than zeroes, so a
 * caller can pass a buffer it has already filled. */
void CTFrameGetLineOrigins(CTFrameRef frame, CFRange range, CGPoint origins[])
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;
	CFRange r = range;

	if (f == NULL || origins == NULL) {
		return;
	}
	if (r.length == 0) {
		r.length = f->lineCount - r.location;
	}
	if (r.location < 0) {
		r.length += r.location;
		r.location = 0;
	}
	if (r.length <= 0 || r.location >= f->lineCount) {
		return;
	}
	if (r.location + r.length > f->lineCount) {
		r.length = f->lineCount - r.location;
	}
	memcpy(origins, f->origins + r.location, (size_t)r.length * sizeof(CGPoint));
}

/* AND DRAWING IS THE ORIGINS USED: each line is drawn with the context's text position set to where the frame says
 * it goes, which works because `CTLineDraw` places the line's origin AT the context's text position — the semantic
 * `CTLine.c` records and its own case measures in pixels. */
void CTFrameDraw(CTFrameRef frame, CGContextRef context)
{
	const struct __CTFrame *f = (const struct __CTFrame *)frame;
	CFIndex i, count;

	if (f == NULL || context == NULL || f->lines == NULL) {
		return;
	}
	count = CFArrayGetCount(f->lines);
	for (i = 0; i < count; i++) {
		CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(f->lines, i);

		CGContextSetTextPosition(context, f->origins[i].x, f->origins[i].y);
		CTLineDraw(line, context);
	}
}
