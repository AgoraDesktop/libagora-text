/*
 * CTLine.m — the line class, and THE SHAPER.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THIS FILE IS WHERE CORE TEXT BECOMES CORE TEXT.** Everything before it — the font object, the
 * descriptor, the metrics, the character map, the coverage set — is a font. A LINE is what turns a
 * string into POSITIONED GLYPHS, and the turning is done by HARFBUZZ, linked with `hb-ft` exactly as the
 * plan's rule 3 requires: the shaper is handed the face CoreGraphics already opened (`cg_font_face`), so
 * the glyph ids the shaper emits, the advances the metrics door reports and the outlines the drawing
 * machinery will use are ALL ONE FACE'S — the alternative, a second handle on the same file, is the
 * defect this guest measured long ago (`font_twice`).
 *
 * WHAT SHAPING ADDS OVER THE CHARACTER MAP IS THE WHOLE REASON FOR THE UNIT, and it is measurable in this
 * font: with the font's own default features, "AV" advances 1001/64 pt and "fi" is ONE GLYPH where the
 * character map alone would give two and 1050/64 (measured before the unit was written). The probe
 * asserts both, because "a shaped run whose positions differ from the unshaped advance sum" is this
 * milestone's stated proof.
 *
 * THE SHAPER'S INPUTS ARE NAMED RATHER THAN ASSUMED: the segment properties (script, direction,
 * language) come from HarfBuzz's own guess, because Apple's door for naming them is `CTTypesetter`'s and
 * that is M2b's unit; the size is set on the shared face (`FT_Set_Char_Size`) per the "one face per font,
 * SIZED PER USE" rule `CGFont.h` states, and the drawing side re-sets its own when it draws; and the
 * features are the font's defaults, which is what turns kerning and ligatures ON.
 */
#import <Foundation/Foundation.h>

#include "CTLine.h"
#include "CTRun_internal.h"
/* **ONE IMPLEMENTATION OF THE INK BOX** (see the header): this file and CTRun.m both ask
 * for ink, and a second copy is a second place to get the glyph origin wrong. */
#include "CTInk_internal.h"
/* **THE UNENCODED-GLYPH OVERRIDE (M6f)**: a `kCTGlyphInfoAttributeName` value is a `CTGlyphInfo` object, and
 * this file is where the range's CONTENTS are in hand — which is the first of the key's two conditions. */
#include "CTGlyphInfo_internal.h"
/* **THE FONT'S SEAM, BECAUSE THE SHAPER READS THE FONT'S OWN IVARS**: `font->graphics` (to reach the face
 * the CoreGraphics font already opened) and `font->size` (to set it on that face). A forward `@class` is
 * not enough for that — the compiler says "incomplete definition of type 'CTFont'", which is how this
 * include came to be written. */
#include "CTFont_internal.h"
#include <CoreText/CTStringAttributes.h>
#include <CoreGraphics/CGFont_internal.h>

#include <ft2build.h>
#include FT_FREETYPE_H
/* THE SHAPER'S HEADERS, SPELLED WITH THE DIRECTORY NAMED, AND THAT IS A DELIBERATE CHOICE RATHER THAN A
 * STYLE ONE: `-I` paths differ between the two builds (the guest's HarfBuzz lives under the X prefix the
 * CoreGraphics flags already name, the host's under `/usr/include`), while BOTH put the headers in a
 * `harfbuzz/` subdirectory of a directory that IS on the include path. So `<harfbuzz/hb.h>` resolves on
 * both sides with no new flag, and the one flag that IS needed — the link — is named beside the other
 * libraries. */
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

#include <stdlib.h>
/* `strlen` IS USED BY EXACTLY ONE LINE OF THIS FILE — the shaper's byte-length arithmetic for the run's
 * string range — and the include sits here rather than in the block above because that block is about the
 * ENGINE (FreeType, HarfBuzz) while this is the C library. */
#include <string.h>

/* ONE FACE PER FONT, SIZED PER USE: HarfBuzz's positions come back in 26.6 at whatever size the face
 * carries, so the size the font object holds is the size the face is given — hence the /64.0 below. */
#define CT_SHAPE_UNITS_PER_POINT 64.0

/* **THE SUPERSCRIPT RULE IS OURS, AND IT IS RECORDED AS A DIVERGENCE.** The era's own sentence about
 * `kCTSuperscriptAttributeName` makes it conditional — "**if supported by the specified font**, a value of 1
 * enables superscripting and a value of -1 enables subscripting" — and the superscript offsets it refers to
 * are OS/2's, which this tier's font engine does not publish. So the rise (and the drop) is a FRACTION OF
 * THE FONT SIZE, these two numbers, rather than a table: a rule, not a table, which is the same boundary
 * the calendar and the URL types drew. */
#define CT_SUPERSCRIPT_FRACTION 0.34
#define CT_SUBSCRIPT_FRACTION 0.20

@interface CTLine : NSObject {
	@public
	NSArray *runs;			/* CTRun objects, retained by the array */
	long glyph_count;
	double width;
	double trailing_whitespace;
	double ascent;
	double descent;
	double leading;
}
@end

@implementation CTLine

- (void)dealloc
{
	[runs release];
	[super dealloc];
}

@end

/* -------------------------------------------------------------------------------------------------
 * THE SHAPER: one attributed string's text, one font, one run.
 * ---------------------------------------------------------------------------------------------- */

/* A SPACE, TAB, NEWLINE OR CARRIAGE RETURN — the four characters this tier counts as whitespace for
 * `CTLineGetTrailingWhitespaceWidth`, tested on the UTF-8 BYTE because that is what the clusters index
 * into. Named rather than delegated: this tree has no `NSCharacterSet`-based answer for a BYTE, and the
 * four bytes are the whole of what an ASCII text needs. */
static int ct_is_space_byte(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* **A RUN THAT IS ONE CALLER-CHOSEN GLYPH**, which is what `kCTGlyphInfoAttributeName` produces: the shaper is
 * not consulted at all, because the glyph is not the text's — that is the point of the key ("allows the use of
 * unencoded glyphs"). Its advance comes from the font's own metrics for that glyph, so the run measures like
 * any other and the line's width stays honest.
 *
 * THE OTHER CONDITIONS ARE THE CALLER'S: the base string has already been compared, and `ctg_is_resolvable`
 * has already answered whether the font carries the glyph. This function is handed a glyph that qualifies. */
static CTRun *ct_run_from_glyph(CTFontRef font, CGGlyph glyph, NSString *piece, NSUInteger byte_base,
				double x_offset, NSDictionary *attributes)
{
	FT_Face face = (FT_Face)cg_font_face(font->graphics);
	const char *utf8;
	double advance = 0.0;
	CTRun *run;

	if (face == NULL) {
		return nil;
	}
	FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font->size * CT_SHAPE_UNITS_PER_POINT), 72, 72);
	if (FT_Load_Glyph(face, glyph, FT_LOAD_DEFAULT) == 0) {
		advance = (double)face->glyph->advance.x / (double)CT_SHAPE_UNITS_PER_POINT;
	}
	utf8 = [piece UTF8String];
	if (utf8 == NULL) {
		utf8 = "";
	}
	run = [[CTRun alloc] init];
	run->glyph_count = 1;
	run->attributes = [attributes retain];
	run->string_start = (long)byte_base;
	run->string_length = (long)strlen(utf8);
	run->trailing_whitespace = 0.0;
	run->glyphs = malloc(sizeof(CGGlyph));
	run->positions = malloc(sizeof(CGPoint));
	run->advances = malloc(sizeof(CGSize));
	run->string_indices = malloc(sizeof(long));
	if (run->glyphs == NULL || run->positions == NULL || run->advances == NULL ||
	    run->string_indices == NULL) {
		[run release];
		return nil;
	}
	run->glyphs[0] = glyph;
	run->positions[0] = CGPointMake((CGFloat)x_offset, 0.0);
	run->advances[0] = CGSizeMake((CGFloat)advance, 0.0);
	run->string_indices[0] = 0;
	return run;
}

static CTRun *ct_shape_run(CTFontRef font, NSString *text, NSRange range, NSUInteger byte_base,
			   double x_offset, NSDictionary *attributes)
{
	NSString *piece;
	const char *utf8;
	FT_Face face;
	hb_font_t *hb;
	hb_buffer_t *buf;
	hb_glyph_info_t *info;
	hb_glyph_position_t *pos;
	unsigned count = 0;
	unsigned i;
	CTRun *run;
	/* **THE PEN STARTS AT THE RUN'S OFFSET IN THE LINE, WHICH IS APPLE'S CONTRACT IN ITS OWN WORDS**: "the
	 * glyph positions in a run are relative to the origin of the line containing the run" (`CTRun.h`). A run
	 * shaped from zero is right only while it is the ONLY run — the second run of a two-font line would be
	 * drawn on top of the first, which is exactly what this tier's drawing probe caught. */
	double pen = x_offset;

	/* **THE PIECE THIS RUN IS SHAPED FROM** is one attribute run of the line's string, named by the caller
	 * (`CTLineCreateWithAttributedString` walks the attribute ranges). Shaping the PIECE rather than the
	 * whole string is what makes several runs possible at all — and it is why `byte_base` is a parameter:
	 * the shaper's clusters are offsets into the piece, while the run's string range has to be stated in
	 * the LINE's bytes (`CTRun.h` records those units). */
	piece = (range.location == 0 && range.length == [text length]) ? text
								      : [text substringWithRange:range];
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return nil;
	}
	FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font->size * CT_SHAPE_UNITS_PER_POINT), 72, 72);
	hb = hb_ft_font_create_referenced(face);
	if (hb == NULL) {
		return nil;
	}
	buf = hb_buffer_create();
	utf8 = [piece UTF8String];
	if (utf8 == NULL) {
		utf8 = "";
	}
	hb_buffer_add_utf8(buf, utf8, -1, 0, -1);
	/* **THE SCRIPT AND LANGUAGE ARE HARFBUZZ'S OWN GUESS** (see the file comment): Apple's doors for
	 * naming them are `CTTypesetter`'s, so guessing is what this shaper can honestly do, and the guess is
	 * the shaper's own rather than a table this tier invents. */
	hb_buffer_guess_segment_properties(buf);
	/* AND THE FEATURES ARE THE FONT'S DEFAULTS — which is what turns kerning and ligatures ON, and what
	 * makes this door different from summing `CTFontGetAdvancesForGlyphs`. */
	/* **M6d: THE THREE NUMERIC TEXT ATTRIBUTES, EACH WITH THE ERA'S OWN CONTRACT IN HAND.** All three are
	 * `AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER` in `CTStringAttributes.h`, and each is a number:
	 *
	 *   * `kCTKernAttributeName` — "how many points the FOLLOWING character should be shifted from its default
	 *     offset … a positive kern indicates a shift farther along and a negative kern indicates a shift closer
	 *     … **If this attribute is set to 0.0, no kerning will be done at all**";
	 *   * `kCTLigatureAttributeName` — "**0** indicates that only ligatures **essential for proper rendering** of
	 *     text should be used, **1** indicates that standard ligatures should be used, and **2** indicates that
	 *     all available ligatures should be used";
	 *   * `kCTSuperscriptAttributeName` — "a value of **1** enables superscripting and a value of **-1** enables
	 *     subscripting".
	 *
	 * The first two are read here because they are SHAPER settings; the third is applied to the shaped
	 * positions below. `kern_set` exists because the era treats a missing attribute and a 0.0 one
	 * differently — absent is "standard kerning", zero is "none at all". */
	double kern = 0.0;
	int ligature = 1;
	int superscript = 0;
	int kern_set = 0;
	hb_feature_t features[4];
	unsigned nfeatures = 0;
	{
		id value = [attributes objectForKey:kCTKernAttributeName];

		if ([value respondsToSelector:@selector(doubleValue)]) {
			kern = [value doubleValue];
			kern_set = 1;
		}
		value = [attributes objectForKey:kCTLigatureAttributeName];
		if ([value respondsToSelector:@selector(intValue)]) {
			ligature = [value intValue];
		}
		value = [attributes objectForKey:kCTSuperscriptAttributeName];
		if ([value respondsToSelector:@selector(intValue)]) {
			superscript = [value intValue];
		}
	}
	/* LIGATURES, IN HARFBUZZ'S OWN VOCABULARY: the standard pair is `liga`/`clig`; "all available" reaches the
	 * DISCRETIONARY and HISTORICAL sets as well; and `rlig` — the REQUIRED joins that Arabic and the Indic
	 * scripts need for proper rendering — is left on at every setting, which is what the era means by
	 * "essential". */
	if (ligature <= 0) {
		hb_feature_from_string("liga=0", -1, &features[nfeatures++]);
		hb_feature_from_string("clig=0", -1, &features[nfeatures++]);
	} else if (ligature >= 2) {
		hb_feature_from_string("dlig=1", -1, &features[nfeatures++]);
		hb_feature_from_string("hlig=1", -1, &features[nfeatures++]);
	}
	if (kern_set && kern == 0.0) {
		hb_feature_from_string("kern=0", -1, &features[nfeatures++]);
	}
	/* **AND THE VERTICAL FORMS, WHICH ARE ALTERNATES RATHER THAN A LAYOUT** (M6e): the era's key says
	 * "true indicates that vertical glyph forms are to be used", which is HarfBuzz's `vert` — the font's
	 * upright variants — and nothing else. No stacking, no rotation, no direction change: the probe requires
	 * the layout to come out undisturbed, which is how a naive reading of the key's NAME would fail. */
	{
		id vertical = [attributes objectForKey:kCTVerticalFormsAttributeName];

		if (vertical != nil && [vertical respondsToSelector:@selector(boolValue)] &&
		    [vertical boolValue]) {
			hb_feature_from_string("vert=1", -1, &features[nfeatures++]);
		}
	}
	hb_shape(hb, buf, features, nfeatures);
	info = hb_buffer_get_glyph_infos(buf, &count);
	pos = hb_buffer_get_glyph_positions(buf, &count);
	/* **AND THE TWO ADJUSTMENTS TO THE SHAPED POSITIONS, which is the only place both are in hand.** The
	 * kern shifts every glyph EXCEPT THE FIRST — "the FOLLOWING character" — which also leaves the run's
	 * first advance and a trailing space's advance alone; the superscript raises (or lowers) the WHOLE run
	 * including its first glyph, by the fraction defined at the top of this file. */
	{
		double shift = 0.0;

		if (superscript > 0) {
			shift = font->size * CT_SUPERSCRIPT_FRACTION;
		} else if (superscript < 0) {
			shift = -(font->size * CT_SUBSCRIPT_FRACTION);
		}
		for (i = 0; i < count; i++) {
			if (kern_set && i > 0) {
				pos[i].x_advance += (hb_position_t)(kern * CT_SHAPE_UNITS_PER_POINT);
			}
			if (shift != 0.0) {
				pos[i].y_offset += (hb_position_t)(shift * CT_SHAPE_UNITS_PER_POINT);
			}
		}
	}
	run = [[CTRun alloc] init];
	run->glyph_count = (long)count;
	run->attributes = [attributes retain];
	/* **AND THE BYTES THIS RUN CAME FROM**, which is what `CTRunGetStringRange` answers: the piece's own
	 * first cluster (a byte offset WITHIN the piece, hence `byte_base`) plus the piece's byte length. It is
	 * computed from the clusters the shaper reported rather than assumed, which is what let this code
	 * survive the change from one run per line to several without being rewritten. */
	run->string_start = (long)byte_base + ((count > 0) ? (long)info[0].cluster : 0);
	run->string_length = (long)strlen(utf8);
	/* AND THE WHITESPACE THIS RUN ENDS WITH (see `CTRun_internal.h`): taken here because this function is
	 * the only one holding the run's own bytes, and read by the line from its LAST run. */
	run->trailing_whitespace = 0.0;
	{
		int idx = (int)count - 1;

		while (idx >= 0 && ct_is_space_byte(utf8[info[idx].cluster])) {
			run->trailing_whitespace += pos[idx].x_advance / CT_SHAPE_UNITS_PER_POINT;
			idx--;
		}
	}
	if (count > 0) {
		run->glyphs = malloc(count * sizeof(CGGlyph));
		run->positions = malloc(count * sizeof(CGPoint));
		run->advances = malloc(count * sizeof(CGSize));
		run->string_indices = malloc(count * sizeof(long));
		if (run->glyphs == NULL || run->positions == NULL || run->advances == NULL ||
		    run->string_indices == NULL) {
			[run release];
			hb_buffer_destroy(buf);
			hb_font_destroy(hb);
			return nil;
		}
	}
	for (i = 0; i < count; i++) {
		run->glyphs[i] = (CGGlyph)info[i].codepoint;
		/* **THE PEN BEFORE THE GLYPH, so position + advance[0] is the next one: which is what Apple's
		 * positions mean and what a drawing caller adds its own origin to. AND THE Y IS THE SHAPER'S OWN
		 * `y_offset`, NOT A ZERO** — it was hard-coded zero here until the numeric-attribute probe asked for a
		 * superscript and got `y == 0` at every glyph: HarfBuzz puts a superscript's (and a subscript's, and
		 * mark positioning's) vertical displacement in `y_offset`, and this line was discarding it because
		 * nothing before M6d had ever made it non-zero. */
		run->positions[i] = CGPointMake((CGFloat)pen,
						(CGFloat)(pos[i].y_offset / CT_SHAPE_UNITS_PER_POINT));
		run->advances[i] = CGSizeMake((CGFloat)(pos[i].x_advance / CT_SHAPE_UNITS_PER_POINT),
					      (CGFloat)(pos[i].y_advance / CT_SHAPE_UNITS_PER_POINT));
		/* THE CLUSTER, WHICH IS THE BYTE OFFSET IN THE UTF-8 TEXT THIS SHAPER HANDED OVER (CTRun.h
		 * records the units and the divergence from Apple's UTF-16 indices). */
		run->string_indices[i] = (long)info[i].cluster;
		pen += pos[i].x_advance / CT_SHAPE_UNITS_PER_POINT;
	}
	hb_buffer_destroy(buf);
	hb_font_destroy(hb);
	return run;
}

/* -------------------------------------------------------------------------------------------------
 * THE LINE DOORS.
 * ---------------------------------------------------------------------------------------------- */

CTLineRef CTLineCreateWithAttributedString(NSAttributedString *string)
{
	CTLine *line;
	NSString *text;
	NSUInteger count;
	NSUInteger i;
	double x_offset;

	if (string == nil) {
		return NULL;
	}
	line = [[CTLine alloc] init];
	line->runs = [[NSMutableArray array] retain];
	text = [string string];
	/* AN EMPTY STRING IS AN EMPTY LINE, and it needs no font: an attributed string with no characters has
	 * no attributes at index 0 to ask for (`-attributesAtIndex:` is documented to raise past the end), and
	 * "a line with no glyphs" is what such a string describes. */
	if ([string length] == 0) {
		return (CTLineRef)line;
	}
	/* **AND NOW THE STRING IS SPLIT INTO RUNS, ONE PER ATTRIBUTE RANGE** (CT-M2b-2). Apple's model is
	 * exactly this: a run is a maximal range of the string whose attributes do not change, and
	 * `-attributesAtIndex:effectiveRange:` is the door that says where the current one ends. So the walk is
	 * the model rather than an approximation of it, and each piece is shaped by `ct_shape_run` with the
	 * font ITS OWN attributes name — which is what makes a line with two fonts possible at all. */
	count = [string length];
	x_offset = 0.0;
	line->width = 0.0;
	line->trailing_whitespace = 0.0;
	line->ascent = 0.0;
	line->descent = 0.0;
	line->leading = 0.0;
	for (i = 0; i < count; i += 0) {
		NSRange effective = NSMakeRange(i, count - i);
		NSDictionary *attributes = [string attributesAtIndex:i effectiveRange:&effective];
		/* **NO FONT, NO LINE** — Apple's door takes neither a font nor a size, so the font has to come out
		 * of the attributes, and a range that names none cannot be shaped: the LINE is refused rather than
		 * substituting one. */
		CTFontRef font = (CTFontRef)[attributes objectForKey:kCTFontAttributeName];
		NSUInteger base = 0;
		CTRun *run;
		long g;

		if (font == NULL) {
			[line release];
			return NULL;
		}
		if (effective.length == 0) {
			effective = NSMakeRange(i, count - i);	/* a zero-length range would not advance */
		}
		/* **THE PIECE'S BYTE OFFSET IN THE LINE'S STRING**, which is what the run's string range needs. It
		 * is taken from the string itself — `strlen` of the prefix's UTF-8 — rather than assumed to be the
		 * character index, because the units this tree's strings report a range in are the string's own
		 * business and the shaper's clusters are bytes. */
		if (effective.location > 0) {
			base = strlen([[text substringToIndex:effective.location] UTF8String]);
		}
		/* **THE UNENCODED-GLYPH OVERRIDE, ASKED BEFORE THE SHAPER** — and both of the key's conditions are
		 * consulted: "its contents match the specified base string" is the comparison below (the range's text
		 * is `effective` and the string is right here), and "the specified glyph is available in the font" is
		 * `ctg_is_resolvable`. A glyph info that fails either one changes nothing at all. */
		run = nil;
		{
			CTGlyphInfo *glyph_info = (attributes != nil)
							? [attributes objectForKey:kCTGlyphInfoAttributeName]
							: nil;

			if (glyph_info != nil && [glyph_info isKindOfClass:[CTGlyphInfo class]]) {
				NSString *wanted = ctg_base_string(glyph_info);
				CGGlyph glyph = 0;

				if (wanted != nil && [wanted isEqual:[text substringWithRange:effective]] &&
				    ctg_is_resolvable(glyph_info, font, &glyph)) {
					run = ct_run_from_glyph(font, glyph, [text substringWithRange:effective],
								base, x_offset, attributes);
				}
			}
		}
		if (run == nil) {
			run = ct_shape_run(font, text, effective, base, x_offset, attributes);
		}
		if (run != nil) {
			double ascent = CTFontGetAscent(font);
			double descent = CTFontGetDescent(font);
			double leading = CTFontGetLeading(font);
			/* **THE FIRST RUN'S METRICS ARE TAKEN VERBATIM AND THE REST ARE MAXIMA**, and the difference
			 * matters: seeding these with zero is a SIGN ASSUMPTION, and this tree's descent is not always
			 * positive. A probe check that predates this unit caught exactly that — with the seed at 0, a
			 * negative descent came back as 0 — which is the argument for re-running the probes when a
			 * shared door changes. */
			BOOL first_run = ([(NSMutableArray *)line->runs count] == 0);

			[(NSMutableArray *)line->runs addObject:run];
			line->glyph_count += run->glyph_count;
			for (g = 0; g < run->glyph_count; g++) {
				line->width += run->advances[g].width;
				/* AND THE OFFSET THE *NEXT* RUN WILL START AT, which is what makes its positions
				 * line-relative rather than run-relative. */
				x_offset += run->advances[g].width;
			}
			/* THE LAST RUN'S, because each iteration overwrites: a line's trailing whitespace belongs to
			 * whichever run it actually ends in, and a bold space measures differently from a regular one. */
			line->trailing_whitespace = run->trailing_whitespace;
			/* AND THE METRICS ARE THE MAXIMUM OVER THE RUNS, which is what Apple's typographic bounds mean
			 * for a line whose runs carry different fonts. */
			if (first_run || ascent > line->ascent) {
				line->ascent = ascent;
			}
			if (first_run || descent > line->descent) {
				line->descent = descent;
			}
			if (first_run || leading > line->leading) {
				line->leading = leading;
			}
			[run release];	/* the array holds it */
		}
		i = effective.location + effective.length;
	}
	return (CTLineRef)line;
}

long CTLineGetGlyphCount(CTLineRef line)
{
	return (line != NULL) ? line->glyph_count : 0;
}

NSArray *CTLineGetGlyphRuns(CTLineRef line)
{
	if (line == NULL) {
		return nil;
	}
	return [(NSArray *)line->runs retain];
}

double CTLineGetTypographicBounds(CTLineRef line, CGFloat *ascent, CGFloat *descent, CGFloat *leading)
{
	if (line == NULL) {
		return 0.0;
	}
	if (ascent != NULL) {
		*ascent = (CGFloat)line->ascent;
	}
	if (descent != NULL) {
		*descent = (CGFloat)line->descent;
	}
	if (leading != NULL) {
		*leading = (CGFloat)line->leading;
	}
	return line->width;
}

double CTLineGetTrailingWhitespaceWidth(CTLineRef line)
{
	return (line != NULL) ? line->trailing_whitespace : 0.0;
}

/* THE LINE'S STRING RANGE IS THE UNION OF ITS RUNS', which is why this door needs no state of its own: a
 * line is exactly the characters its runs came from, and the runs already know (CTRun.h records the units
 * and the divergence from Apple's UTF-16 indices). */
/* =================================================================================================
 * CT-M3: THE DRAWING DOORS. THE PLAN'S RULE 1 IN AS MANY WORDS — the bodies reach `CGContextSetFont`,
 * `CGContextSetFontSize`, `CGContextSetTextMatrix` and `CGContextShowGlyphsAtPositions`, the glyph doors
 * Apple did NOT deprecate, and never the ones it did. §15 measured their rules (the mask route's
 * device-coordinate placement, the pen rounding, the flip), so nothing here rounds a pen or flips a y: the
 * context's own CTM does that, which is what those rules were about.
 * =============================================================================================== */

/* A RUN'S FONT IS WHERE ITS ATTRIBUTES BECOME A DRAWING: the font is the value of `kCTFontAttributeName`, and
 * the context wants a `CGFontRef`, a size and a matrix beside the glyphs. */


/* **A STROKED RUN, IN THE ERA'S THREE MODES.** "interpreted as a percentage of font point size" is the line
 * width; a POSITIVE width draws "stroke only" and a NEGATIVE one "stroke and fill", so the sign chooses
 * between `StrokePath` and `DrawPath(kCGPathFillStroke)` and the magnitude is scaled by the point size. The
 * stroke's colour is the key's if it has one and the FOREGROUND colour otherwise, which is the colour key's
 * own stated default — and that one was set before this was called.
 *
 * One glyph at a time into one path: the run's positions are relative to its origin, and the sink places each
 * outline at its own position, so a stroked run measures and draws like any other. */
static int ct_draw_stroked_run(CTRunRef run, CGContextRef context, NSRange r, double width_pct)
{
	CGMutablePathRef path = CGPathCreateMutable();
	CGColorRef stroke_color = NULL;
	/* **THE FONT COMES FROM THE RUN'S ATTRIBUTES**, not from an ivar: a run does not store the font it was
	 * shaped with, it stores the ATTRIBUTES it was shaped from, which is what `kCTFontAttributeName` is for. */
	CTFontRef font = (run->attributes != nil)
				? (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName] : NULL;
	CGFloat size = (font != NULL) ? CTFontGetSize(font) : 0.0;
	int drawn = 0;

	if (path == NULL || font == NULL || size <= 0.0) {
		if (path != NULL) {
			CGPathRelease(path);
		}
		return 0;
	}
	/* THE RANGE IS ALREADY NORMALISED BY THE CALLER: `ct_draw_run` applies this framework's empty-range rule
	 * (a length of 0 means the whole run) before it gets here, and the first version of this function repeated
	 * that work under a comment claiming it fixed a bug it did not — the outline was fine; the INK was being
	 * read as the wrong thing. Repeated conventions with confident comments are how a wrong story survives. */
	for (NSUInteger i = r.location; i < r.location + r.length && i < (NSUInteger)run->glyph_count; i++) {
		ct_outline_ctx ctx;

		ctx.path = path;
		ctx.x = run->positions[i].x;
		ctx.y = run->positions[i].y;
		/* **THE FONT'S OWN MATRIX REACHES THIS ROUTE THROUGH THE DOOR'S MATRIX PARAMETER.** The ordinary
		 * path composes it into the context's TEXT matrix; the outline door takes it directly, which is what
		 * that parameter is for. Passing the identity here — as the first version did — made a font with a
		 * non-identity matrix draw correctly through the glyph doors and UNTRANSFORMED through the stroked one,
		 * the one asymmetry the drawing route had. */
		if (cg_font_glyph_outline(font->graphics, run->glyphs[i], size, font->matrix,
					  &ct_outline_sink_impl, &ctx)) {
			drawn++;
		}
	}
	if (drawn == 0) {
		CGPathRelease(path);
		return 0;	/* nothing outlined: fall back to the font's own drawing rather than drawing nothing */
	}
	{
		id color = (run->attributes != nil)
					? [run->attributes objectForKey:kCTStrokeColorAttributeName] : nil;

		if (color != nil && [color isKindOfClass:[NSObject class]]) {
			stroke_color = (CGColorRef)color;
			CGContextSetStrokeColorWithColor(context, stroke_color);
		}
	}
	CGContextSetLineWidth(context, (CGFloat)((width_pct < 0.0 ? -width_pct : width_pct) * size / 100.0));
	CGContextAddPath(context, (CGPathRef)path);
	if (width_pct > 0.0) {
		CGContextDrawPath(context, kCGPathStroke);	/* "stroke only" */
	} else {
		CGContextDrawPath(context, kCGPathFillStroke);	/* "stroke and fill" */
	}
	CGPathRelease(path);
	(void)stroke_color;
	return 1;
}

static void ct_draw_run(CTRunRef run, CGContextRef context, NSRange range)
{
	NSDictionary *attributes;
	CTFontRef font;
	NSRange r;
	long n;
	int stroked = 0;

	if (run == NULL || context == NULL) {
		return;
	}
	attributes = (NSDictionary *)run->attributes;
	font = (CTFontRef)[attributes objectForKey:kCTFontAttributeName];
	if (font == NULL) {
		return;	/* a run whose attributes name no font cannot be drawn — and a caller cannot have got here
			 * with one, because the LINE's creation refuses a string with no font (CTLine.h). */
	}
	/* **THE SAME EMPTY-RANGE RULE THE ACCESSORS FOLLOW** (a length of 0 means the whole run): a drawing door
	 * that drew nothing for the default range would be the one place in this tier where "the whole run" and
	 * "nothing" disagreed. */
	n = CTRunGetGlyphCount(run);
	r = (range.length == 0) ? NSMakeRange(0, (NSUInteger)n) : range;
	if (n <= 0 || r.location >= (NSUInteger)n || r.length == 0) {
		return;
	}
	if (r.location + r.length > (NSUInteger)n) {
		r.length = (NSUInteger)n - r.location;
	}
	/* **THE PEN.** Core Text's idiom is `CGContextSetTextPosition` and then `CTLineDraw`, so a line must land at
	 * the context's text position — and it did not, because the glyphs below go to
	 * `CGContextShowGlyphsAtPositions`, WHICH IS AN ABSOLUTE DOOR: it draws each glyph at the position it is
	 * given and never reads `c->state.text_position` (`CGContextShowGlyphs` is the door that takes its pen from
	 * there). The drawing probe measured it as two baselines ten points and eighty points apart landing in the
	 * SAME memory rows.
	 *
	 * **THE COMPOSITION IS A TRANSLATION, APPLIED AND TAKEN BACK.** A translate rather than an offset array,
	 * because the glyph renderer applies the context's CTM to its pen (`CGContext.c:4958`) — so one transform
	 * moves the glyphs AND the underline rule below, which is computed from the same run positions and would
	 * otherwise need the same offset twice in two different shapes. Taken back rather than saved, because a
	 * `CGContextSaveGState`/`RestoreGState` pair around the run ALSO restores THE FILL COLOUR AND LINE WIDTH, which
	 * this function sets for the run and which the rest of the drawing currently inherits: the first version did
	 * that and turned `coretext_stroke`'s "the 2x matrix scales the stroked route" check red, which is how a
	 * probe that depends on that inheritance announces itself. **The measurement decides the shape of the fix**,
	 * and the shape it chose is the narrowest one that moves the pen. */
	{
		CGPoint pen = CGContextGetTextPosition(context);

		CGContextTranslateCTM(context, pen.x, pen.y);
	}
	/* **AND THE TEXT MATRIX IS TOUCHED ONLY WHEN THE FONT'S MATRIX SAYS SOMETHING.** The caller's own text
	 * matrix is where the line is being drawn — its pen — so combining the font's matrix into it is right in
	 * general and WRONG BY DEFAULT here: every font this tier resolves has the IDENTITY matrix, and a call
	 * that reads the caller's matrix back and writes it again would also depend on what an unset text matrix
	 * reads as. So the identity case is left alone (a no-op either way, and the caller's placement survives
	 * untouched), and the composition happens only for a font that actually carries one. */
	if (font->matrix.a != 1.0 || font->matrix.b != 0.0 || font->matrix.c != 0.0 ||
	    font->matrix.d != 1.0 || font->matrix.tx != 0.0 || font->matrix.ty != 0.0) {
		CGContextSetTextMatrix(context,
				       CGAffineTransformConcat(font->matrix,
							       CGContextGetTextMatrix(context)));
	}
	/* **M6c: THE FOREGROUND COLOUR, WHICH IS AN OBJECT NOW.** Apple's key says the value "must be a
	 * CGColorRef" and that it is "used to fill the glyphs"; the companion BOOLEAN key says when to take the
	 * fill from the CONTEXT instead. Both are honoured here, and the order matters: the fill is set BEFORE
	 * the glyphs and before the underline below re-sets it, so each run leaves the state the next one
	 * expects rather than depending on what the last one did. */
	{
		id from_context = [attributes objectForKey:kCTForegroundColorFromContextAttributeName];
		CGColorRef color = (CGColorRef)[attributes objectForKey:kCTForegroundColorAttributeName];

		if (color != NULL && (from_context == nil || ![from_context boolValue])) {
			CGContextSetFillColorWithColor(context, color);
		}
	}
	/* **M6g: THE STROKE ROUTE.** A run whose `kCTStrokeWidthAttributeName` is not zero is drawn as an
	 * OUTLINE PATH rather than through the font's own glyph doors, because the era's sentence says the number
	 * "controls the text drawing mode" — and this function is where the mode is decided. A width of 0.0 (or no
	 * key at all) means no stroke, which is the default and leaves every earlier probe's drawing untouched. */
	{
		id stroke = (run->attributes != nil)
					? [run->attributes objectForKey:kCTStrokeWidthAttributeName] : nil;
		double stroke_width = (stroke != nil && [stroke respondsToSelector:@selector(doubleValue)])
						? [stroke doubleValue] : 0.0;

		if (stroke_width != 0.0 && ct_draw_stroked_run(run, context, r, stroke_width)) {
			stroked = 1;
		}
	}
	/* **AND A STROKED RUN STILL GETS ITS UNDERLINE.** This branch replaces the run's GLYPH DRAWING — the
	 * outline path is the drawing — but it must not replace the rest of the function: the underline below is
	 * the same rule over the same advances, and an earlier version returned from here, which meant the stroke
	 * and the underline keys could not both apply. That is a gap between two keys shipped in this file, and it
	 * is closed by skipping the two glyph doors rather than the whole remainder. */
	if (!stroked) {
		CGContextSetFont(context, font->graphics);
		CGContextSetFontSize(context, font->size);
		CGContextShowGlyphsAtPositions(context, run->glyphs + r.location,
					       run->positions + r.location, (size_t)r.length);
	}
	/* **M6a: THE UNDERLINE, AND ITS NUMBERS ARE THE FACE'S OWN** — `CTFontGetUnderlinePosition` is normally
	 * NEGATIVE (below the baseline) and `…Thickness` is the `post` table's, both already scaled to the point
	 * size (M1d). The rule is therefore drawn from the FONT rather than from a constant, which is what lets a
	 * probe check the ink's position against those two doors.
	 *
	 * `Single` is one rectangle of the face's thickness centred on the face's position; `Thick` is half again
	 * as thick; `Double` is two of them with a gap of one thickness. The PATTERNS are refused by name in the
	 * header rather than faked here. */
	{
		id style_value = [attributes objectForKey:kCTUnderlineStyleAttributeName];
		int value = (style_value != nil) ? [style_value intValue] : kCTUnderlineStyleNone;
		int style = value & 0x000F;
		int pattern = value & 0x0F00;

		if (style != kCTUnderlineStyleNone && pattern == kCTUnderlinePatternSolid) {
			/* AND THE RULE'S OWN COLOUR (M6c) — the fourth key, and the one that makes TWO colours visible
			 * in one drawing: the glyphs in the foreground colour and the rule in its own. With no colour
			 * key the rule takes the foreground's, which is what Apple's default is. */
			CGColorRef rule_color = (CGColorRef)[attributes objectForKey:kCTUnderlineColorAttributeName];

			if (rule_color != NULL) {
				CGContextSetFillColorWithColor(context, rule_color);
			}
			double position = CTFontGetUnderlinePosition(font);
			double thickness = CTFontGetUnderlineThickness(font);
			CGFloat x0 = run->positions[r.location].x;
			CGFloat x1 = run->positions[r.location + r.length - 1].x +
				     run->advances[r.location + r.length - 1].width;

			if (thickness <= 0.0) {
				thickness = 1.0;	/* a face with no `post` thickness still gets a line */
			}
			if (style == kCTUnderlineStyleDouble) {
				CGContextFillRect(context, CGRectMake(x0, (CGFloat)(position - thickness),
								      x1 - x0, (CGFloat)thickness));
				CGContextFillRect(context, CGRectMake(x0, (CGFloat)(position - thickness * 2.0),
								      x1 - x0, (CGFloat)thickness));
			} else {
				if (style == kCTUnderlineStyleThick) {
					thickness *= 1.5;
				}
				CGContextFillRect(context, CGRectMake(x0, (CGFloat)(position - thickness * 0.5),
								      x1 - x0, (CGFloat)thickness));
			}
		}
	}
	/* AND THE PEN IS GIVEN BACK, in the same shape it was taken: the inverse of the translate above. */
	{
		CGPoint pen = CGContextGetTextPosition(context);

		CGContextTranslateCTM(context, -pen.x, -pen.y);
	}
}

void CTRunDraw(CTRunRef run, CGContextRef context, NSRange range)
{
	ct_draw_run(run, context, range);
}

void CTLineDraw(CTLineRef line, CGContextRef context)
{
	NSUInteger i;
	NSUInteger count;

	if (line == NULL || context == NULL) {
		return;
	}
	/* ONE CALL PER RUN, EACH WITH ITS OWN FONT — which is what a line of several fonts needs, and what
	 * M2b-2's attribute split made possible. */
	count = [line->runs count];
	for (i = 0; i < count; i++) {
		ct_draw_run((CTRunRef)[line->runs objectAtIndex:i], context, NSMakeRange(0, 0));
	}
}

NSRange CTLineGetStringRange(CTLineRef line)
{
	NSRange range = NSMakeRange(0, 0);
	NSUInteger i;
	NSUInteger count;

	if (line == NULL) {
		return range;
	}
	count = [line->runs count];
	for (i = 0; i < count; i++) {
		NSRange run_range = CTRunGetStringRange((CTRunRef)[line->runs objectAtIndex:i]);

		if (run_range.length == 0) {
			continue;
		}
		if (range.length == 0) {
			range = run_range;
		} else {
			NSUInteger start = (run_range.location < range.location) ? run_range.location
										: range.location;
			NSUInteger end = (run_range.location + run_range.length >
					  range.location + range.length)
						 ? run_range.location + run_range.length
						 : range.location + range.length;

			range = NSMakeRange(start, end - start);
		}
	}
	return range;
}

CTLineRef CTLineRetain(CTLineRef line)
{
	return (CTLineRef)[(id)(uintptr_t)line retain];
}

void CTLineRelease(CTLineRef line)
{
	[(id)(uintptr_t)line release];
}

/* ====================================================================================================
 * M6m: THE LINE'S HIT-TESTING AND PLACEMENT DOORS.
 *
 * **THE INDEX UNIT IS BYTES, NOT UTF-16**, and that is this tier's recorded divergence (`CTRun.h`): every
 * glyph in a run carries the UTF-8 byte offset it came from (`string_indices`, piece-relative) and the run
 * carries where its piece began in the line (`string_start`), so a line byte offset is the sum of the two.
 *
 * The era's sentences, since they fix the semantics:
 *   * `CTLineGetStringIndexForPosition` "determines the string index for a mouse click ... the character
 *     before which the next character should be inserted", which is why a position past the end answers the
 *     line's END index rather than the last glyph's;
 *   * `CTLineGetOffsetForStringIndex` answers offsets "suitable for movement between adjacent lines or for
 *     drawing a custom caret" — the primary is the insertion point's LEADING edge and the secondary its
 *     TRAILING edge.
 * ================================================================================================== */

/* THE GLYPH THAT REPRESENTS A LINE BYTE OFFSET: the first whose own byte offset is at or past it. */
static const CTRun *ct_line_run_for_byte(CTLineRef line, long byte, NSUInteger *run_index, long *local)
{
	NSUInteger n = [(NSMutableArray *)line->runs count];

	for (NSUInteger r = 0; r < n; r++) {
		CTRun *run = (CTRun *)[(NSMutableArray *)line->runs objectAtIndex:r];

		if (byte >= run->string_start && byte < run->string_start + run->string_length) {
			for (long g = 0; g < run->glyph_count; g++) {
				if (run->string_start + run->string_indices[g] >= byte) {
					*run_index = r;
					*local = g;
					return run;
				}
			}
			*run_index = r;
			*local = run->glyph_count - 1;
			return run;
		}
	}
	/* PAST THE END (or an empty line): the last glyph of the last run, or nothing. */
	if (n > 0) {
		CTRun *run = (CTRun *)[(NSMutableArray *)line->runs objectAtIndex:n - 1];

		if (run->glyph_count > 0) {
			*run_index = n - 1;
			*local = run->glyph_count - 1;
			return run;
		}
	}
	*run_index = (NSUInteger)-1;
	*local = -1;
	return nil;
}

double CTLineGetOffsetForStringIndex(CTLineRef line, long charIndex, CGFloat *secondaryOffset)
{
	NSUInteger r = 0;
	long g = -1;
	const CTRun *run;
	double x, trailing;

	if (line == NULL) {
		if (secondaryOffset != NULL) {
			*secondaryOffset = 0.0;
		}
		return 0.0;
	}
	run = ct_line_run_for_byte(line, charIndex, &r, &g);
	if (run == nil) {
		if (secondaryOffset != NULL) {
			*secondaryOffset = 0.0;
		}
		return 0.0;
	}
	x = (double)run->positions[g].x;
	trailing = x + (double)run->advances[g].width;
	if (secondaryOffset != NULL) {
		*secondaryOffset = (CGFloat)trailing;
	}
	return x;
}

long CTLineGetStringIndexForPosition(CTLineRef line, CGPoint position)
{
	NSUInteger n;

	if (line == NULL) {
		return 0;
	}
	n = [(NSMutableArray *)line->runs count];
	for (NSUInteger r = 0; r < n; r++) {
		CTRun *run = (CTRun *)[(NSMutableArray *)line->runs objectAtIndex:r];

		for (long g = 0; g < run->glyph_count; g++) {
			double x0 = (double)run->positions[g].x;
			double x1 = x0 + (double)run->advances[g].width;

			if ((double)position.x >= x0 && (double)position.x < x1) {
				return run->string_start + run->string_indices[g];
			}
		}
	}
	/* "THE CHARACTER BEFORE WHICH THE NEXT CHARACTER SHOULD BE INSERTED": past the end that is the line's
	 * own end, which is what makes a click to the right of the text place a caret after it. */
	if (n > 0) {
		CTRun *run = (CTRun *)[(NSMutableArray *)line->runs objectAtIndex:n - 1];

		return run->string_start + run->string_length;
	}
	return 0;
}

double CTLineGetPenOffsetForFlush(CTLineRef line, CGFloat flushFactor, double flushWidth)
{
	if (line == NULL) {
		return 0.0;
	}
	/* The era's sentence: "a flushFactor of 0 or" 1 is left or right, half is centred — so the offset is
	 * simply the slack, distributed by the factor. */
	return (flushWidth - (double)line->width) * (double)flushFactor;
}

CGRect CTLineGetImageBounds(CTLineRef line, CGContextRef context)
{
	CGRect box;
	NSUInteger n;

	/* **THE CONTEXT IS ACCEPTED AND NOT CONSULTED**, and that is a stated divergence: Apple's door measures in
	 * the context's space, while this tier's glyph metrics are the font's own in the LINE's space (which is
	 * where `CTRunGetPositions` reports them). A caller that has scaled the context therefore gets the line's
	 * own coordinates, which is the space the line was built in. */
	(void)context;
	if (line == NULL) {
		return CGRectNull;
	}
	box = CGRectNull;
	n = [(NSMutableArray *)line->runs count];
	for (NSUInteger r = 0; r < n; r++) {
		CTRun *run = (CTRun *)[(NSMutableArray *)line->runs objectAtIndex:r];
		CTFontRef font = (CTFontRef)[run->attributes objectForKey:kCTFontAttributeName];
		CGRect run_ink;

		if (font == NULL) {
			continue;
		}
		run_ink = ct_ink_bounds(font, run->glyphs, run->positions, run->glyph_count, NSMakeRange(0, 0));
		if (!CGRectIsNull(run_ink)) {
			box = CGRectIsNull(box) ? run_ink : CGRectUnion(box, run_ink);
		}
	}
	return box;
}


/* -------------------------------------------------------------------------------------------------
 * M7c: the line reshaped to a measure — truncation and justification.
 * ---------------------------------------------------------------------------------------------- */

/* **A NEW RUN FROM A SLICE OF AN EXISTING ONE** — the one helper both doors need, and it lives here rather
 * than in `CTRun.m` because a `CTRun` is already built by direct ivar assignment in this file
 * (`ct_run_from_glyph`, `ct_shape_run`): a second factory in the class's own file would be a second place for
 * the two to disagree about what a run is.
 *
 * THE STRING RANGE IS DERIVED RATHER THAN GUESSED. `string_indices` are offsets RELATIVE to the run's
 * `string_start` — this file's own `ct_line_run_for_byte` reads them that way — so a slice's range begins at
 * the source's start plus the first cluster it keeps, and ends at the NEXT cluster it does NOT keep, or at the
 * source's own end when the slice runs to it. A slice that keeps the source's LAST glyph keeps its trailing
 * whitespace, because that is a per-run fact about the text AFTER the glyphs; one that stops short does not.
 *
 * AND THE TWO SHIFTS ARE WHAT MAKES ONE HELPER SERVE BOTH DOORS: `dx` moves the whole slice (`spread` is
 * zero — the truncation case), and `spread` adds a fixed amount to every glyph's advance and to every glyph's
 * position in proportion to how many glyphs precede it (the justification case). `spread_all` says whether
 * the slice's LAST glyph also grows — true for every run but the line's final one, which is what makes the
 * gaps BETWEEN runs part of the same arithmetic as the gaps inside them. */
static CTRun *ct_run_slice(CTRun *src, long first, long count, double dx, double spread, int spread_all)
{
	CTRun *run;
	long i;
	long begin, end;

	if (src == nil || count <= 0 || first < 0 || first + count > src->glyph_count) {
		return nil;
	}
	run = [[CTRun alloc] init];
	run->glyph_count = count;
	run->glyphs = malloc(sizeof(CGGlyph) * (size_t)count);
	run->positions = malloc(sizeof(CGPoint) * (size_t)count);
	run->advances = malloc(sizeof(CGSize) * (size_t)count);
	run->string_indices = malloc(sizeof(long) * (size_t)count);
	if (run->glyphs == NULL || run->positions == NULL || run->advances == NULL ||
	    run->string_indices == NULL) {
		[run release];	/* -dealloc frees whatever was allocated; free(NULL) is legal */
		return nil;
	}
	for (i = 0; i < count; i++) {
		run->glyphs[i] = src->glyphs[first + i];
		run->advances[i] = src->advances[first + i];
		run->string_indices[i] = src->string_indices[first + i];
		if (spread_all || i < count - 1) {
			run->advances[i].width += spread;
		}
		run->positions[i] = src->positions[first + i];
		run->positions[i].x += dx + spread * (double)i;
	}
	begin = src->string_indices[first];
	end = (first + count < src->glyph_count) ? src->string_indices[first + count] : src->string_length;
	run->string_start = src->string_start + begin;
	run->string_length = end - begin;
	run->trailing_whitespace = (first + count == src->glyph_count) ? src->trailing_whitespace : 0.0;
	run->attributes = [src->attributes retain];
	return run;
}

/* APPEND THE GLYPHS `[first, first + count)` OF `line`, IN LINE ORDER, TO `out` — re-based so the FIRST of
 * them lands on `*pen`, and leaving `*pen` at the x the NEXT piece should start at. THE SHIFT IS COMPUTED PER
 * RUN from that run's own recorded position rather than from a running sum, so a line whose positions are not
 * a plain accumulation (a single-glyph `kCTGlyphInfoAttributeName` override, which the shaper does not
 * position) is carried faithfully instead of being re-derived wrongly. */
static void ct_append_glyph_range(NSMutableArray *out, CTLine *line, long first, long count, double *pen)
{
	long base = 0;
	NSUInteger r;

	for (r = 0; r < [line->runs count] && count > 0; r++) {
		CTRun *run = [line->runs objectAtIndex:r];
		long run_end = base + run->glyph_count;
		long lo = (first > base) ? first : base;
		long hi = (first + count < run_end) ? first + count : run_end;

		if (hi > lo) {
			CTRun *piece = ct_run_slice(run, lo - base, hi - lo,
						    *pen - run->positions[lo - base].x, 0.0, 0);
			long g;

			if (piece != nil) {
				[out addObject:piece];
				for (g = 0; g < piece->glyph_count; g++) {
					*pen += piece->advances[g].width;
				}
				[piece release];	/* the array holds it */
			}
		}
		base = run_end;
	}
}

/* A LINE FROM A LIST OF RUNS: the width is the advance sum and the glyph count the sum of the runs', which is
 * how `CTLineCreateWithAttributedString` computes both — so a derived line is the same KIND of object as an
 * original one. THE VERTICAL METRICS ARE THE SOURCE'S, because neither operation moves a glyph in y. */
static CTLine *ct_line_alloc_from_runs(NSArray *runs, const CTLine *metrics_from)
{
	CTLine *line = [[CTLine alloc] init];
	NSUInteger r;
	long g;

	line->runs = [runs retain];
	line->glyph_count = 0;
	line->width = 0.0;
	line->ascent = metrics_from->ascent;
	line->descent = metrics_from->descent;
	line->leading = metrics_from->leading;
	line->trailing_whitespace = 0.0;
	for (r = 0; r < [runs count]; r++) {
		CTRun *run = [runs objectAtIndex:r];

		line->glyph_count += run->glyph_count;
		for (g = 0; g < run->glyph_count; g++) {
			line->width += run->advances[g].width;
		}
		line->trailing_whitespace = run->trailing_whitespace;
	}
	return line;
}

/* THE LINE'S GLYPHS IN LINE ORDER as `(run, local)` pairs, and the advance of each — the flat view the two
 * truncation cuts are measured against. `malloc`ed, and the caller frees. */
static int ct_line_flatten(CTLine *line, long **runs_out, long **locals_out)
{
	long *runs = malloc(sizeof(long) * (size_t)(line->glyph_count > 0 ? line->glyph_count : 1));
	long *locals = malloc(sizeof(long) * (size_t)(line->glyph_count > 0 ? line->glyph_count : 1));
	long n = 0;
	NSUInteger r;

	if (runs == NULL || locals == NULL) {
		free(runs);
		free(locals);
		return 0;
	}
	for (r = 0; r < [line->runs count]; r++) {
		CTRun *run = [line->runs objectAtIndex:r];
		long g;

		for (g = 0; g < run->glyph_count; g++) {
			runs[n] = (long)r;
			locals[n] = g;
			n++;
		}
	}
	*runs_out = runs;
	*locals_out = locals;
	return 1;
}

static double ct_glyph_advance(CTLine *line, const long *runs, const long *locals, long i)
{
	CTRun *run = [line->runs objectAtIndex:(NSUInteger)runs[i]];

	return run->advances[locals[i]].width;
}

/* **THE LONGEST SUFFIX OF AT MOST `limit` THAT FITS `budget`** — the mirror of the prefix walk, and the reason
 * the Start type needs no separate arithmetic. Both return a COUNT of glyphs, counting 0 for "none". */
static long ct_prefix_fit(CTLine *line, const long *runs, const long *locals, double budget)
{
	double used = 0.0;
	long n = line->glyph_count;
	long i;

	for (i = 0; i < n; i++) {
		double w = ct_glyph_advance(line, runs, locals, i);

		if (used + w > budget) {
			return i;
		}
		used += w;
	}
	return n;
}

static long ct_suffix_fit(CTLine *line, const long *runs, const long *locals, double budget)
{
	double used = 0.0;
	long n = line->glyph_count;
	long i;

	for (i = n - 1; i >= 0; i--) {
		double w = ct_glyph_advance(line, runs, locals, i);

		if (used + w > budget) {
			return n - 1 - i;
		}
		used += w;
	}
	return n;
}

/* **A TRUNCATED LINE, AND THE TWO ANSWERS THE ERA DOES NOT SETTLE ARE HERE.** `width` is the width
 * truncation begins at: a line that is not wider than it needs NO truncation, and this tier answers the line
 * ITSELF (retained) rather than NULL, because "this line, at most this wide" is the question the caller asked
 * and the line is a true answer to it. And the cut falls on a GLYPH boundary, which is what a shaped line
 * offers. A token whose own width leaves no room answers NULL — Apple's own sentence — which is also what a
 * NULL token plus a zero-or-negative budget would be, so the two collapse into one check. */
CTLineRef CTLineCreateTruncatedLine(CTLineRef line, double width, CTLineTruncationType truncationType,
				    CTLineRef truncationToken)
{
	CTLine *src = (CTLine *)line;
	CTLine *token = (CTLine *)truncationToken;
	NSMutableArray *out;
	double budget, token_width, pen;
	long *runs = NULL, *locals = NULL;
	long keep_start, keep_end;

	if (src == NULL) {
		return NULL;
	}
	if (src->glyph_count == 0 || (double)src->width <= width) {
		return (CTLineRef)[src retain];	/* no truncation needed, and that is a success */
	}
	token_width = (token != NULL) ? (double)token->width : 0.0;
	budget = width - token_width;
	if (budget <= 0.0) {
		return NULL;	/* the token alone does not fit the width asked for */
	}
	if (!ct_line_flatten(src, &runs, &locals)) {
		return NULL;
	}
	keep_start = 0;
	keep_end = 0;
	switch (truncationType) {
	case kCTLineTruncationStart:
		/* THE BEGINNING GOES; THE END STAYS. */
		keep_end = ct_suffix_fit(src, runs, locals, budget);
		break;
	case kCTLineTruncationMiddle:
		/* BOTH ENDS STAY AND THE MIDDLE GOES, and the budget is SPLIT RATHER THAN SHARED: the start half
		 * takes what it can of HALF the budget and the end half takes the REST, so the two halves cannot
		 * disagree about who got the slack and every glyph comes from one end or the other. */
		keep_start = ct_prefix_fit(src, runs, locals, budget / 2.0);
		{
			double taken = 0.0;
			long i;

			for (i = 0; i < keep_start; i++) {
				taken += ct_glyph_advance(src, runs, locals, i);
			}
			keep_end = ct_suffix_fit(src, runs, locals, budget - taken);
		}
		break;
	case kCTLineTruncationEnd:
	default:
		/* THE END GOES; THE BEGINNING STAYS. */
		keep_start = ct_prefix_fit(src, runs, locals, budget);
		break;
	}
	out = [NSMutableArray array];
	pen = 0.0;
	if (keep_start > 0) {
		ct_append_glyph_range(out, src, 0, keep_start, &pen);
	}
	if (token != NULL && token->glyph_count > 0) {
		/* **AND THE TOKEN IS A LINE TOO**, so it is appended by the SAME helper that appends a piece of
		 * the source: a token with one run or four is carried faithfully, and the token's own origin is
		 * re-based onto the pen exactly as a source piece's is. */
		ct_append_glyph_range(out, token, 0, token->glyph_count, &pen);
	}
	if (keep_end > 0) {
		ct_append_glyph_range(out, src, src->glyph_count - keep_end, keep_end, &pen);
	}
	free(runs);
	free(locals);
	if ([out count] == 0) {
		return NULL;	/* nothing survived, which is not a line */
	}
	return (CTLineRef)ct_line_alloc_from_runs(out, src);
}

/* **A JUSTIFIED LINE, AND THE MECHANISM IS OURS AS THE HEADER SAYS.** Apple fixes the CONTRACT — 1.0 or more
 * is full justification, less is partial, 0 or less is none, and a target under the line's own width squishes
 * it — and not where the slack is placed. This tier places it in the GAPS, which is what "per-glyph spacing"
 * means: the line's glyphs are the same glyphs with the same glyph ids and the same ORDER, and the only thing
 * that changes is how far the pen moves between them.
 *
 * SO THE SLACK GOES INTO THE ADVANCES, one gap's worth per consecutive pair, and the positions follow from the
 * same arithmetic — which is what makes `CTLineGetTypographicBounds` on the result EXACTLY the target rather
 * than a number kept beside it. A run that has glyphs AFTER it grows its last glyph's advance too, because the
 * gap between two runs is a gap like any other; the line's final glyph does not, because there is nothing after
 * it to space away from. That is why the growth test is "are there glyphs after this run" rather than "is this
 * the last run" — an empty run at the end must not move the boundary.
 *
 * AND A LINE OF ONE GLYPH HAS NO GAP, so it is answered as itself, retained — the degenerate case stated
 * rather than a special path, exactly as for a factor that asks for none. */
CTLineRef CTLineCreateJustifiedLine(CTLineRef line, CGFloat justificationFactor, double justificationWidth)
{
	CTLine *src = (CTLine *)line;
	NSMutableArray *out;
	double target, spread;
	long n, base;
	NSUInteger r;

	if (src == NULL) {
		return NULL;
	}
	if (src->glyph_count <= 1 || (double)justificationFactor <= 0.0) {
		return (CTLineRef)[src retain];	/* nothing to justify: no gap, or no justification asked for */
	}
	if ((double)justificationFactor >= 1.0) {
		target = justificationWidth;
	} else {
		target = (double)src->width +
			 (double)justificationFactor * (justificationWidth - (double)src->width);
	}
	n = src->glyph_count;
	spread = (target - (double)src->width) / (double)(n - 1);
	out = [NSMutableArray array];
	base = 0;
	for (r = 0; r < [src->runs count]; r++) {
		CTRun *run = [src->runs objectAtIndex:r];
		CTRun *piece;

		if (run->glyph_count <= 0) {
			continue;
		}
		/* THE SHIFT FOR A GLYPH IS ITS INDEX TIMES THE GAP, so a run's own `dx` is its first glyph's
		 * index times it — and `ct_run_slice` adds the rest within the run. */
		piece = ct_run_slice(run, 0, run->glyph_count, (double)base * spread, spread,
				     (base + run->glyph_count < n));
		if (piece == nil) {
			continue;
		}
		[out addObject:piece];
		[piece release];
		base += run->glyph_count;
	}
	if ([out count] == 0) {
		return NULL;
	}
	return (CTLineRef)ct_line_alloc_from_runs(out, src);
}
