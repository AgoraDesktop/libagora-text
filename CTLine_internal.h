/*
 * CTLine_internal.h — the seam between a line and the things that make one out of part of a string.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
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

#include <CoreText/CTLine.h>
#include <CoreText/CTStringAttributes.h>

/* A LINE OVER A RANGE OF AN ATTRIBUTED STRING, which is what a typesetter makes. `range` is IN THE STRING'S OWN
 * COORDINATES and is what the line reports as its string range; the glyphs are that range's, positioned from the
 * line's OWN origin, exactly as the whole-string door does it. A range that does not lie inside the string answers
 * NULL. */
CTLineRef CTLineCreateWithAttributedStringAndRange(CFAttributedStringRef string, CFRange range);

/* THE FONT ATTRIBUTE'S KEY, AS THIS LIBRARY DECLARES IT — `kCTFontAttributeName`, whose value is PROVABLE now that
 * `CTStringAttributes.h` exists (the AppKit declares `NSFontAttributeName`, and a dictionary crosses). A constant,
 * borrowed, never released. */
CFStringRef CTLineFontAttributeName(void);

#endif /* CORE_TEXT_CTLINE_INTERNAL_H */
