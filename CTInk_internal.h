/*
 * CTInk_internal.h — the ink box of a run of glyphs, in ONE implementation.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THIS IS THE STROKED-DRAWING BRIDGE GENERALISED**: the outline door is a callback sink, and turning a
 * glyph's outline into a rectangle is the same in every door that asks for INK — `CTLineGetImageBounds` and
 * `CTRunGetImageBounds` (and the stroked route's path builder, which unions curves instead of corners). So
 * the box lives here, `static inline`, in one place rather than twice: a second copy is a second thing to
 * get the glyph origin wrong in, which is exactly what happened once already.
 */
#ifndef CORE_TEXT_CTINK_INTERNAL_H
#define CORE_TEXT_CTINK_INTERNAL_H

#include <CoreText/CTFont.h>
/* **AND THE FONT'S OWN IVARS** (`graphics`, `size`): the outline door is addressed by the face, so this
 * header reads them exactly as `CTLine.m` does — the class-side include, not the public one. */
#include <CoreText/CTFont_internal.h>
#include <CoreGraphics/CGFont_internal.h>
/* §41: this header's PATH bridge names `CGMutablePathRef` and calls the path doors, so their header is
 * traced HERE — a header that calls a door owes its own include. */
#include <CoreGraphics/CGPath.h>

typedef struct {
	CGRect box;
	int first;
	double dx, dy;		/* THE GLYPH'S OWN ORIGIN: the outline arrives at the origin, so the offset is
				 * applied to each EVENT rather than to the accumulated box afterwards. */
} ct_ink_box;

static inline void ct_ink_include(ct_ink_box *b, double x, double y)
{
	CGPoint p = CGPointMake((CGFloat)(x + b->dx), (CGFloat)(y + b->dy));

	if (b->first) {
		b->box = CGRectMake(p.x, p.y, 0.0, 0.0);
		b->first = 0;
		return;
	}
	b->box = CGRectUnion(b->box, CGRectMake(p.x, p.y, 0.0, 0.0));
}

static inline void ct_ink_move(void *info, CGFloat x, CGFloat y)
{
	ct_ink_include((ct_ink_box *)info, (double)x, (double)y);
}

static inline void ct_ink_line(void *info, CGFloat x, CGFloat y)
{
	ct_ink_include((ct_ink_box *)info, (double)x, (double)y);
}

static inline void ct_ink_conic(void *info, CGFloat cx, CGFloat cy, CGFloat x, CGFloat y)
{
	ct_ink_box *b = (ct_ink_box *)info;

	ct_ink_include(b, (double)cx, (double)cy);
	ct_ink_include(b, (double)x, (double)y);
}

static inline void ct_ink_cubic(void *info, CGFloat c1x, CGFloat c1y, CGFloat c2x, CGFloat c2y, CGFloat x,
				CGFloat y)
{
	ct_ink_box *b = (ct_ink_box *)info;

	ct_ink_include(b, (double)c1x, (double)c1y);
	ct_ink_include(b, (double)c2x, (double)c2y);
	ct_ink_include(b, (double)x, (double)y);
}

static inline void ct_ink_close(void *info)
{
	(void)info;
}

/* **THE ONE INK FUNCTION**: the union of the given glyphs' outlines, placed at the given positions, over the
 * given range (an empty range meaning all of them — this framework's convention). A NULL font, no glyphs or
 * no outline at all answers `CGRectNull`, which is what a caller checks. */
static inline CGRect ct_ink_bounds(CTFontRef font, const CGGlyph *glyphs, const CGPoint *positions,
				   long count, NSRange range)
{
	static const cg_outline_sink sink = { ct_ink_move, ct_ink_line, ct_ink_conic, ct_ink_cubic,
					      ct_ink_close };
	ct_ink_box box;

	if (font == NULL || glyphs == NULL || positions == NULL || count <= 0) {
		return CGRectNull;
	}
	if (range.length == 0 || (long)(range.location + range.length) > count) {
		range = NSMakeRange(0, (NSUInteger)count);
	}
	box.first = 1;
	box.box = CGRectNull;
	box.dx = 0.0;
	box.dy = 0.0;
	for (long g = (long)range.location; g < (long)(range.location + range.length); g++) {
		ct_ink_box local = box;		/* the RUNNING union, plus this glyph's origin for the sink */

		local.dx = (double)positions[g].x;
		local.dy = (double)positions[g].y;
		if (cg_font_glyph_outline(font->graphics, glyphs[g], (CGFloat)font->size,
					  CGAffineTransformIdentity, &sink, &local)) {
			box = local;		/* the sink has already unioned THIS glyph in, translated */
		}
	}
	return box.first ? CGRectNull : box.box;
}



/* **ONE DEFINITION, TWO USERS** (§M6n): the stroked drawing route and `CTFontCreatePathForGlyph` both turn
 * an outline into a path, so the bridge lives here and the names are the ones the drawn route already used.
 */
/* **THE OUTLINE DOOR IS A CALLBACK SINK**, so the glyph's outline arrives as move/line/conic/cubic/close
 * events and has to be turned into the `CGPath` the context can stroke. This is that bridge: the sink's `info`
 * is the path plus the GLYPH'S OWN ORIGIN, which is why the coordinates land where the glyph belongs rather
 * than all on top of one another. */
typedef struct {
	CGMutablePathRef path;
	double x, y;
} ct_outline_ctx;

static inline void ct_sink_move(void *info, CGFloat x, CGFloat y)
{
	ct_outline_ctx *o = (ct_outline_ctx *)info;

	CGPathMoveToPoint(o->path, NULL, (CGFloat)(o->x + x), (CGFloat)(o->y + y));
}

static inline void ct_sink_line(void *info, CGFloat x, CGFloat y)
{
	ct_outline_ctx *o = (ct_outline_ctx *)info;

	CGPathAddLineToPoint(o->path, NULL, (CGFloat)(o->x + x), (CGFloat)(o->y + y));
}

static inline void ct_sink_conic(void *info, CGFloat cx, CGFloat cy, CGFloat x, CGFloat y)
{
	ct_outline_ctx *o = (ct_outline_ctx *)info;

	CGPathAddQuadCurveToPoint(o->path, NULL, (CGFloat)(o->x + cx), (CGFloat)(o->y + cy),
				  (CGFloat)(o->x + x), (CGFloat)(o->y + y));
}

static inline void ct_sink_cubic(void *info, CGFloat c1x, CGFloat c1y, CGFloat c2x, CGFloat c2y, CGFloat x,
			  CGFloat y)
{
	ct_outline_ctx *o = (ct_outline_ctx *)info;

	CGPathAddCurveToPoint(o->path, NULL, (CGFloat)(o->x + c1x), (CGFloat)(o->y + c1y),
			      (CGFloat)(o->x + c2x), (CGFloat)(o->y + c2y), (CGFloat)(o->x + x),
			      (CGFloat)(o->y + y));
}

static inline void ct_sink_close(void *info)
{
	ct_outline_ctx *o = (ct_outline_ctx *)info;

	CGPathCloseSubpath(o->path);
}

static const cg_outline_sink ct_outline_sink_impl = {
	ct_sink_move, ct_sink_line, ct_sink_conic, ct_sink_cubic, ct_sink_close
};
/* THE ONE GLYPH-OUTLINE-AS-A-PATH FUNCTION. A NULL font, a glyph the face does not carry or an empty outline
 * answers NULL, which the era's own "or NULL" allows. */
static inline CGMutablePathRef ct_outline_path(CTFontRef font, CGGlyph glyph, CGFloat size,
						CGAffineTransform matrix)
{
	CGMutablePathRef path;
	ct_outline_ctx ctx;

	if (font == NULL || size <= 0.0) {
		return NULL;
	}
	path = CGPathCreateMutable();
	if (path == NULL) {
		return NULL;
	}
	ctx.path = path;
	ctx.x = 0.0;
	ctx.y = 0.0;
	if (!cg_font_glyph_outline(font->graphics, glyph, size, matrix, &ct_outline_sink_impl, &ctx)) {
		CGPathRelease(path);
		return NULL;
	}
	return path;
}

#endif /* CORE_TEXT_CTINK_INTERNAL_H */
