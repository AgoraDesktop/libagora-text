/*
 * CTLine_internal.h — the seam between a line and the things that make one out of part of a string.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER. Apple has ONE public door that makes a
 * line from an attributed string — `CTLineCreateWithAttributedString`, which takes the WHOLE string — and a
 * TYPESETTER makes lines from PARTS of one. That second way in has no public spelling, because in Apple's
 * implementation the typesetter owns the runs and the line is a view of them; here the two are separate objects
 * that must agree, so they agree in a header instead of one file `extern`ing what the other defines.
 *
 * THE SECOND DECLARATION IS THE ATTRIBUTE KEY, and IT IS NO LONGER WHERE THE KEY'S VALUE LIVES: `CTStringAttributes.h`
 * has landed, so `CTLine.c` reads `kCTFontAttributeName` from it rather than carrying `"NSFontAttributeName"` as a
 * recorded boundary. What remains here is the seam's own door — a typesetter asks a LINE what key names a font,
 * instead of naming it itself — and this header includes the one it now needs, because a declaration that returns a
 * value is a declaration that depends on where that value is declared.
 */
#ifndef CORE_TEXT_CTLINE_INTERNAL_H
#define CORE_TEXT_CTLINE_INTERNAL_H

#include <stdbool.h>

#include <CoreText/CTLine.h>
#include <CoreText/CTStringAttributes.h>

/* AND THE FONT, WHICH ONE OF THE DECLARATIONS BELOW TAKES AS AN ARGUMENT AND WHICH `CTLine.h` DOES NOT NAME — Apple's
 * own CTLine.h includes no font header either, so a caller of the seam has to bring the type with it. An INTERNAL
 * header may do that; a public one must not. */
#include <CoreText/CTFont.h>

/* A LINE OVER A RANGE OF AN ATTRIBUTED STRING, which is what a typesetter makes. `range` is IN THE STRING'S OWN
 * COORDINATES and is what the line reports as its string range; the glyphs are that range's, positioned from the
 * line's OWN origin, exactly as the whole-string door does it. A range that does not lie inside the string answers
 * NULL. */
CTLineRef CTLineCreateWithAttributedStringAndRange(CFAttributedStringRef string, CFRange range);

/* **TWO BIDI DIRECTIVES, WHICH ARE THE ONE THING A TYPESETTER'S OPTIONS ARE ABOUT, AND THEY BELONG HERE BECAUSE THE
 * ALGORITHM IS THE LINE'S.** `kCTTypesetterOptionDisableBidiProcessing` and
 * `kCTTypesetterOptionForcedEmbeddingLevel` are recorded on the typesetter and honoured by the LINE — the typesetter
 * consults neither while breaking, because bidi does not change a single advance — so the facts have to cross this
 * seam, and a struct crosses it more honestly than three positional arguments that a reader would have to count. */
typedef struct {
	int forcedLevel;	/* -1 = NOT FORCED; 0 or 1 is the paragraph level P3 is not allowed to choose */
	bool disabled;		/* the algorithm does not run at all: no levels, no reordering, no mirroring */
} CTLineBidiDirective;

/* THE NEUTRAL DIRECTIVE, which is what every caller that has no options passes: P2/P3 decide the paragraph level, and
 * the whole algorithm runs. Named rather than written as `{-1, false}` at each call site, so "no directive" has ONE
 * spelling and a later field cannot be added without finding every place that means "nothing". */
extern const CTLineBidiDirective CTLineBidiDirectiveNone;

/* THE SAME DOOR AS ABOVE WITH THE DIRECTIVES GIVEN, for the one caller that has them. */
CTLineRef CTLineCreateWithAttributedStringAndRangeWithBidi(CFAttributedStringRef string, CFRange range,
							  CTLineBidiDirective bidi);

/* THE FONT ATTRIBUTE'S KEY, AS THIS LIBRARY DECLARES IT — `kCTFontAttributeName`, whose value is PROVABLE now that
 * `CTStringAttributes.h` exists (the AppKit declares `NSFontAttributeName`, and a dictionary crosses). A constant,
 * borrowed, never released. */
CFStringRef CTLineFontAttributeName(void);

/* A LINE OVER GLYPHS THAT ARE NOT A STRING RANGE, which is what a TRUNCATED line is: `CTLineCreateTruncatedLine` keeps
 * some of a line's glyphs and adds a token's, and for a MIDDLE truncation that is a prefix AND a suffix — so the result
 * cannot be made from any one range of the string. The glyphs are placed from the line's own origin by their advances
 * in the given font, and `range` is whatever the caller says the result covers. */
CTLineRef CTLineCreateWithGlyphs(CFAttributedStringRef string, CFRange range, CTFontRef font, const CGGlyph *glyphs,
				 CFIndex count);

#endif /* CORE_TEXT_CTLINE_INTERNAL_H */
