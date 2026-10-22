/*
 * CTFramesetter.c — the framesetter: it fills a shape with lines until the shape or the text runs out.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * THE LAYOUT LOOP IS SIX LINES LONG AND EVERY DECISION IN IT IS SOMEBODY ELSE'S: the TYPESETTER says where each line
 * ends (this file asks it, in the shape of a width), the LINE says how tall the text is (this file asks it for the
 * font's metrics), and the PATH says how much room there is. What the framesetter adds is the ARITHMETIC BETWEEN
 * THEM — where the next baseline goes, and when there is no room for one — which is exactly the arithmetic Apple's
 * own sentence describes: fill "until it either runs out of text or it finds that text no longer fits".
 *
 * THE PATH IS READ AS ITS BOUNDING BOX, AND THAT IS THE ONE PLACE THIS TIER IS NOT APPLE. Apple fills the path's
 * SHAPE, reflowing a line around a notch or inside a circle; this tier fills the path's BOX, which is exact for every
 * rectangle and wrong for everything else. RECORDED RATHER THAN DISCOVERED, and with the reason: the rules for
 * filling an arbitrary outline are the text system's, not a signature's, and a rectangle is what a page of text —
 * and every caller of this door so far — actually is. The box is taken ONCE, from `CGPathGetPathBoundingBox`.
 *
 * AND THE PROGRESSION IS READ AND NOT HONOURED, which is said here rather than left to be found: Apple's
 * `CTFrameProgression` exists for VERTICAL text, where lines stack in columns, and this tier lays out horizontally
 * only — a frame attribute asking for right-to-left or left-to-right progression is KEPT (the dictionary is kept
 * whole) and changes NO ORIGIN, because a column of horizontal lines is a layout this library does not have.
 */
#include <CoreText/CTFramesetter.h>

#include <CoreText/CTFrame_internal.h>
#include <CoreText/CTLine_internal.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdbool.h>
#include <stdlib.h>

struct __CTFramesetter {
	CFRuntimeBase base;
	CFAttributedStringRef string;	/* RETAINED: needed for its LENGTH, which a zero-length range is relative to */
	CTTypesetterRef typesetter;	/* RETAINED: the engine, and what `GetTypesetter` hands back */
};

static void ct_fs_finalize(CFTypeRef cf)
{
	struct __CTFramesetter *fs = (struct __CTFramesetter *)cf;

	if (fs->string != NULL) {
		CFRelease(fs->string);
	}
	if (fs->typesetter != NULL) {
		CFRelease(fs->typesetter);
	}
}

static Boolean ct_fs_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTFramesetter *a = (const struct __CTFramesetter *)cf1;
	const struct __CTFramesetter *b = (const struct __CTFramesetter *)cf2;

	if ((a->string == NULL) != (b->string == NULL)) {
		return false;
	}
	return !(a->string != NULL && !CFEqual(a->string, b->string));
}

static CFHashCode ct_fs_hash(CFTypeRef cf)
{
	const struct __CTFramesetter *fs = (const struct __CTFramesetter *)cf;

	return (fs->string == NULL) ? 0u : CFHash(fs->string);
}

static const CFRuntimeClass ct_fs_class = {
	0,		/* version */
	"CTFramesetter",/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_fs_finalize,	/* finalize */
	ct_fs_equal,	/* equal */
	ct_fs_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTFramesetterGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_fs_class);
	}
	return type_id;
}

CTFramesetterRef CTFramesetterCreateWithAttributedString(CFAttributedStringRef string)
{
	struct __CTFramesetter *fs;

	if (string == NULL) {
		return NULL;
	}
	fs = (struct __CTFramesetter *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTFramesetterGetTypeID(),
		(CFIndex)(sizeof(struct __CTFramesetter) - sizeof(CFRuntimeBase)), NULL);
	if (fs == NULL) {
		return NULL;
	}
	fs->typesetter = CTTypesetterCreateWithAttributedString(string);
	if (fs->typesetter == NULL) {
		/* NO TYPESETTER, NO FRAMESETTER: the same refusal a line makes for a string with no font, and for the same
		 * reason — there is nothing underneath to lay out with. */
		CFRelease((CFTypeRef)fs);
		return NULL;
	}
	fs->string = (CFAttributedStringRef)CFRetain(string);
	return (CTFramesetterRef)fs;
}

CTTypesetterRef CTFramesetterGetTypesetter(CTFramesetterRef framesetter)
{
	const struct __CTFramesetter *fs = (const struct __CTFramesetter *)framesetter;

	/* BORROWED: the typesetter belongs to the framesetter, and Apple's name for this door is `Get`. */
	return (fs == NULL) ? NULL : fs->typesetter;
}

/* RESOLVE A CALLER'S RANGE AGAINST THE STRING: a zero length means the rest of it, and anything outside answers
 * false. One place, because four of the doors below take a range and they must agree about what one means. */
static bool ct_fs_resolve_range(const struct __CTFramesetter *fs, CFRange range, CFRange *out)
{
	CFIndex length = CFStringGetLength(CFAttributedStringGetString(fs->string));
	CFRange r = range;

	if (r.length == 0) {
		r.length = length - r.location;
	}
	if (r.location < 0 || r.length < 0 || r.location > length || r.location + r.length > length) {
		return false;
	}
	*out = r;
	return true;
}

CTFrameRef CTFramesetterCreateFrame(CTFramesetterRef framesetter, CFRange stringRange, CGPathRef path,
				    CFDictionaryRef frameAttributes)
{
	const struct __CTFramesetter *fs = (const struct __CTFramesetter *)framesetter;
	CFMutableArrayRef lines;
	CGRect box;
	CFRange range, visible;
	CFIndex index;
	CGFloat baseline, lineHeight, ascent = 0.0, descent = 0.0, leading = 0.0;
	CGPoint *origins = NULL;
	CTFrameRef frame;

	if (fs == NULL || path == NULL) {
		return NULL;
	}
	if (!ct_fs_resolve_range(fs, stringRange, &range)) {
		return NULL;
	}
	/* THE SHAPE, READ ONCE (see this file's head: it is the BOX here, and the box is what a page of text is). */
	box = CGPathGetPathBoundingBox(path);
	if (box.size.width <= 0.0 || box.size.height <= 0.0) {
		return NULL;
	}
	lines = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (lines == NULL) {
		return NULL;
	}

	/* THE FIRST LINE IS MADE BEFORE ANYTHING IS PLACED, because a frame's VERTICAL RULE is the TEXT's: the font's own
	 * ascent, descent and leading, read from the line that was just laid out rather than assumed here. */
	index = range.location;
	visible = CFRangeMake(range.location, 0);
	if (index >= range.location + range.length) {
		frame = CTFrameCreateWithLines(lines, NULL, 0, range, visible, path, frameAttributes);
		CFRelease(lines);
		return frame;
	}
	while (index < range.location + range.length) {
		CFIndex count = CTTypesetterSuggestLineBreak(fs->typesetter, index, (double)box.size.width);
		CTLineRef line;
		CFRange lineRange;
		CGRect lineBounds;

		if (count <= 0) {
			count = 1;	/* a break that took nothing would spin this loop, the typesetter's own case covers it */
		}
		if (index + count > range.location + range.length) {
			count = (range.location + range.length) - index;
		}
		line = CTTypesetterCreateLine(fs->typesetter, CFRangeMake(index, count));
		if (line == NULL) {
			break;
		}
		/* THE LINE'S OWN HEIGHTS, ASKED OF EVERY LINE rather than of the first one only: this tier sets a frame with
		 * ONE font, but a line is what knows its own metrics, and asking each one costs a call. */
		(void)CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		lineHeight = ascent + descent + leading;
		if (lineHeight <= 0.0) {
			/* A FONT THAT REPORTS NO HEIGHT AT ALL cannot be stacked; one line is what there is. */
			lineHeight = 1.0;
		}
		/* WHERE THIS LINE'S BASELINE GOES. The FIRST line's is one ascent below the box's TOP — Core Text's own
		 * convention, and the reason a frame's origin is its bottom-left — and each line after it is one line height
		 * further down. */
		if (CFArrayGetCount(lines) == 0) {
			baseline = box.size.height - ascent;
		} else {
			baseline -= lineHeight;
		}
		/* AND WHETHER THERE IS ROOM FOR IT: A Y TEST, BECAUSE THE WIDTH IS THE BREAKER'S BUSINESS — a line wider than
		 * the box means a word the breaker could not split, and that mode has its own answer upstream. THE TEST IS IN
		 * THE PATH'S OWN COORDINATES, so a path drawn away from the origin is measured where it actually is; the
		 * line's box is what the origin below is placed by, which is why it is built rather than compared to zero. */
		lineBounds = CGRectMake(box.origin.x, box.origin.y + baseline - descent, box.size.width, lineHeight);
		if (CGRectGetMinY(lineBounds) < box.origin.y) {
			/* NO ROOM: Apple's sentence is that filling stops when "text no longer fits", and this is that moment. */
			CFRelease(line);
			break;
		}
		/* AND THE INDEX TAKES THE LINE'S OWN RANGE OVER THE BREAKER'S PROMISE WHEN THEY DISAGREE: a frame that
		 * trusted a count the line does not have would lay out a range that is not there, and the two doors are two
		 * doors — the typesetter suggests, the line reports. */
		lineRange = CTLineGetStringRange(line);
		if (lineRange.length > 0 && lineRange.length != count) {
			count = lineRange.length;
		}
		{
			CGPoint *grown = (CGPoint *)realloc(origins,
							   (size_t)(CFArrayGetCount(lines) + 1) * sizeof(CGPoint));

			if (grown == NULL) {
				CFRelease(line);
				break;
			}
			origins = grown;
			origins[CFArrayGetCount(lines)] = CGPointMake(box.origin.x, box.origin.y + baseline);
		}
		CFArrayAppendValue(lines, line);
		CFRelease(line);
		index += count;
		visible.length = (index - range.location);
	}

	frame = CTFrameCreateWithLines(lines, origins, CFArrayGetCount(lines), range, visible, path, frameAttributes);
	free(origins);
	CFRelease(lines);
	return frame;
}

/* HOW MUCH ROOM THE RANGE NEEDS, AND HOW MUCH OF IT FITS. The same loop as above with the path taken out and a SIZE
 * put in its place: lines are cut at the constraint's width (or at their natural width when there is no constraint)
 * and stacked until the constraint's height is used or the text ends. THE ANSWER'S WIDTH IS THE WIDEST LINE, not the
 * constraint — a caller asking how much room a string needs is asking what to make the frame, not what they told this
 * door — and its height is the stack that was measured. */
CGSize CTFramesetterSuggestFrameSizeWithConstraints(CTFramesetterRef framesetter, CFRange stringRange,
						    CFDictionaryRef frameAttributes, CGSize constraints,
						    CFRange *fitRange)
{
	const struct __CTFramesetter *fs = (const struct __CTFramesetter *)framesetter;
	CFRange range;
	CFIndex index;
	CGFloat widest = 0.0, stacked = 0.0, width, lineHeight = 0.0;

	(void)frameAttributes;	/* RECORDED, not obeyed: the only attribute this tier reads is the progression, and it
				 * does not lay out columns (see this file's head). */

	if (fitRange != NULL) {
		*fitRange = CFRangeMake(0, 0);
	}
	if (fs == NULL) {
		return CGSizeMake(0.0, 0.0);
	}
	if (!ct_fs_resolve_range(fs, stringRange, &range)) {
		return CGSizeMake(0.0, 0.0);
	}
	/* NO WIDTH CONSTRAINT MEANS NO WRAPPING, which is what an enormous width does to the typesetter — Apple's word for
	 * the parameter is "optionally by constraining the space", so a zero or negative one constrains nothing. */
	width = (constraints.width > 0.0) ? constraints.width : 1.0e9;

	index = range.location;
	while (index < range.location + range.length) {
		CFIndex count = CTTypesetterSuggestLineBreak(fs->typesetter, index, (double)width);
		CTLineRef line;
		CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;
		double lineWidth;

		if (count <= 0) {
			count = 1;
		}
		if (index + count > range.location + range.length) {
			count = (range.location + range.length) - index;
		}
		line = CTTypesetterCreateLine(fs->typesetter, CFRangeMake(index, count));
		if (line == NULL) {
			break;
		}
		lineWidth = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		lineHeight = ascent + descent + leading;
		if (lineHeight <= 0.0) {
			lineHeight = 1.0;
		}
		if (constraints.height > 0.0 && stacked + lineHeight > constraints.height) {
			/* THE HEIGHT IS FULL: this line does not fit, and the range ends before it. */
			CFRelease(line);
			break;
		}
		if ((CGFloat)lineWidth > widest) {
			widest = (CGFloat)lineWidth;
		}
		stacked += lineHeight;
		index += count;
		CFRelease(line);
	}
	if (fitRange != NULL) {
		*fitRange = CFRangeMake(range.location, index - range.location);
	}
	return CGSizeMake(widest, stacked);
}
