/*
 * CTRun.m — the run class, and the memory it owns.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * A RUN OWNS FOUR ARRAYS AND A DICTIONARY, and `-dealloc` is the one place they go back. The arrays are
 * `malloc`ed rather than held in Foundation containers because the accessors hand out RAW POINTERS
 * (Apple's own contract for the `*Ptr` doors: "valid until the run is deallocated"), and a raw pointer
 * is exactly what an `NSData` or an `NSValue` cannot give while also being the thing the shaper wrote.
 *
 * THE CLASS IS CREATED BY THE LINE'S SHAPER (`CTLine.m`) and is **not** constructible from here — no
 * header of this file declares an initialiser, because Apple's header declares a create DOOR for a line
 * and none for a run.
 */
#import <Foundation/Foundation.h>

#include "CTRun.h"
/* THE IVARS LIVE IN THE SEAM, because THE SHAPER — which is `CTLine.m` — is what fills a run in. */
#include "CTRun_internal.h"
#include "CTInk_internal.h"
/* AND THE STRING ATTRIBUTE KEYS, which is where `kCTFontAttributeName` lives — the font a run was
 * shaped with is read back out of its own attributes. */
#include <CoreText/CTStringAttributes.h>
#include <CoreText/CTFont_internal.h>

#include <stdlib.h>
#include <string.h>

@implementation CTRun

- (void)dealloc
{
	free(glyphs);
	free(positions);
	free(advances);
	free(string_indices);
	[attributes release];
	[super dealloc];
}

@end

/* THE ACCESSORS ARE C FUNCTIONS BECAUSE APPLE'S ARE, and they read the run's own ivars through
 * `@public` — the same seam the font's metric doors use, and for the same reason. */

long CTRunGetGlyphCount(CTRunRef run)
{
	return (run != NULL) ? (long)run->glyph_count : 0;
}

const CGGlyph *CTRunGetGlyphsPtr(CTRunRef run)
{
	return (run != NULL) ? run->glyphs : NULL;
}

const CGPoint *CTRunGetPositionsPtr(CTRunRef run)
{
	return (run != NULL) ? run->positions : NULL;
}

const CGSize *CTRunGetAdvancesPtr(CTRunRef run)
{
	return (run != NULL) ? run->advances : NULL;
}

const long *CTRunGetStringIndicesPtr(CTRunRef run)
{
	return (run != NULL) ? run->string_indices : NULL;
}

NSDictionary *CTRunGetAttributes(CTRunRef run)
{
	if (run == NULL) {
		return nil;
	}
	return [(NSDictionary *)run->attributes retain];
}

/* -------------------------------------------------------------------------------------------------
 * THE RANGED FORMS — the doors the range type was decided for (`dec-5245eae215922f08`).
 *
 * TWO RULES, ONE FROM APPLE AND ONE OURS, AND BOTH ARE IN THE CLAMP BELOW RATHER THAN REPEATED FOUR TIMES:
 * **a `length` of 0 means THE WHOLE RUN** (Apple's documented empty-range rule, and the one thing about
 * these doors a caller cannot guess), and **a range that would run past the end is CLAMPED rather than
 * refused**, because Apple documents the range as the caller's assertion about a run the caller already
 * holds. A `location` beyond the run is empty, which is the honest answer to "the characters after this
 * run" rather than a refusal.
 * =============================================================================================== */
static NSRange ct_run_clamp(CTRunRef run, NSRange range)
{
	NSRange whole;

	if (run == NULL) {
		return NSMakeRange(0, 0);
	}
	whole = NSMakeRange(0, (NSUInteger)run->glyph_count);
	if (range.length == 0) {
		return whole;
	}
	if (range.location >= whole.length) {
		return NSMakeRange(0, 0);
	}
	if (range.location + range.length > whole.length) {
		range.length = whole.length - range.location;
	}
	return range;
}

void CTRunGetGlyphs(CTRunRef run, NSRange range, CGGlyph buffer[])
{
	NSRange r = ct_run_clamp(run, range);
	NSUInteger i;

	if (buffer == NULL) {
		return;
	}
	for (i = 0; i < r.length; i++) {
		buffer[i] = run->glyphs[r.location + i];
	}
}

void CTRunGetPositions(CTRunRef run, NSRange range, CGPoint buffer[])
{
	NSRange r = ct_run_clamp(run, range);
	NSUInteger i;

	if (buffer == NULL) {
		return;
	}
	for (i = 0; i < r.length; i++) {
		buffer[i] = run->positions[r.location + i];
	}
}

void CTRunGetAdvances(CTRunRef run, NSRange range, CGSize buffer[])
{
	NSRange r = ct_run_clamp(run, range);
	NSUInteger i;

	if (buffer == NULL) {
		return;
	}
	for (i = 0; i < r.length; i++) {
		buffer[i] = run->advances[r.location + i];
	}
}

void CTRunGetStringIndices(CTRunRef run, NSRange range, long buffer[])
{
	NSRange r = ct_run_clamp(run, range);
	NSUInteger i;

	if (buffer == NULL) {
		return;
	}
	for (i = 0; i < r.length; i++) {
		buffer[i] = run->string_indices[r.location + i];
	}
}

NSRange CTRunGetStringRange(CTRunRef run)
{
	if (run == NULL) {
		return NSMakeRange(0, 0);
	}
	return NSMakeRange((NSUInteger)run->string_start, (NSUInteger)run->string_length);
}

/* ====================================================================================================
 * M6n: THE RUN'S STATUS, MATRIX AND METRICS.
 * ================================================================================================== */

/* **THE STATUS IS DERIVED, AND EACH BIT IS ONE OF THE ERA'S OWN**: `HasNonIdentityMatrix` is true exactly
 * when the run's font carries a matrix, `RightToLeft` and `NonMonotonic` are answered zero because this
 * tier's layout is left-to-right and its glyph order monotonic — and the framework REFUSES a base writing
 * direction by name (CTParagraphStyle.h), so a zero here is a fact about this tier rather than an
 * unconsidered default. */
CTRunStatus CTRunGetStatus(CTRunRef run)
{
	CTFontRef font;

	if (run == NULL) {
		return kCTRunStatusNoStatus;
	}
	font = (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName];
	if (font == NULL) {
		return kCTRunStatusNoStatus;
	}
	if (font->matrix.a != 1.0 || font->matrix.b != 0.0 || font->matrix.c != 0.0 ||
	    font->matrix.d != 1.0 || font->matrix.tx != 0.0 || font->matrix.ty != 0.0) {
		return kCTRunStatusHasNonIdentityMatrix;
	}
	return kCTRunStatusNoStatus;
}

CGAffineTransform CTRunGetTextMatrix(CTRunRef run)
{
	CTFontRef font;

	if (run == NULL) {
		return CGAffineTransformIdentity;
	}
	font = (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName];
	return (font == NULL) ? CGAffineTransformIdentity : font->matrix;
}

/* **THE RUN'S METRICS ARE ITS FONT'S**, and that is stated rather than implied: this tier's runs carry no
 * per-run ascent and descent — the LINE takes the maxima over its runs — so the honest answer for a run is
 * the metrics of the face it was shaped with, which is the same face whose advances it reports. */
double CTRunGetTypographicBounds(CTRunRef run, NSRange range, CGFloat *ascent, CGFloat *descent,
				 CGFloat *leading)
{
	CTFontRef font;
	double width = 0.0;
	long count, from, to;

	if (run == NULL) {
		return 0.0;
	}
	font = (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName];
	if (ascent != NULL) {
		*ascent = (font == NULL) ? 0.0 : CTFontGetAscent(font);
	}
	if (descent != NULL) {
		*descent = (font == NULL) ? 0.0 : CTFontGetDescent(font);
	}
	if (leading != NULL) {
		*leading = (font == NULL) ? 0.0 : CTFontGetLeading(font);
	}
	count = run->glyph_count;
	if (range.length == 0 || (long)(range.location + range.length) > count) {
		range = NSMakeRange(0, (NSUInteger)count);
	}
	from = (long)range.location;
	to = from + (long)range.length;
	for (long g = from; g < to; g++) {
		width += (double)run->advances[g].width;
	}
	return width;
}

CGRect CTRunGetImageBounds(CTRunRef run, CGContextRef context, NSRange range)
{
	CTFontRef font;

	(void)context;		/* the LINE's space, as the header states */
	if (run == NULL) {
		return CGRectNull;
	}
	font = (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName];
	return ct_ink_bounds(font, run->glyphs, run->positions, run->glyph_count, range);
}
