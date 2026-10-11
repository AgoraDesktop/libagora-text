/*
 * CTFont_internal — the seam between the two halves of a Core Text font.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * NOT INSTALLED FOR A CONSUMER'S USE AND NOT INCLUDED BY ANY PUBLIC HEADER — it is named the way
 * `CGFont_internal.h` and `CGColorSpace_internal.h` are, and for the same reason: this library is
 * written in TWO LANGUAGES on purpose. The object, the metrics and the character-to-glyph step are
 * plain C (`CTFont.c`) because they are the engine's own arithmetic; the NAME doors are Objective-C
 * (`CTFont.m`) because Apple's return `CFStringRef`, this tree has no CoreFoundation, and the string
 * that IS here is `NSString` — which is the same split CoreGraphics already carries, its registry in
 * `CGFontNames.m` beside its C half.
 *
 * AND THAT SPLIT NEEDS A SEAM, because the ObjC half has to reach the `CGFontRef` the C half built.
 * The alternative was a getter per field, which is three functions and three chances for the two
 * halves to disagree about what a font is; one struct, declared once, is none of those.
 */
#ifndef CORE_TEXT_CTFONT_INTERNAL_H
#define CORE_TEXT_CTFONT_INTERNAL_H

#include <CoreText/CTFont.h>
#import <Foundation/Foundation.h>
#include <CoreGraphics/CGFont.h>

/* A `CTFont` IS A `CGFont` PLUS A POINT SIZE AND A MATRIX — the whole object, and the reason this
 * framework exists at all: a `CGFont` answers in FONT UNITS, and every caller wants points.
 *
 * **AND IT IS A REAL OBJECTIVE-C OBJECT SINCE CT-M1e-b** (`dec-35c114bf75c7c2d7`): the struct that used
 * to be here would now be A SECOND DECLARATION OF ONE LAYOUT, and the runtime's non-fragile ABI computes
 * ivar offsets at runtime, so the two are not promised to agree. The same reasoning CT-M1e-a recorded for
 * the descriptor. **THE IVARS ARE `@public`** because the metric doors and the character-to-glyph step are
 * plain C, and a C caller cannot read an Objective-C ivar; and this header is now ObjC-ONLY, because the
 * only files that include it are `.m` files and one of them declares the class.
 *
 * NO `retain` FIELD: the count is the runtime's (`NSObject`'s), which is what the make-work line
 * `f->retain = 1` used to spell by hand. */
@interface CTFont : NSObject {
	@public
	CGFontRef graphics;	/* RETAINED by the object's own lifetime; `-dealloc` releases it */
	CGFloat size;
	CGAffineTransform matrix;
}
@end

#endif /* CORE_TEXT_CTFONT_INTERNAL_H */
