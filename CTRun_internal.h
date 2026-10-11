/*
 * CTRun_internal — the run's class declaration, which the SHAPER also needs.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE SHAPER LIVES IN `CTLine.m` AND IT IS THE THING THAT FILLS A RUN IN, so the ivars have to be
 * visible in a file other than the run's own — which is the same seam the font carries
 * (`CTFont_internal.h`) and the descriptor carries (`CTFontDescriptor_internal.h`). **THE IVARS ARE
 * `@public` BECAUSE THE SHAPER AND THE ACCESSORS ARE C-SHAPED CODE**: `CTRunGetGlyphsPtr` and friends are
 * C functions (Apple's own signatures), and a C function cannot read an Objective-C ivar.
 *
 * THE ARRAYS ARE `malloc`ED AND OWNED BY THE RUN: the accessors hand out raw pointers, which is Apple's
 * contract for the `*Ptr` doors ("valid until the run is deallocated"), and a Foundation container
 * cannot give a raw pointer while also being what the shaper wrote into. `-dealloc` (in `CTRun.m`) is the
 * one place they are freed, together with the attributes dictionary.
 */
#ifndef CORE_TEXT_CTRUN_INTERNAL_H
#define CORE_TEXT_CTRUN_INTERNAL_H

#import <Foundation/Foundation.h>

#include <CoreText/CTRun.h>
#include <CoreGraphics/CGGeometry.h>

@interface CTRun : NSObject {
	@public
	CGGlyph *glyphs;		/* malloc'd, or NULL for an empty run */
	CGPoint *positions;		/* the pen position BEFORE each glyph, so pen + advance[0] is the next */
	CGSize *advances;		/* what the shaper said each glyph moves the pen by, in points */
	long *string_indices;		/* the cluster mapping, in UTF-8 byte offsets (see CTRun.h) */
	long glyph_count;
	/* THE RANGE OF THE LINE'S STRING THIS RUN CAME FROM, in the same byte units: the run's first cluster
	 * and how many bytes it consumed. For the one-run-per-line shape of CT-M2a that is the whole string,
	 * and it stays correct when a line is split into several runs (M2b) because it is computed from the
	 * clusters the shaper reported rather than assumed. */
	long string_start;
	long string_length;
	/* **THE WHITESPACE THIS RUN ENDS WITH**, in points — computed by the shaper, which is the only place
	 * that holds the run's own bytes, and READ BY THE LINE from its last run: that is what
	 * `CTLineGetTrailingWhitespaceWidth` answers, and it is a per-run fact rather than a per-line one the
	 * moment a line has several runs (a line ending in a bold space reports the BOLD space's advance). */
	double trailing_whitespace;
	id attributes;			/* the string's attributes, RETAINED */
}
@end

#endif /* CORE_TEXT_CTRUN_INTERNAL_H */
