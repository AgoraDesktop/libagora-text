/*
 * CTRun_internal.h — the seam between a run and the line that makes one.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER. APPLE HAS NO PUBLIC CREATOR FOR A RUN —
 * a run exists because a line (or a typesetter, which does not exist here yet) produced one — so the two files that
 * must agree about how one is made need a place to agree in, and this is it. The alternative was to declare the
 * constructor inside one `.c` file and `extern` it in the other, which is the same coupling with none of the
 * checking.
 *
 * THE ARRAYS ARE BORROWED FOR THE DURATION OF THE CALL AND COPIED, which is the same contract the paragraph
 * style's settings have and for the same reason: the caller keeps its own memory, and the run outlives it.
 */
#ifndef CORE_TEXT_CTRUN_INTERNAL_H
#define CORE_TEXT_CTRUN_INTERNAL_H

#include <CoreText/CTRun.h>

#include <CoreText/CTFont.h>

#include <stdbool.h>

/* MAKE ONE. `attributes` and `font` are RETAINED; the five arrays are COPIED. `count` is the glyph count, and the
 * arrays are all `count` long; `isBox` is NULL when no glyph is a run-delegate box, and otherwise one flag per glyph
 * saying which ones draw no ink. A count of zero is legal and yields a run with nothing in it. */
CTRunRef CTRunCreateWithAttributesAndGlyphs(CFDictionaryRef attributes, CTFontRef font, CFRange stringRange,
					    const CFIndex *stringIndices, const CGGlyph *glyphs,
					    const CGPoint *positions, const CGSize *advances,
					    const bool *isBox, CFIndex count);

#endif /* CORE_TEXT_CTRUN_INTERNAL_H */
