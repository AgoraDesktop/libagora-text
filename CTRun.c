/*
 * CTRun.c — the run: what a line is made of, and every door Apple puts on it.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * A RUN OWNS WHAT IT SAYS IT OWNS, which is the whole of this file's design and the reason the "Ptr" doors here
 * never answer NULL: the four arrays are copied in at creation and freed at the end, so a caller that retains a run
 * and drops the line still holds a run whose pointers are valid (see CTRun.h's head). What that costs is a second
 * copy of the glyph data per line, and it is paid deliberately.
 *
 * TWO DOORS EXIST IN A SHAPE THE LINE'S DOORS DO NOT HAVE, AND BOTH SHAPES ARE APPLE'S. The four array doors take a
 * RANGE and apply ONE rule to it — a length of 0 means "to the end", in Apple's own words — which is why that rule
 * lives in `ct_run_clamp` and nowhere else: eight doors share it, and eight copies of it would be eight chances to
 * disagree. And the measurement doors take a range too, because a RUN can be measured in part where a LINE is
 * measured whole.
 */
#include <CoreText/CTRun_internal.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTRun {
	CFRuntimeBase base;
	CFDictionaryRef attributes;	/* RETAINED: the dictionary the run was laid out with */
	CTFontRef font;			/* RETAINED: what the glyphs were indexed in, and what draws them */
	CFRange stringRange;		/* the characters that spawned the glyphs */
	CFIndex *stringIndices;		/* malloc'd, count entries */
	CGGlyph *glyphs;		/* malloc'd, count entries */
	CGPoint *positions;		/* malloc'd, count entries, relative to the LINE's origin */
	CGSize *advances;		/* malloc'd, count entries */
	/* **AND WHICH GLYPHS ARE BOXES, COPIED FROM THE LINE.** A box occupies its delegate's width and draws no ink; the
	 * flag is NULL when the line has none, and `CTRunDraw` skips the ones that are set. */
	bool *isBox;			/* malloc'd, count entries, or NULL */
	CFIndex count;
	/* **AND WHAT THE RUN SAYS ABOUT ITSELF**, which is `CTRunGetStatus`'s whole answer: the line hands over its own
	 * base writing direction, so a right-to-left paragraph makes a run that REPORTS `kCTRunStatusRightToLeft`. */
	CTRunStatus status;
};

static void ct_run_finalize(CFTypeRef cf)
{
	struct __CTRun *run = (struct __CTRun *)cf;

	if (run->attributes != NULL) {
		CFRelease(run->attributes);
	}
	if (run->font != NULL) {
		CFRelease(run->font);
	}
	free(run->stringIndices);
	free(run->glyphs);
	free(run->positions);
	free(run->advances);
	free(run->isBox);
}

/* TWO RUNS ARE EQUAL WHEN THEY WERE LAID OUT WITH THE SAME ATTRIBUTES, IN THE SAME FONT, OVER THE SAME STRING
 * RANGE **AND THE SAME STATUS**: the glyphs and their positions are a FUNCTION of those, so comparing them would
 * compare one fact twice — and the status is not a function of them, so it is compared. */
static Boolean ct_run_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTRun *a = (const struct __CTRun *)cf1;
	const struct __CTRun *b = (const struct __CTRun *)cf2;

	if (a->status != b->status) {
		return false;
	}
	if ((a->attributes == NULL) != (b->attributes == NULL) || (a->font == NULL) != (b->font == NULL)) {
		return false;
	}
	if (a->attributes != NULL && !CFEqual(a->attributes, b->attributes)) {
		return false;
	}
	if (a->font != NULL && !CFEqual(a->font, b->font)) {
		return false;
	}
	return a->stringRange.location == b->stringRange.location &&
	       a->stringRange.length == b->stringRange.length;
}

static CFHashCode ct_run_hash(CFTypeRef cf)
{
	const struct __CTRun *run = (const struct __CTRun *)cf;
	CFHashCode h = 7u;

	if (run->attributes != NULL) {
		h = h * 31u + CFHash(run->attributes);
	}
	if (run->font != NULL) {
		h = h * 31u + CFHash(run->font);
	}
	return h * 31u + (CFHashCode)run->stringRange.length;
}

static const CFRuntimeClass ct_run_class = {
	0,		/* version */
	"CTRun",	/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_run_finalize,	/* finalize */
	ct_run_equal,	/* equal */
	ct_run_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTRunGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_run_class);
	}
	return type_id;
}

/* =================================================================================================
 * MAKING ONE. This is the seam CTRun_internal.h exists for; nothing public reaches it.
 * =============================================================================================== */
CTRunRef CTRunCreateWithAttributesAndGlyphs(CFDictionaryRef attributes, CTFontRef font, CTRunStatus status,
					    CFRange stringRange,
					    const CFIndex *stringIndices, const CGGlyph *glyphs,
					    const CGPoint *positions, const CGSize *advances,
					    const bool *isBox, CFIndex count)
{
	struct __CTRun *run;

	if (count < 0) {
		return NULL;
	}
	run = (struct __CTRun *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTRunGetTypeID(),
		(CFIndex)(sizeof(struct __CTRun) - sizeof(CFRuntimeBase)), NULL);
	if (run == NULL) {
		return NULL;
	}
	run->attributes = (attributes == NULL) ? NULL : (CFDictionaryRef)CFRetain(attributes);
	run->font = (font == NULL) ? NULL : (CTFontRef)CFRetain(font);
	run->stringRange = stringRange;
	run->status = status;
	run->count = count;
	if (count == 0) {
		return (CTRunRef)run;
	}
	/* THE FOUR ARRAYS ARE COPIED, ALL AT ONCE AND ALL OF OR NOTHING: a run with glyphs but no positions would be a
	 * run that cannot be drawn, and answering a caller with half of its data is worse than answering NULL. */
	run->stringIndices = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	run->glyphs = (CGGlyph *)malloc((size_t)count * sizeof(CGGlyph));
	run->positions = (CGPoint *)malloc((size_t)count * sizeof(CGPoint));
	run->advances = (CGSize *)malloc((size_t)count * sizeof(CGSize));
	if (run->stringIndices == NULL || run->glyphs == NULL || run->positions == NULL || run->advances == NULL) {
		CFRelease((CFTypeRef)run);	/* the finalizer frees whichever arrays did arrive */
		return NULL;
	}
	memcpy(run->stringIndices, stringIndices, (size_t)count * sizeof(CFIndex));
	memcpy(run->glyphs, glyphs, (size_t)count * sizeof(CGGlyph));
	memcpy(run->positions, positions, (size_t)count * sizeof(CGPoint));
	memcpy(run->advances, advances, (size_t)count * sizeof(CGSize));
	if (isBox != NULL) {
		run->isBox = (bool *)malloc((size_t)count * sizeof(bool));
		if (run->isBox == NULL) {
			CFRelease((CFTypeRef)run);
			return NULL;
		}
		memcpy(run->isBox, isBox, (size_t)count * sizeof(bool));
	}
	return (CTRunRef)run;
}

/* =================================================================================================
 * APPLE'S RULE FOR A RANGE, IN ONE PLACE. "If the length of the range is set to 0, then the copy operation will
 * continue from the range's start index to the end of the run." A start outside the run yields NOTHING, which is the
 * only reading of a range that cannot be satisfied, and a negative start is moved to the beginning with its length
 * shortened by the same amount rather than refused.
 * =============================================================================================== */
static CFRange ct_run_clamp(const struct __CTRun *run, CFRange range)
{
	CFRange r = range;

	if (r.length == 0) {
		r.length = run->count - r.location;
	}
	if (r.location < 0) {
		r.length += r.location;
		r.location = 0;
	}
	if (r.length <= 0 || r.location >= run->count) {
		return CFRangeMake((CFIndex)((r.location < 0) ? 0 : r.location), 0);
	}
	if (r.location + r.length > run->count) {
		r.length = run->count - r.location;
	}
	return r;
}

/* =================================================================================================
 * WHAT A RUN IS MADE OF.
 * =============================================================================================== */
CFIndex CTRunGetGlyphCount(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? 0 : r->count;
}

CFDictionaryRef CTRunGetAttributes(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? NULL : r->attributes;
}

/* **THE RUN'S OWN ANSWER ABOUT ITSELF.** Until the bidi milestone began this was `NoStatus` unconditionally, and that
 * was the answer rather than a placeholder: nothing reordered. Now the LINE reads the paragraph's base writing
 * direction and hands it to the run, so `kCTRunStatusRightToLeft` is SET when the paragraph runs right to left — the
 * fact a caller asks this door for. REORDERING the glyphs is the next slice, so `NonMonotonic` stays clear (no string
 * index has moved) and the matrix flag stays clear (the tier draws no matrix). */
CTRunStatus CTRunGetStatus(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? kCTRunStatusNoStatus : r->status;
}

const CGGlyph *CTRunGetGlyphsPtr(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? NULL : r->glyphs;
}

void CTRunGetGlyphs(CTRunRef run, CFRange range, CGGlyph buffer[])
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CFRange r2;

	if (r == NULL || buffer == NULL) {
		return;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length > 0) {
		memcpy(buffer, r->glyphs + r2.location, (size_t)r2.length * sizeof(CGGlyph));
	}
}

const CGPoint *CTRunGetPositionsPtr(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? NULL : r->positions;
}

void CTRunGetPositions(CTRunRef run, CFRange range, CGPoint buffer[])
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CFRange r2;

	if (r == NULL || buffer == NULL) {
		return;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length > 0) {
		memcpy(buffer, r->positions + r2.location, (size_t)r2.length * sizeof(CGPoint));
	}
}

const CGSize *CTRunGetAdvancesPtr(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? NULL : r->advances;
}

void CTRunGetAdvances(CTRunRef run, CFRange range, CGSize buffer[])
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CFRange r2;

	if (r == NULL || buffer == NULL) {
		return;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length > 0) {
		memcpy(buffer, r->advances + r2.location, (size_t)r2.length * sizeof(CGSize));
	}
}

const CFIndex *CTRunGetStringIndicesPtr(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? NULL : r->stringIndices;
}

void CTRunGetStringIndices(CTRunRef run, CFRange range, CFIndex buffer[])
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CFRange r2;

	if (r == NULL || buffer == NULL) {
		return;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length > 0) {
		memcpy(buffer, r->stringIndices + r2.location, (size_t)r2.length * sizeof(CFIndex));
	}
}

CFRange CTRunGetStringRange(CTRunRef run)
{
	const struct __CTRun *r = (const struct __CTRun *)run;

	return (r == NULL) ? CFRangeMake(0, 0) : r->stringRange;
}

/* =================================================================================================
 * MEASUREMENT, OVER A RANGE.
 * =============================================================================================== */
double CTRunGetTypographicBounds(CTRunRef run, CFRange range, CGFloat *ascent, CGFloat *descent, CGFloat *leading)
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CFRange r2;
	double width = 0.0;

	if (r == NULL) {
		return 0.0;
	}
	/* THE HEIGHTS ARE THE FONT'S, as they are for the line: a run does not know which of its glyphs a framesetter
	 * will place it beside, and the font's metrics are what lines are stacked with. */
	if (ascent != NULL) {
		*ascent = (r->font == NULL) ? 0.0 : CTFontGetAscent(r->font);
	}
	if (descent != NULL) {
		*descent = (r->font == NULL) ? 0.0 : CTFontGetDescent(r->font);
	}
	if (leading != NULL) {
		*leading = (r->font == NULL) ? 0.0 : CTFontGetLeading(r->font);
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length <= 0) {
		return 0.0;
	}
	/* FROM THE FIRST GLYPH'S POSITION TO THE LAST ONE'S ADVANCE — the extent the range would cover if it were drawn
	 * where the run says its glyphs go, which is what a "typographic width" is. */
	width = (double)((r->positions[r2.location + r2.length - 1].x + r->advances[r2.location + r2.length - 1].width) -
			 r->positions[r2.location].x);
	return (width < 0.0) ? 0.0 : width;
}

CGRect CTRunGetImageBounds(CTRunRef run, CGContextRef context, CFRange range)
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CGRect *boxes;
	CGRect ink = CGRectNull;
	CGPoint pen;
	CFRange r2;
	CFIndex i;

	if (r == NULL || context == NULL || r->font == NULL) {
		return CGRectNull;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length <= 0) {
		return CGRectNull;
	}
	boxes = (CGRect *)malloc((size_t)r2.length * sizeof(CGRect));
	if (boxes == NULL) {
		return CGRectNull;
	}
	/* THE FONT'S BOXES ARE RELATIVE TO EACH GLYPH, so the run's own positions are added — and then the context's
	 * text position, which is Apple's sentence for this door: the rect "will be translated by the supplied context's
	 * text position and the positions of the individual glyphs". */
	(void)CTFontGetBoundingRectsForGlyphs(r->font, kCTFontHorizontalOrientation, r->glyphs + r2.location, boxes,
					      r2.length);
	pen = CGContextGetTextPosition(context);
	for (i = 0; i < r2.length; i++) {
		CGRect box = boxes[i];

		box.origin.x += r->positions[r2.location + i].x + pen.x;
		box.origin.y += r->positions[r2.location + i].y + pen.y;
		ink = CGRectIsNull(ink) ? box : CGRectUnion(ink, box);
	}
	free(boxes);
	return ink;
}

/* THE IDENTITY, WHICH IS THIS TIER'S ANSWER AND NOT AN OMISSION: no run this library makes carries a matrix of its
 * own — the glyphs are drawn where their positions are, under whatever transform the caller's context already has.
 * The flag that would say otherwise (`kCTRunStatusHasNonIdentityMatrix`) is clear for the same reason. */
CGAffineTransform CTRunGetTextMatrix(CTRunRef run)
{
	(void)run;
	return CGAffineTransformIdentity;
}

/* =================================================================================================
 * DRAWING, OVER A RANGE.
 * =============================================================================================== */
void CTRunDraw(CTRunRef run, CGContextRef context, CFRange range)
{
	const struct __CTRun *r = (const struct __CTRun *)run;
	CGFontRef graphics;
	CGPoint *absolute;
	CGPoint pen;
	CFRange r2;
	CFIndex i;

	if (r == NULL || context == NULL || r->font == NULL) {
		return;
	}
	r2 = ct_run_clamp(r, range);
	if (r2.length <= 0) {
		return;
	}
	graphics = CTFontCopyGraphicsFont(r->font, NULL);
	if (graphics == NULL) {
		return;
	}
	/* THE SAME PEN RULE CTLineDraw RECORDS, and the same reason: this Core Graphics' ShowGlyphsAtPositions takes
	 * positions "specified in user space" and lets "the CTM alone" place the pen, so the context's text position is
	 * added here rather than by the drawing door. */
	pen = CGContextGetTextPosition(context);
	absolute = (CGPoint *)malloc((size_t)r2.length * sizeof(CGPoint));
	if (absolute == NULL) {
		CGFontRelease(graphics);
		return;
	}
	/* **AND A BOX IS SKIPPED, AS IN `CTLineDraw`: IT OCCUPIES SPACE AND DRAWS NO INK.** The glyphs and positions are
	 * collected without the boxes, because `CGContextShowGlyphsAtPositions` takes one contiguous run. */
	{
		const CGGlyph *drawGlyphs = r->glyphs + r2.location;
		CFIndex drawCount = r2.length;
		CFIndex k = 0;

		for (i = 0; i < r2.length; i++) {
			if (r->isBox != NULL && r->isBox[r2.location + i]) {
				CGGlyph *filteredGlyphs = (CGGlyph *)malloc((size_t)r2.length * sizeof(CGGlyph));
				CGPoint *filteredPositions = (CGPoint *)malloc((size_t)r2.length * sizeof(CGPoint));

				if (filteredGlyphs == NULL || filteredPositions == NULL) {
					free(filteredGlyphs);
					free(filteredPositions);
					free(absolute);
					CGFontRelease(graphics);
					return;
				}
				for (i = 0; i < r2.length; i++) {
					if (!(r->isBox != NULL && r->isBox[r2.location + i])) {
						filteredGlyphs[k] = r->glyphs[r2.location + i];
						filteredPositions[k] = CGPointMake(r->positions[r2.location + i].x + pen.x,
										   r->positions[r2.location + i].y + pen.y);
						k++;
					}
				}
				CGContextSetFont(context, graphics);
				CGContextSetFontSize(context, CTFontGetSize(r->font));
				CGContextShowGlyphsAtPositions(context, filteredGlyphs, filteredPositions, (size_t)k);
				free(filteredGlyphs);
				free(filteredPositions);
				free(absolute);
				CGFontRelease(graphics);
				return;
			}
		}
		for (i = 0; i < r2.length; i++) {
			absolute[i] = CGPointMake(r->positions[r2.location + i].x + pen.x,
						  r->positions[r2.location + i].y + pen.y);
		}
		CGContextSetFont(context, graphics);
		CGContextSetFontSize(context, CTFontGetSize(r->font));
		CGContextShowGlyphsAtPositions(context, drawGlyphs, absolute, (size_t)drawCount);
	}
	free(absolute);
	CGFontRelease(graphics);
}
