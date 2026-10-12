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
 * THE SECOND DECLARATION IS THE ATTRIBUTE KEY, and it is here for the same reason: the key that names the font
 * inside an attributed string (`kCTFontAttributeName`) belongs to `CTStringAttributes.h`, which is LATER in the
 * include order this repository writes the headers in, so `CTLine.c` carries its VALUE as a recorded boundary. A
 * typesetter has to read the same key to know what face it is measuring, and THE VALUE MUST NOT BE WRITTEN TWICE —
 * a second copy is a second thing to keep in step — so the one copy is handed out from where it lives.
 */
#ifndef CORE_TEXT_CTLINE_INTERNAL_H
#define CORE_TEXT_CTLINE_INTERNAL_H

#include <CoreText/CTLine.h>

/* A LINE OVER A RANGE OF AN ATTRIBUTED STRING, which is what a typesetter makes. `range` is IN THE STRING'S OWN
 * COORDINATES and is what the line reports as its string range; the glyphs are that range's, positioned from the
 * line's OWN origin, exactly as the whole-string door does it. A range that does not lie inside the string answers
 * NULL. */
CTLineRef CTLineCreateWithAttributedStringAndRange(CFAttributedStringRef string, CFRange range);

/* THE FONT ATTRIBUTE'S NAME, AS THIS LIBRARY WRITES IT. A constant, borrowed, never released — see this file's head
 * and `CTLine.c`, where its value and its reason live. */
CFStringRef CTLineFontAttributeName(void);

#endif /* CORE_TEXT_CTLINE_INTERNAL_H */
