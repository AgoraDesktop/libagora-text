/*
 * CTFont_internal.h — the doors a font owes the layer that TYPESETS with it.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER. Apple's Core Text has no public kerning
 * door at all, and that is a fact about where the work happens rather than an omission: a FONT answers advances,
 * and a TYPESETTER is what knows that the pair `To` wants its two letters closer together — the adjustment is
 * applied while a line is being made and never handed out as a number. Here the line and the font are two objects
 * in two files, so the number has to cross between them, and it crosses in a header instead of one file `extern`ing
 * what the other defines.
 */
#ifndef CORE_TEXT_CTFONT_INTERNAL_H
#define CORE_TEXT_CTFONT_INTERNAL_H

#include <CoreText/CTFont.h>
#include <CoreGraphics/CGFont.h>

/* **THE ADJUSTMENT A PAIR OF GLYPHS ASKS FOR, IN POINTS — WHAT KERNING IS.** Zero is the ordinary answer: most
 * pairs are not in the font's table, and a face with no `kern` table at all answers zero for every pair. A NEGATIVE
 * answer is the usual direction, because closing a gap is what a typographer's pair adjustment almost always does.
 *
 * IT IS SCALED LIKE `CTFontGetAdvancesForGlyphs` IS — both come back in POINTS — so a caller adding the two
 * together is adding the same kind of number.
 *
 * AND IT IS THE `kern` TABLE THE FACE OWES, NOT `GPOS`: see `cg_font_kern`, which answers the same number in font
 * units and states the limit. */
double CTFontGetKerningBetweenGlyphs(CTFontRef font, CGGlyph left, CGGlyph right);

#endif /* CORE_TEXT_CTFONT_INTERNAL_H */
