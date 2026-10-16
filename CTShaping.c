/*
 * CTShaping.c — shaping, over HarfBuzz, with the font's own tables.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WHAT THIS IS FOR. Everything else in this library answers what a font SAYS — an advance, a kerning pair, an
 * ascent, a table. Shaping is the question of what a font DOES with a sequence of characters: which glyphs it sets
 * them in, in what order, and how far apart. `ffi` is three characters and (in a face with the ligature) one glyph;
 * a Devanagari syllable is several characters and a fraction of that many glyphs; an Arabic word's letters change
 * shape according to their neighbours. A "one glyph per character" mapping cannot express any of that, and this file
 * is the engine that can.
 *
 * **THE ENGINE IS HARFBUZZ, AND THAT IS A DECISION RATHER THAN A SHORTCUT.** Shaping is OpenType's `GSUB` and
 * `GPOS` — contextual lookups, ligatures, mark attachment, cursive joining — plus a per-script rule set that runs to
 * thousands of lines of published behaviour for Devanagari alone. A hand-written shaper is a fifth-rate one that is
 * wrong for most of the world's scripts and silently wrong for many, which is the failure this tree refuses; the
 * project's own plan names this milestone "the HarfBuzz milestone"; and `libharfbuzz-dev` is a distribution package
 * on the machine this tree is built on. `CTShaping_internal.h` carries the full statement of that.
 *
 * **AND THE FONT CROSSES AS ITS TABLES, THROUGH THE DOOR THIS LIBRARY ALREADY HAD.** HarfBuzz is never handed a
 * file path and never parses a face behind this library's back: `hb_face_create_for_tables` is given a callback, and
 * that callback is `CTFontCopyTable` — the door that landed for the `post`/`OS/2`/`head` metrics. So `cmap`, `GSUB`,
 * `GPOS`, `GDEF`, `glyf`/`loca`, `hmtx` and the rest all come through ONE reader of a font's tables, and the shaper
 * is a consumer of it rather than a second implementation of it.
 *
 * WHAT THIS FILE IS NOT, SAID PLAINLY, because a shaping engine that looks finished and is not is worse than one
 * that says what it is missing:
 *
 *   - IT DOES NOT ITEMIZE. There is no "split this text into runs of one script, one direction and one face" here.
 *     The caller says what a run is; the script and the language are the caller's, and NULL means "read it off the
 *     characters", which is right only for a run that already is one script.
 *   - IT DOES NOT DO FONT FALLBACK. A character the face has no glyph for shapes to `.notdef` and stays there.
 *   - IT IS NOT WIRED INTO `CTLine` YET, and that is deliberate rather than incomplete. The line's arrays are one
 *     glyph per character, its caret doors index by character, and the AppKit's layout manager counts glyphs one per
 *     character — so shaping the line means changing the correspondence those three rely on, in one change, with the
 *     clusters as the new source of truth. That is the next slice and it is named; what is here is the engine, and
 *     NOTHING IN THE LAYOUT PATH CALLS IT, so nothing that exists today has been made silently wrong.
 *   - AND THE ORDER IT ANSWERS IN IS HARFBUZZ'S: logical for a left-to-right run, VISUAL for a right-to-left one.
 *     A line that has already run UAX #9 has its own order (L2 reorders by resolved level), so the two are
 *     reconciled in the wiring slice rather than guessed at here.
 *
 * THE SCALE IS THE FACTS AND THE CONVERSION IS THIS FILE'S. HarfBuzz answers in whatever scale its font was set to,
 * so the font is set to the face's own UNITS PER EM: every number that comes back is an exact INTEGER of font units,
 * with no rounding anywhere in HarfBuzz. Turning those into points is then one multiplication by `size / upem`, done
 * here, once, so the caller gets points and never has to know what a font unit is.
 */
#include <CoreText/CTShaping_internal.h>

#include <CoreText/CTFont.h>
#include <CoreText/CTFont_internal.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <stdlib.h>
#include <string.h>

struct __CTShaper {
	CTFontRef font;		/* RETAINED: the face the tables come from, which is what the callback needs */
	hb_font_t *hbFont;	/* OWNED: the HarfBuzz font over those tables, with the face's own tables as its funcs */
	hb_unicode_funcs_t *unicode;	/* OWNED: the default Unicode functions WITH MIRRORING TURNED OFF (below) */
	CGFloat size;		/* the point size the scale was built from */
	unsigned upem;		/* the face's units per em, which is also the scale HarfBuzz was set to */
};

/* **MIRRORING IS TURNED OFF, AND THIS IS THE ONE PLACE IT CAN BE TURNED OFF.** HarfBuzz implements UAX #9's **L4**
 * itself — `hb_ot_shape` mirrors a character whose script runs right to left before it looks the glyph up, because
 * the shaper cannot know that its caller already ran the algorithm — and UAX #9's L4 is ALREADY THIS LIBRARY'S
 * (`CTLine.c` runs `ct_bidi_mirror` over its characters so that the font is asked for the character that will be
 * drawn). With both, a `)` in Hebrew text is mirrored to `(` by L4 and then mirrored BACK by HarfBuzz, and the line
 * draws the wrong parenthesis. MEASURED, which is how this was found: the mirrored bracket came back as glyph 11 —
 * the `(` — where the face's `)` is 12.
 *
 * **AND THE MECHANISM IS NOT THE `rtlm` FEATURE, WHICH IS WHAT IT LOOKS LIKE.** Disabling `rtlm` through
 * `hb_shape`'s feature list does NOT stop it — MEASURED, `rtlm=0` still answers 11 for `)` in a right-to-left run,
 * because the mirroring is not a GSUB feature at all. It is a UNICODE FUNCTION, and the only way to say "this text
 * has already been mirrored" is to hand HarfBuzz a Unicode-functions object whose mirroring answers the character
 * unchanged. That object is built once per shaper and its mirroring is the identity; everything else falls through to
 * HarfBuzz's own default, which is what every other shaping decision needs. */
static hb_codepoint_t fn_shaper_identity_mirroring(hb_unicode_funcs_t *ufuncs, hb_codepoint_t unicode, void *user_data)
{
	(void)ufuncs;
	(void)user_data;
	return unicode;
}

/* =================================================================================================
 * THE FACE, OVER THE TABLES THIS LIBRARY ALREADY READS.
 * =============================================================================================== */
/* **ONE TABLE, AS HARFBUZZ ASKS FOR IT.** HarfBuzz calls this for each tag it needs, as it needs it, and caches
 * what it gets — so this is where `cmap`, `GSUB` and `GPOS` are read, and it is the only place in the library that
 * reads a table on another library's behalf.
 *
 * THE BLOB BORROWS THE `CFData` RATHER THAN COPYING IT, and the `CFData` is what the blob's destroy function
 * releases: `hb_blob_create` with `HB_MEMORY_MODE_READONLY` promises the bytes will not change and hands ownership
 * of the reference to the blob, so the table lives exactly as long as HarfBuzz wants it and is freed when it is
 * done — with no copy of a table that can be hundreds of kilobytes. Returning NULL for a table the face does not
 * have is the contract, and it is how HarfBuzz learns that this face has no `kern` table or no `GPOS`. */
static hb_blob_t *fn_shaper_table(hb_face_t *face, hb_tag_t tag, void *user_data)
{
	CTFontRef font = (CTFontRef)user_data;
	CFDataRef data;

	(void)face;
	data = CTFontCopyTable(font, (CTFontTableTag)tag, kCTFontTableOptionNoOptions);
	if (data == NULL) {
		return NULL;
	}
	return hb_blob_create((const char *)CFDataGetBytePtr(data), (unsigned int)CFDataGetLength(data),
			      HB_MEMORY_MODE_READONLY, (void *)data, (hb_destroy_func_t)CFRelease);
}

CTShaperRef CTShaperCreate(CTFontRef font)
{
	struct __CTShaper *shaper;
	hb_face_t *face;
	CGFontRef graphics = NULL;
	int upem = 0;

	if (font == NULL) {
		return NULL;
	}
	graphics = CTFontCopyGraphicsFont(font, NULL);
	if (graphics != NULL) {
		upem = CGFontGetUnitsPerEm(graphics);
	}
	/* **A FACE WITH NO SENSIBLE UNITS PER EM IS NOT ONE TO SHAPE WITH**, and refusing here is cheaper than
	 * producing a run whose every number is wrong by an unknown factor. */
	if (upem <= 0) {
		if (graphics != NULL) {
			CGFontRelease(graphics);
		}
		return NULL;
	}
	/* **THE FACE'S USER DATA IS A RETAINED FONT**, because the callback outlives this call: HarfBuzz holds the
	 * face and calls back for tables long after `CTShaperCreate` has returned, and the destroy function it is given
	 * is what releases that reference when the face goes. The shaper keeps its OWN reference as well, so the font is
	 * alive whether the face outlives the shaper or the other way round. */
	face = hb_face_create_for_tables(fn_shaper_table, (void *)CFRetain(font), (hb_destroy_func_t)CFRelease);
	if (face == NULL) {
		CFRelease(font);
		CGFontRelease(graphics);
		return NULL;
	}
	shaper = (struct __CTShaper *)calloc(1, sizeof(struct __CTShaper));
	if (shaper == NULL) {
		hb_face_destroy(face);
		CGFontRelease(graphics);
		return NULL;
	}
	shaper->font = (CTFontRef)CFRetain(font);
	shaper->size = CTFontGetSize(font);
	shaper->upem = (unsigned)upem;
	shaper->hbFont = hb_font_create(face);
	/* THE FACE IS THE FONT'S NOW, so this reference goes whether or not the font was made. */
	hb_face_destroy(face);
	CGFontRelease(graphics);
	if (shaper->hbFont == NULL) {
		CTShaperRelease(shaper);
		return NULL;
	}
	/* **THE SCALE IS THE FACE'S OWN UNITS PER EM, WHICH IS WHAT MAKES EVERY ANSWER EXACT.** HarfBuzz rounds to the
	 * scale it is given, so a scale of `upem` means no rounding at all: a value comes back as the integer number of
	 * font units the table holds, and the conversion to points happens once, in `CTShaperShape`. */
	hb_font_set_scale(shaper->hbFont, (int)upem, (int)upem);
	/* AND HAPPINESS IS OFF, WHICH IS THE WHOLE ENGINE. `hb_ot_font_set_funcs` is the OpenType implementation over
	 * the face's tables: `GSUB`, `GPOS`, `GDEF`, `cmap`, `hmtx`, `glyf`/`CFF`. `hb_font_create` sets it already;
	 * naming it here is what makes the dependency visible to a reader. */
	hb_ot_font_set_funcs(shaper->hbFont);
	{
		hb_unicode_funcs_t *base = hb_unicode_funcs_get_default();

		shaper->unicode = hb_unicode_funcs_create(base);
		if (shaper->unicode != NULL) {
			hb_unicode_funcs_set_mirroring_func(shaper->unicode, fn_shaper_identity_mirroring, NULL, NULL);
		}
	}
	return shaper;
}

void CTShaperRelease(CTShaperRef shaper)
{
	struct __CTShaper *s = (struct __CTShaper *)shaper;

	if (s == NULL) {
		return;
	}
	/* THE HARFBUZZ FONT FIRST: it holds the face, and the face's destroy function is what releases the font
	 * reference the callback was given. */
	if (s->hbFont != NULL) {
		hb_font_destroy(s->hbFont);
	}
	if (s->unicode != NULL) {
		hb_unicode_funcs_destroy(s->unicode);
	}
	if (s->font != NULL) {
		CFRelease(s->font);
	}
	free(s);
}

/* =================================================================================================
 * THE RUN.
 * =============================================================================================== */
void CTShapedRunZero(CTShapedRun *out)
{
	if (out != NULL) {
		memset(out, 0, sizeof(CTShapedRun));
	}
}

void CTShapedRunFree(CTShapedRun *run)
{
	if (run == NULL) {
		return;
	}
	free(run->glyphs);
	free(run->offsets);
	free(run->advances);
	free(run->clusters);
	CTShapedRunZero(run);
}

/* ONE FONT UNIT, IN POINTS. The only place the two units meet, so a reader looking for "where does a font unit
 * become a point" finds one multiplication rather than a scatter of them. */
static CGFloat fn_shaper_points(const struct __CTShaper *shaper, int units)
{
	return (CGFloat)units * shaper->size / (CGFloat)shaper->upem;
}

bool CTShaperShape(CTShaperRef shaper, const UniChar *chars, CFIndex count, CTWritingDirection direction,
		   const char *scriptTag, CFStringRef language, CTShapedRun *out)
{
	struct __CTShaper *s = (struct __CTShaper *)shaper;
	hb_buffer_t *buffer;
	hb_glyph_info_t *info;
	hb_glyph_position_t *positions;
	unsigned int length = 0;
	CFIndex i;

	CTShapedRunZero(out);
	if (s == NULL || out == NULL || chars == NULL || count <= 0) {
		return false;
	}
	buffer = hb_buffer_create();
	if (buffer == NULL) {
		return false;
	}
	hb_buffer_add_utf16(buffer, (const uint16_t *)chars, (int)count, 0, (int)count);
	hb_buffer_set_direction(buffer, (direction == kCTWritingDirectionRightToLeft) ? HB_DIRECTION_RTL
										    : HB_DIRECTION_LTR);
	/* **A SCRIPT TAG FROM THE CALLER IS A STATEMENT AND IS OBEYED; NO TAG IS A QUESTION.** HarfBuzz reads an
	 * ISO 15924 tag off the first strong character, which is right for the runs a line is made of and is the
	 * caller's business to get right when its run is not one script. The LANGUAGE works the same way, and it is
	 * converted through Foundation because the caller holds a `CFStringRef` and HarfBuzz wants a C string. */
	if (scriptTag != NULL && scriptTag[0] != '\0') {
		hb_buffer_set_script(buffer, hb_script_from_string(scriptTag, -1));
	}
	if (language != NULL) {
		char langTag[64];

		if (CFStringGetCString(language, langTag, (CFIndex)sizeof(langTag), kCFStringEncodingUTF8)) {
			hb_buffer_set_language(buffer, hb_language_from_string(langTag, -1));
		}
	}
	if (scriptTag == NULL || scriptTag[0] == '\0') {
		/* Fills in the script, the language AND the direction from the characters — the direction is set above and
		 * is not overwritten, because a line that knows its direction from UAX #9 knows better than a guess. */
		hb_buffer_guess_segment_properties(buffer);
		hb_buffer_set_direction(buffer, (direction == kCTWritingDirectionRightToLeft) ? HB_DIRECTION_RTL
											    : HB_DIRECTION_LTR);
	}
	/* **AND THE BUFFER IS TOLD THE CHARACTERS ARE ALREADY MIRRORED** — see `fn_shaper_identity_mirroring`, which is
	 * where that decision and its measurement live. `rtlm` is NOT the mechanism and turning it off does nothing. */
	if (s->unicode != NULL) {
		hb_buffer_set_unicode_funcs(buffer, s->unicode);
	}
	hb_shape(s->hbFont, buffer, NULL, 0);
	info = hb_buffer_get_glyph_infos(buffer, &length);
	positions = hb_buffer_get_glyph_positions(buffer, &length);
	if (info == NULL || positions == NULL || length == 0) {
		hb_buffer_destroy(buffer);
		return false;
	}
	out->glyphs = (CGGlyph *)malloc((size_t)length * sizeof(CGGlyph));
	out->offsets = (CGPoint *)malloc((size_t)length * sizeof(CGPoint));
	out->advances = (CGSize *)malloc((size_t)length * sizeof(CGSize));
	out->clusters = (CFIndex *)malloc((size_t)length * sizeof(CFIndex));
	if (out->glyphs == NULL || out->offsets == NULL || out->advances == NULL || out->clusters == NULL) {
		hb_buffer_destroy(buffer);
		CTShapedRunFree(out);
		return false;
	}
	out->glyphCount = (CFIndex)length;
	for (i = 0; i < (CFIndex)length; i++) {
		out->glyphs[i] = (CGGlyph)info[i].codepoint;
		/* THE CLUSTER IS AN INDEX INTO THE BUFFER, and the buffer is exactly the run, so it is an index into
		 * `chars` with nothing to add — which is what makes a caller's translation back to the string a lookup
		 * rather than an arithmetic. */
		out->clusters[i] = (CFIndex)info[i].cluster;
		out->advances[i] = CGSizeMake(fn_shaper_points(s, positions[i].x_advance),
					      fn_shaper_points(s, positions[i].y_advance));
		/* **THE OFFSET IS A DISPLACEMENT, NOT A POSITION**, which is the difference a mark makes: HarfBuzz says
		 * "draw this glyph this far from where its own advance would have put it", and it is how an accent is
		 * moved onto the letter it belongs to. */
		out->offsets[i] = CGPointMake(fn_shaper_points(s, positions[i].x_offset),
					      fn_shaper_points(s, positions[i].y_offset));
	}
	hb_buffer_destroy(buffer);
	return true;
}
