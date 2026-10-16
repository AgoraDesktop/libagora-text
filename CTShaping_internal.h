/*
 * CTShaping_internal.h — the shaping engine: what turns a run of CHARACTERS into the GLYPHS a face actually
 * sets them in, which is the one thing this library takes from outside itself.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER — the same standing `CTBidi_internal.h`
 * and `CTLine_internal.h` have, and for the same reason: Apple's Core Text has no shaping door at all. On the
 * platform this library follows, the shaper is HarfBuzz and Core Text calls it; here the line and the shaper are
 * two things in two files, so they agree in a header rather than one file `extern`ing what the other defines.
 *
 * **WHY HARFBUZZ AND NOT A REIMPLEMENTATION, WHICH IS THE DECISION THIS HEADER RECORDS.** Everything else in this
 * library is written from Apple's headers against the standard libraries the tree already links — ICU for text
 * breaking, `kern`/`cmap`/`post` for the font's own numbers. Shaping is not that. It is OpenType's `GSUB` and
 * `GPOS`: four lookup types of contextual substitution, ligature formation, mark attachment, cursive joining, and
 * a per-SCRIPT rule set that runs to thousands of lines of published behaviour for Devanagari alone. A hand-written
 * shaper would be a fifth-rate one that is WRONG for most of the world's scripts and SILENTLY wrong for many —
 * which is exactly the failure this tree refuses. HarfBuzz IS the milestone's own name in this project's plan, it
 * is what every Linux text stack (and Apple's own port) uses, and `libharfbuzz-dev` is a distribution package.
 *
 * AND THE FONT CROSSES BY ITS OWN TABLES, WHICH IS THE INTEGRATION THIS TREE ALREADY HAS THE DOORS FOR. HarfBuzz
 * is not handed a file path and it is not asked to parse a face independently: `CTFontCopyTable` — a door that
 * landed for the `post`/`OS/2`/`head` metrics — is what answers HarfBuzz's requests for `cmap`, `GSUB`, `GPOS` and
 * the rest. So there is ONE reader of a font's tables in this library, and the shaper is a consumer of it rather
 * than a second one.
 */
#ifndef CORE_TEXT_CTSHAPING_INTERNAL_H
#define CORE_TEXT_CTSHAPING_INTERNAL_H

#include <CoreText/CTFont.h>
#include <CoreText/CTParagraphStyle.h>

#include <CoreFoundation/CFString.h>
#include <CoreGraphics/CGFont.h>
#include <CoreGraphics/CGGeometry.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* **A SHAPER IS THE HARFBUZZ OBJECTS FOR ONE FACE, MADE ONCE AND USED MANY TIMES.** It is not a Core Foundation
 * object and it has no type id: it is an ENGINE, like the bidi tables, and the caller owns it. Making one parses
 * nothing — HarfBuzz reads a table the first time it needs it and caches it in the face — but a face made per CALL
 * would throw that cache away on every line, so the object exists to be kept. */
typedef struct __CTShaper *CTShaperRef;

/* **ONE SHAPED RUN, IN THE CALLER'S OWN COORDINATES.** Every array is `malloc`'d, every array has `glyphCount`
 * entries, and the caller frees all of it with `CTShapedRunFree` — one place that knows what a run owns.
 *
 * THE GLYPH COUNT IS NOT THE CHARACTER COUNT, which is the whole point: a ligature is two characters and one
 * glyph, and a Devanagari syllable can be five characters and two. `clusters` is what keeps the correspondence:
 * `clusters[i]` is the index INTO `chars` of the first character the glyph `i` stands for, so a caller can walk
 * the run and see which characters a glyph covers. */
typedef struct {
	CGGlyph *glyphs;	/* the face's glyph indices, in the SHAPER's order (see CTShaperShape) */
	CGPoint *offsets;	/* each glyph's displacement from its own advance origin, in POINTS */
	CGSize *advances;	/* each glyph's advance, in POINTS */
	CFIndex *clusters;	/* per glyph: the index into `chars` of the character it begins at */
	CFIndex glyphCount;
} CTShapedRun;

/* A SHAPER OVER A FACE, or NULL when the face has no tables HarfBuzz can use — which is a failure of the FACE
 * rather than of the request, and NULL is the answer rather than a shaper that would shape everything to `.notdef`.
 * THE FONT IS RETAINED. */
CTShaperRef CTShaperCreate(CTFontRef font);

/* RELEASES THE SHAPER AND ITS RETAINED FONT. A NULL SHAPER IS ALLOWED AND DOES NOTHING, so a caller whose
 * `CTShaperCreate` failed needs no branch. */
void CTShaperRelease(CTShaperRef shaper);

/* **SHAPE ONE RUN OF CHARACTERS.** `chars` holds `count` UTF-16 units and the run IS the whole of it, so every
 * index a cluster reports is an index into `chars` with nothing to add to it.
 *
 * `direction` is the RUN's direction, and it decides the ORDER the glyphs come back in — see below. `scriptTag` is
 * an ISO 15924 tag ("Latn", "Arab", "Deva") or NULL to let HarfBuzz read the script off the characters, which is
 * right for a run that is one script and the caller's business when it is not. `language` is a Foundation language
 * ("en", "hi") or NULL, which is HarfBuzz's own default and is what most faces need; it matters for the faces that
 * carry language-specific forms.
 *
 * **THE ORDER IS HARFBUZZ'S OWN, AND IT IS NOT THE LINE's L2 ORDER.** For a left-to-right run the glyphs come back
 * in LOGICAL order — the order the characters were given in. For a RIGHT-TO-LEFT run they come back in VISUAL
 * order, which is HarfBuzz's contract and is what a caller that draws a run directly wants. **A LINE THAT HAS
 * ALREADY RUN UAX #9 HAS ITS OWN OPINION ABOUT ORDER** (L2 reorders by resolved level), so the wiring slice is
 * where the two are reconciled; this door states the order it answers in and does not guess at that.
 *
 * Answers false and leaves `out` zeroed when the run cannot be shaped at all — a NULL shaper, an empty run, or a
 * buffer HarfBuzz produced nothing for. `out` is filled by the call and must be released by the caller. */
bool CTShaperShape(CTShaperRef shaper, const UniChar *chars, CFIndex count, CTWritingDirection direction,
		   const char *scriptTag, CFStringRef language, CTShapedRun *out);

/* FILLS `out` WITH ZEROS, which is the state a failed `CTShaperShape` leaves it in and the state a caller may
 * assert. `CTShapedRunFree` on a zeroed run is a no-op. */
void CTShapedRunZero(CTShapedRun *out);

/* RELEASES EVERYTHING A SHAPED RUN OWNS AND ZEROES IT, so a double release is harmless — the same contract
 * `CTShaperRelease` has, and for the same reason: a caller clearing up after a failure should not have to know how
 * far the call got. */
void CTShapedRunFree(CTShapedRun *run);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTSHAPING_INTERNAL_H */
