/*
 * CTFont.m — the font class: the object, its metrics, the character-to-glyph step, its names, and its
 * coverage.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * ONE FILE BECAUSE THE OBJECT IS ONE THING, and this merge is `dec-35c114bf75c7c2d7`'s second half
 * (CT-M1e-b): a `CTFont` is a REAL OBJECTIVE-C OBJECT now, exactly as `CTFontDescriptor` became one in
 * CT-M1e-a — so the arithmetic that used to live in a C file reading a C struct is here, reading its own
 * ivars, and the C API is untouched (`CTFontRef` points at this class, every door keeps its signature).
 *
 * AND IT SUBCLASSES `NSObject` FOR THE REASON CT-M1e-a RECORDS: this library already links Foundation, and
 * the alternative — a hand-written root class — re-implements about fifteen methods the runtime has.
 */
#import <Foundation/Foundation.h>

#include <CoreText/CTFont.h>

/* THE OBJECT IS DECLARED ONCE, IN THE SEAM THE OBJECTIVE-C HALF ALSO INCLUDES: `CTFont.m`'s name doors
 * have to reach the `CGFontRef` this file built, and a struct declared twice is a struct that can
 * disagree with itself. */
#include "CTFont_internal.h"
#include "CTFontDescriptor_internal.h"
/* §41: trace every call to its header — and this one belongs HERE rather than in CTFont.h, whose
 * own mask constant is declared after the point an include there would sit. */
#include <CoreText/CTFontTraits.h>
#include "CTInk_internal.h"

#include <CoreGraphics/CGFont_internal.h>

#include <ft2build.h>
#include FT_FREETYPE_H
/* THE SFNT TABLE HEADER IS HERE FOR ONE FIELD AND ONE CALL: the `post` table's `italicAngle` and the
 * `FT_Get_Sfnt_Table` that fetches it, which `CTFontGetSlantAngle` below reads. */
#include FT_TRUETYPE_TABLES_H
/* **AND THE MULTIPLE-MASTERS HEADER, FOR `FT_Get_MM_Var`** (CT-M7m): the face's own `fvar` data is one call
 * away, which is why `CTFontCopyVariationAxes` is a door rather than a table parser — the ENGINE already reads
 * this table, unlike `feat` beside it. */
#include FT_MULTIPLE_MASTERS_H

#include <math.h>
#include <stdlib.h>

/* APPLE'S DEFAULT POINT SIZE, QUOTED RATHER THAN CHOSEN: the era's own doc block for the create doors
 * says "If 0.0 is specified, the default font size of 12.0 will be used". A NEGATIVE size is treated
 * the same way — Apple's modern page says a negative size is "interpreted as the font's default size"
 * — and that reading is stated here because the 10.6 block in front of me names only 0.0. */
#define CT_DEFAULT_FONT_SIZE 12.0

/* THE REFERENCE COUNT, WHICH IS THIS TIER'S DOOR AND NOT APPLE'S ... AND IT IS NOW A WRAPPER: the count
 * is the runtime's, and `-dealloc` is the one place the graphics font is released. */
CTFontRef CTFontRetain(CTFontRef font)
{
	return (CTFontRef)[(id)(uintptr_t)font retain];
}

void CTFontRelease(CTFontRef font)
{
	[(id)(uintptr_t)font release];
}

CTFontRef CTFontCreateWithGraphicsFont(CGFontRef graphicsFont, CGFloat size, const CGAffineTransform *matrix,
				       CTFontDescriptorRef attributes)
{
	CTFont *f;

	/* A FONT OF NO FONT IS NOT A FONT. Apple's create doors promise "a best match font will always be
	 * returned"; this tier's registry answers NULL for a name it cannot resolve (CGFontCreateWithFontName
	 * says so in its own comment), so the NULL is handled here rather than handed to a caller who would
	 * discover it at the first metric call. */
	if (graphicsFont == NULL) {
		return NULL;
	}
	f = [[CTFont alloc] init];
	if (f == nil) {
		return NULL;
	}
	f->graphics = CGFontRetain(graphicsFont);
	f->size = (size > 0) ? size : CT_DEFAULT_FONT_SIZE;
	/* THE MATRIX IS OPTIONAL, WHICH IS THE ERA'S OWN WORD ("If unspecified, the identity matrix will be
	 * used. Optional.") */
	f->matrix = (matrix != NULL) ? *matrix : CGAffineTransformIdentity;
	/* APPKIT-REFUSE: Apple documents the `attributes` descriptor as overriding "any specified in the
	 * graphics font's descriptor, unless they are unspecified". CT-M1a ships NO WAY TO BUILD A DESCRIPTOR
	 * (the class is its own unit, and every one of its ledger rows is open), so a non-NULL one is
	 * accepted and IGNORED — named here because a silently ignored parameter is the kind of thing this
	 * plan refuses to leave for a caller to find. */
	(void)attributes;
	return (CTFontRef)f;
}

/* ---- AND THE SCALING, WHICH IS WHAT A `CTFont` ADDS TO A `CGFont` ------------------------------------------ */

/* A `CGFont` answers everything in FONT UNITS; a `CTFont` answers in POINTS, "scaled based on the point
 * size and matrix of the font reference" (the era's sentence for every metric door below). The reading
 * this tier takes is at the head of CTFont.h: size / unitsPerEm, times the matrix's y factor (`d`) for
 * a VERTICAL metric and its x factor (`a`) for a HORIZONTAL one — exact for the identity and for a pure
 * scale, which is every matrix this tier can currently receive, and a stated approximation otherwise. */
static double ct_units(CTFontRef font)
{
	int em = CGFontGetUnitsPerEm(font->graphics);

	return (em > 0) ? (double)em : 1000.0;	/* a face with no em is not a face; 1000 is the PostScript one */
}

static double ct_vscale(CTFontRef font)
{
	return font->size / ct_units(font) * font->matrix.d;
}

static double ct_hscale(CTFontRef font)
{
	return font->size / ct_units(font) * font->matrix.a;
}

CGFloat CTFontGetSize(CTFontRef font)
{
	return (font != NULL) ? font->size : 0.0;
}

CGAffineTransform CTFontGetMatrix(CTFontRef font)
{
	return (font != NULL) ? font->matrix : CGAffineTransformIdentity;
}

CGFloat CTFontGetAscent(CTFontRef font)
{
	return (font != NULL) ? CGFontGetAscent(font->graphics) * ct_vscale(font) : 0.0;
}

CGFloat CTFontGetDescent(CTFontRef font)
{
	/* APPLE'S SIGN IS ITS OWN: Core Graphics answers the descent POSITIVE and so does this, because the
	 * CTFontGetDescent doc block says only "the scaled font descent metric" and the substrate's value is
	 * the one a caller already knows. */
	return (font != NULL) ? CGFontGetDescent(font->graphics) * ct_vscale(font) : 0.0;
}

CGFloat CTFontGetLeading(CTFontRef font)
{
	return (font != NULL) ? CGFontGetLeading(font->graphics) * ct_vscale(font) : 0.0;
}

CGFloat CTFontGetCapHeight(CTFontRef font)
{
	return (font != NULL) ? CGFontGetCapHeight(font->graphics) * ct_vscale(font) : 0.0;
}

CGFloat CTFontGetXHeight(CTFontRef font)
{
	return (font != NULL) ? CGFontGetXHeight(font->graphics) * ct_vscale(font) : 0.0;
}

CGRect CTFontGetBoundingBox(CTFontRef font)
{
	CGRect box;

	if (font == NULL) {
		return CGRectMake(0, 0, 0, 0);
	}
	box = CGFontGetFontBBox(font->graphics);
	return CGRectMake(box.origin.x * ct_hscale(font), box.origin.y * ct_vscale(font),
			  box.size.width * ct_hscale(font), box.size.height * ct_vscale(font));
}

unsigned CTFontGetUnitsPerEm(CTFontRef font)
{
	int em;

	if (font == NULL) {
		return 0;
	}
	em = CGFontGetUnitsPerEm(font->graphics);
	return (em > 0) ? (unsigned)em : 0;
}

long CTFontGetGlyphCount(CTFontRef font)
{
	return (font != NULL) ? (long)CGFontGetNumberOfGlyphs(font->graphics) : 0;
}

/* -------------------------------------------------------------------------------------------------
 * M7f: the OS/2 table, and the three trait numbers Apple's sentences describe
 * ---------------------------------------------------------------------------------------------- */

/* **THE OS/2 TABLE, READ ONCE FOR BOTH DOORS BELOW** — `CTFontGetSymbolicTraits` needs its width class and
 * family class, and `CTFontCopyTraits` needs the weight and width classes. A face with no OS/2 table (a bitmap
 * face) answers NULL, and every caller below treats that as "no recorded value" rather than substituting one. */
static TT_OS2 *ct_os2(CTFontRef font)
{
	FT_Face face = (FT_Face)cg_font_face(font->graphics);

	return (face != NULL) ? (TT_OS2 *)FT_Get_Sfnt_Table(face, ft_sfnt_os2) : NULL;
}

/* **THE WEIGHT AND WIDTH SCALES ARE THIS TIER'S, AND THE ERA'S SENTENCES ARE WHAT ANCHOR THEM.** Apple fixes the
 * RANGE and the ZERO and not the scale: weight is "a float value between -1.0 and 1.0 ... 0.0 corresponds to the
 * regular or medium font weight", and width likewise with "0.0 corresponds to regular glyph spacing while
 * negative values represent condensed". So the anchors are the regular values from OS/2's own vocabulary — 400
 * for weight, 5 for width, both the OpenType table's definition of regular — and mapping those to 0.0 while
 * BOTH ends of the range land on +/-1 fixes every coefficient. That is the same shape as the superscript rise
 * (`CT_SUPERSCRIPT_FRACTION`): **a rule, not a table**, and stated as this tier's because the era publishes none.
 *
 * A face with no OS/2 table answers 0.0 for both — the neutral value, which is what "no recorded weight" means
 * once the scale's zero IS regular. */
static double ct_clamp_unit(double v)
{
	if (v < -1.0) {
		return -1.0;
	}
	return (v > 1.0) ? 1.0 : v;
}

static double ct_normalized_weight(unsigned int usWeightClass)
{
	if (usWeightClass == 0) {
		return 0.0;
	}
	/* OS/2's weight range is 100..900 with 400 regular, so the two sides have different spans and BOTH ends
	 * must land on +/-1: 300 below the anchor, 500 above it. */
	return ct_clamp_unit((usWeightClass <= 400u)
				     ? ((double)usWeightClass - 400.0) / 300.0
				     : ((double)usWeightClass - 400.0) / 500.0);
}

static double ct_normalized_width(unsigned int usWidthClass)
{
	if (usWidthClass == 0) {
		return 0.0;
	}
	/* OS/2's width class is 1..9 with 5 regular, so the span is four either way. */
	return ct_clamp_unit(((double)usWidthClass - 5.0) / 4.0);
}

CTFontSymbolicTraits CTFontGetSymbolicTraits(CTFontRef font)
{
	FT_Face face;
	CTFontSymbolicTraits traits = 0;

	if (font == NULL) {
		return 0;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return 0;
	}
	/* WHAT THE FACE KNOWS, AND ONLY THAT. FreeType's own style and face flags answer four of Apple's
	 * bits directly — the face was loaded bold, italic, fixed-pitch or vertical — and the probe asserts
	 * exactly these four.
	 *
	 * **AND THE OTHER THREE ARE NO LONGER REFUSED** (M7f): this function used to answer ZERO for
	 * `kCTFontExpandedTrait`, `kCTFontCondensedTrait` and the four-bit `kCTFontClassMaskTrait` class with a
	 * recorded refusal — "THIS UNIT DOES NOT READ THE OS/2 TABLE", and the note that "the unit that reads OS/2
	 * ... is the one that closes them". That unit is this one, and what it does NOT have is the claim it used
	 * to make: the width class and the family class come from a table this file now opens, so they are DERIVED
	 * rather than guessed, which is the whole of what the refusal was protecting. **THE NON-REGULAR BRANCH HAS
	 * NO INPUT IN THIS TREE** — both shipped faces are width class 5 and family class 0, so the probe checks
	 * that a regular, unclassified face sets NEITHER expanded NOR condensed and no class bits, which is a
	 * check that a door defaulting to either WOULD fail. */
	if (face->style_flags & FT_STYLE_FLAG_ITALIC) {
		traits |= kCTFontItalicTrait;
	}
	if (face->style_flags & FT_STYLE_FLAG_BOLD) {
		traits |= kCTFontBoldTrait;
	}
	if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) {
		traits |= kCTFontMonoSpaceTrait;
	}
	if (face->face_flags & FT_FACE_FLAG_VERTICAL) {
		traits |= kCTFontVerticalTrait;
	}
	{
		TT_OS2 *os2 = ct_os2(font);

		if (os2 != NULL) {
			if (os2->usWidthClass != 0 && os2->usWidthClass < 5) {
				traits |= kCTFontCondensedTrait;
			} else if (os2->usWidthClass > 5) {
				traits |= kCTFontExpandedTrait;
			}
			/* THE CLASS IS THE HIGH BYTE OF `sFamilyClass`, RESERVED IN THE TOP NIBBLE BY
			 * `kCTFontClassMaskShift` — the four bits Apple's class constants are shifted into. */
			traits |= (CTFontSymbolicTraits)
				  ((((unsigned)(os2->sFamilyClass >> 8)) & 0x0Fu) << kCTFontClassMaskShift);
		}
	}
	return traits;
}

bool CTFontGetGlyphsForCharacters(CTFontRef font, const unsigned short characters[], CGGlyph glyphs[],
				  long count)
{
	FT_Face face;
	long i = 0;
	bool all = true;

	if (font == NULL || characters == NULL || glyphs == NULL || count <= 0) {
		return false;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return false;
	}
	while (i < count) {
		FT_ULong code = characters[i];

		/* **A SURROGATE PAIR IS ONE CHARACTER, AND APPLE SAYS SO IN THE DOOR'S OWN DOC BLOCK**: "Non-BMP
		 * characters must be encoded as surrogate pairs", and "Glyphs for non-BMP characters are sparse:
		 * the first glyph corresponds to the full character and the second glyph will be 0." So the pair
		 * is COMBINED for the lookup and the second slot is ZERO rather than the low surrogate's own
		 * (nonexistent) glyph. */
		if (code >= 0xD800 && code <= 0xDBFF && i + 1 < count &&
		    characters[i + 1] >= 0xDC00 && characters[i + 1] <= 0xDFFF) {
			FT_ULong low = characters[i + 1];
			FT_ULong cp = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);

			glyphs[i] = (CGGlyph)FT_Get_Char_Index(face, cp);
			glyphs[i + 1] = 0;
			if (glyphs[i] == 0) {
				all = false;
			}
			i += 2;
			continue;
		}
		/* AND A CHARACTER THIS FONT HAS NO GLYPH FOR IS GLYPH 0 — which is what `FT_Get_Char_Index`
		 * answers, and what the doc block promises ("glyphs for unmapped characters will be 0"). */
		glyphs[i] = (CGGlyph)FT_Get_Char_Index(face, code);
		if (glyphs[i] == 0) {
			all = false;
		}
		i++;
	}
	return all;
}

double CTFontGetAdvancesForGlyphs(CTFontRef font, CTFontOrientation orientation, const CGGlyph glyphs[],
				  CGSize advances[], long count)
{
	int *units;
	double total = 0.0;
	double scale;
	long i;

	if (font == NULL || glyphs == NULL || count <= 0) {
		return 0.0;
	}
	/* APPKIT-REFUSE: A VERTICAL ADVANCE IS REFUSED BY NAME AND ANSWERS 0. Apple's own sentence for this
	 * door says the orientation is "used to determine which glyph metrics to return", and the vertical
	 * ones live in the font's vertical metrics table — a table this tier has no path to at all (the
	 * engine's vertical layout is not used anywhere in this tree, and `kCTFontVerticalTrait` is reported
	 * from the face's flags without any vertical metric being read). Answering the HORIZONTAL value for
	 * a vertical request would be a wrong number rather than a missing one. */
	if (orientation == kCTFontVerticalOrientation) {
		return 0.0;
	}
	units = (int *)calloc((size_t)count, sizeof(int));
	if (units == NULL) {
		return 0.0;
	}
	/* "the ideal metrics for each glyph scaled and transformed in font space", from the substrate's own
	 * advances: `CGFontGetGlyphAdvances` answers font units, and the scale is the one at the head of
	 * CTFont.h (x, because an advance is horizontal — including for `kCTFontDefaultOrientation`, which
	 * Apple defines as "the font's intended orientation" and which every face this tier can load has as
	 * horizontal). */
	if (!CGFontGetGlyphAdvances(font->graphics, glyphs, (size_t)count, units)) {
		free(units);
		return 0.0;
	}
	scale = ct_hscale(font);
	for (i = 0; i < count; i++) {
		double a = units[i] * scale;

		total += a;
		if (advances != NULL) {
			advances[i] = CGSizeMake(a, 0.0);
		}
	}
	free(units);
	return total;
}

/* =================================================================================================
 * CT-M3: THE GRAPHICS FONT — the one door the drawing needs, and the bridge from an object a caller holds
 * to the thing `CGContextSetFont` takes.
 * =============================================================================================== */
CGFontRef CTFontCopyGraphicsFont(CTFontRef font, CTFontDescriptorRef *attributes)
{
	if (attributes != NULL) {
		/* **APPLE'S OWN SENTENCE**: "additional attributes from the font will be passed back as a font
		 * descriptor via the attributes parameter". A descriptor is always buildable here — the font's own
		 * name, size and matrix — so a caller that asks for one always gets one, and it is +1 like every
		 * other Copy door. */
		*attributes = CTFontCopyFontDescriptor(font);
	}
	/* AND THE GRAPHICS FONT IS THE FONT'S OWN, RETAINED: a `CTFont` holds exactly one `CGFontRef` for its
	 * whole life, which is why this is a retain rather than a copy to keep in step. */
	return (font != NULL) ? CGFontRetain(font->graphics) : NULL;
}

/* =================================================================================================
 * CT-M1c: THE DESCRIPTOR'S THREE DOORS ON THIS CLASS.
 *
 * The descriptor itself is `CTFontDescriptor.m`'s (it is a dictionary, and dictionaries are
 * Objective-C); what lives here is THE MAKING OF A FONT FROM ONE, which is C arithmetic and an engine
 * lookup — the same split the rest of this library uses. The three questions to ask a descriptor are the
 * three things a font is made of, and `CTFontDescriptor_internal.h` asks them BY NAME.
 * =============================================================================================== */

CTFontRef CTFontCreateWithFontDescriptor(CTFontDescriptorRef descriptor, CGFloat size,
					 const CGAffineTransform *matrix)
{
	char name[512];
	CGAffineTransform stored;
	CGFontRef graphics;
	CTFontRef font;

	if (descriptor == NULL) {
		return NULL;
	}
	/* **THE NAME IS THE ONE ATTRIBUTE THE ENGINE CAN MATCH**, and the refusal for the others is stated in
	 * `CTFont.h` rather than silently returning a wrong font: a descriptor whose family, traits, variation
	 * or cascade list is set but whose `kCTFontNameAttribute` is not answers NULL here, because the
	 * matcher that would use those is the font-collection unit's. */
	if (ctf_descriptor_name_utf8(descriptor, name, sizeof name) <= 0) {
		return NULL;
	}
	graphics = cg_font_create_with_name(name);
	if (graphics == NULL) {
		return NULL;	/* the registry's own rule: no substitution for a name that resolves to nothing */
	}
	/* **"THE SIZE AND MATRIX PARAMETERS WILL OVERRIDE ANY SPECIFIED IN THE FONT DESCRIPTOR, UNLESS THEY
	 * ARE UNSPECIFIED"** (Apple's sentence). Unspecified size is 0.0 — the same 0.0 the create doors read
	 * as "use the default", so a descriptor with no size and no argument lands on Apple's documented 12.0
	 * without a second rule. Unspecified matrix is NULL. */
	if (size <= 0.0) {
		(void)ctf_descriptor_size(descriptor, &size);
	}
	if (matrix == NULL && ctf_descriptor_matrix(descriptor, &stored)) {
		matrix = &stored;
	}
	font = CTFontCreateWithGraphicsFont(graphics, size, matrix, descriptor);
	CGFontRelease(graphics);
	return font;
}

CTFontDescriptorRef CTFontCopyFontDescriptor(CTFontRef font)
{
	if (font == NULL) {
		return NULL;
	}
	/* THE DESCRIPTOR A FONT ANSWERS FOR ITSELF IS BUILT FROM THE FONT — its PostScript name, its size and
	 * its matrix — which is the round trip Apple's own door describes: descriptor -> font -> descriptor.
	 * The two dictionary-valued attributes a MATCHED font would also carry (traits, variation) are not
	 * here, because this tier never matched anything to get this font. */
	return ctf_descriptor_from_font(font);
}

CTFontRef CTFontCreateCopyWithAttributes(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
					 CTFontDescriptorRef attributes)
{
	char name[512];
	CGAffineTransform m;
	CGFloat s;
	CGFontRef graphics;
	CTFontRef copy;

	if (font == NULL) {
		return NULL;
	}
	/* THE SAME "OVERRIDE UNLESS UNSPECIFIED" RULE AS THE CREATE DOOR ABOVE, one level further out: the
	 * ARGUMENTS override the attribute descriptor, and the ATTRIBUTE DESCRIPTOR overrides the font being
	 * copied — so an unspecified size falls back to the descriptor's and then to the font's, which is what
	 * Apple's door means by copying "the original" with attributes applied. */
	s = size;
	if (s <= 0.0 && !(attributes != NULL && ctf_descriptor_size(attributes, &s))) {
		s = CTFontGetSize(font);
	}
	if (matrix != NULL) {
		m = *matrix;
	} else if (!(attributes != NULL && ctf_descriptor_matrix(attributes, &m))) {
		m = CTFontGetMatrix(font);
	}
	/* AND THE FACE IS THE FONT'S OWN UNLESS THE ATTRIBUTE DESCRIPTOR NAMES ANOTHER ONE — which is what
	 * makes this door a copy-with-changes rather than a clone. */
	if (attributes != NULL && ctf_descriptor_name_utf8(attributes, name, sizeof name) > 0) {
		graphics = cg_font_create_with_name(name);
		if (graphics == NULL) {
			return NULL;
		}
		copy = CTFontCreateWithGraphicsFont(graphics, s, &m, attributes);
		CGFontRelease(graphics);
	} else {
		copy = CTFontCreateWithGraphicsFont(font->graphics, s, &m, attributes);
	}
	return copy;
}

/* =================================================================================================
 * CT-M1d: THE FONT'S OWN NUMBERS.
 *
 * Both underline metrics and the slant angle are the FACE'S, scaled by the same rule every metric above
 * uses — and the face is reached through `cg_font_face`, so nothing here opens a file.
 * =============================================================================================== */

CGFloat CTFontGetUnderlinePosition(CTFontRef font)
{
	FT_Face face;

	if (font == NULL) {
		return 0.0;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return 0.0;
	}
	/* THE FACE'S OWN VALUE, IN FONT UNITS, SCALED — and A FACE WITH NO `post` TABLE ANSWERS ZERO, which
	 * is FreeType's own value for "no such metric" rather than a substitute this tier invents. **THE SIGN
	 * IS PASSED THROUGH**: an underline sits BELOW the baseline, so the number is normally negative, and
	 * flipping it to make it "look positive" would disagree with the substrate every other caller reads. */
	return face->underline_position * ct_vscale(font);
}

CGFloat CTFontGetUnderlineThickness(CTFontRef font)
{
	FT_Face face;

	if (font == NULL) {
		return 0.0;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return 0.0;
	}
	return face->underline_thickness * ct_vscale(font);
}

CGFloat CTFontGetSlantAngle(CTFontRef font)
{
	FT_Face face;
	TT_Postscript *post;
	double base = 0.0;
	double skew;

	if (font == NULL) {
		return 0.0;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return 0.0;
	}
	/* THE ITALIC ANGLE, FROM THE `post` TABLE, IN DEGREES: the table stores it as a 16.16 fixed point and
	 * FreeType has already parsed it, so this divides rather than parses. A face with no `post` table (a
	 * bitmap face, or a CFF face without one) answers ZERO — "no recorded slant" — and **THE TYPE1 ROUTE
	 * (`FT_Get_PS_Font_Info`'s `italic_angle`) IS NOT READ HERE**, which is a named gap rather than an
	 * unnoticed one: no face this tier can resolve out of `userland/fonts` needs it. */
	post = (TT_Postscript *)FT_Get_Sfnt_Table(face, ft_sfnt_post);
	if (post != NULL) {
		base = post->italicAngle / 65536.0;
	}
	/* **"WITH ANY SKEW FROM THE TRANSFORMATION MATRIX APPLIED"** (Apple's sentence for this door): the
	 * matrix's `c` is the horizontal shear per unit of y, so the angle it introduces is `atan2(c, d)`.
	 * WITH THE IDENTITY MATRIX THIS IS ZERO AND THE ANSWER IS EXACTLY THE FACE'S ANGLE, which is the case
	 * the probe asserts; for a shear it is the same reading the metrics above take of a scale, and it is
	 * STATED here because Apple's sentence does not say how the two are combined. */
	skew = atan2(font->matrix.c, font->matrix.d) * 180.0 / 3.14159265358979323846;
	return base + skew;
}

/* =================================================================================================
 * THE CLASS. `-dealloc` is the ONE place a font's graphics font is released, which is what the
 * hand-rolled `CTFontRelease` did in its own `free` path and what the create door's `CGFontRetain`
 * balances.
 * =============================================================================================== */
@implementation CTFont

- (void)dealloc
{
	CGFontRelease(graphics);
	[super dealloc];
}

@end


#include "CTFont_internal.h"
#include <CoreGraphics/CGFont_internal.h>

/* AND FREETYPE, FOR EXACTLY ONE FIELD: the family door below asks the face for the family the engine
 * already parsed (`FT_Face->family_name`) rather than walking the `name` table a second time through
 * nameID 1 — the two routes can only agree, or disagree because of a platform preference this tier does
 * not model. The seam hands the face out as a `void *` so that no PUBLIC header of this library forces
 * FreeType's include layout on anyone; a caller that wants a FIELD of the face owns including the
 * engine's header itself, which is why the include is here, in the Objective-C half. */
#include <ft2build.h>
#include FT_FREETYPE_H

/* -------------------------------------------------------------------------------------------------
 * THE NAME SPECIFIER CONSTANTS.
 *
 * APPLE'S VALUES ARE PUBLISHED NOWHERE — the documentation lists the names and the keys they take,
 * never the strings — which is the same finding this tree already recorded for enum values ("Apple
 * and GNUstep documentation both give case names only, so values are ours"). A STRING VALUE IS
 * THEREFORE OURS, and it is safe to be ours because a key is only ever COMPARED with the constants
 * declared beside it: `CTFontCopyName` matches the caller's key against exactly these objects.
 * ---------------------------------------------------------------------------------------------- */
NSString *const kCTFontCopyrightNameKey = @"CTFontCopyrightName";
NSString *const kCTFontFamilyNameKey = @"CTFontFamilyName";
NSString *const kCTFontSubFamilyNameKey = @"CTFontSubFamilyName";
NSString *const kCTFontStyleNameKey = @"CTFontStyleName";
NSString *const kCTFontUniqueNameKey = @"CTFontUniqueName";
NSString *const kCTFontFullNameKey = @"CTFontFullName";
NSString *const kCTFontVersionNameKey = @"CTFontVersionName";
NSString *const kCTFontPostScriptNameKey = @"CTFontPostScriptName";
NSString *const kCTFontTrademarkNameKey = @"CTFontTrademarkName";
NSString *const kCTFontManufacturerNameKey = @"CTFontManufacturerName";
NSString *const kCTFontDesignerNameKey = @"CTFontDesignerName";
NSString *const kCTFontDescriptionNameKey = @"CTFontDescriptionName";
NSString *const kCTFontVendorURLNameKey = @"CTFontVendorURLName";
NSString *const kCTFontDesignerURLNameKey = @"CTFontDesignerURLName";
NSString *const kCTFontLicenseNameKey = @"CTFontLicenseName";
NSString *const kCTFontLicenseURLNameKey = @"CTFontLicenseURLName";
NSString *const kCTFontSampleTextNameKey = @"CTFontSampleTextName";
NSString *const kCTFontPostScriptCIDNameKey = @"CTFontPostScriptCIDName";

/* THE KEY→nameID TABLE IS ONE FUNCTION, and its `-1` is the answer for a key this tier does not know:
 * the two name tables in Apple's own header that are NOT here (`kCTFontVariationAxisNameKey` and the
 * feature keys) belong to the variation and feature collections, whose OWN units own those rows. A
 * caller who passes one of them gets nil rather than a guess about which `name` entry they meant. */
static int ct_name_id_for_key(NSString *key)
{
	if ([key isEqualToString:kCTFontCopyrightNameKey]) {
		return 0;
	}
	if ([key isEqualToString:kCTFontFamilyNameKey]) {
		return 1;
	}
	if ([key isEqualToString:kCTFontSubFamilyNameKey]) {
		return 2;
	}
	if ([key isEqualToString:kCTFontUniqueNameKey]) {
		return 3;
	}
	if ([key isEqualToString:kCTFontFullNameKey]) {
		return 4;
	}
	if ([key isEqualToString:kCTFontVersionNameKey]) {
		return 5;
	}
	if ([key isEqualToString:kCTFontPostScriptNameKey]) {
		return 6;
	}
	if ([key isEqualToString:kCTFontTrademarkNameKey]) {
		return 7;
	}
	if ([key isEqualToString:kCTFontManufacturerNameKey]) {
		return 8;
	}
	if ([key isEqualToString:kCTFontDesignerNameKey]) {
		return 9;
	}
	if ([key isEqualToString:kCTFontDescriptionNameKey]) {
		return 10;
	}
	if ([key isEqualToString:kCTFontVendorURLNameKey]) {
		return 11;
	}
	if ([key isEqualToString:kCTFontDesignerURLNameKey]) {
		return 12;
	}
	if ([key isEqualToString:kCTFontLicenseNameKey]) {
		return 13;
	}
	if ([key isEqualToString:kCTFontLicenseURLNameKey]) {
		return 14;
	}
	if ([key isEqualToString:kCTFontSampleTextNameKey]) {
		return 19;
	}
	if ([key isEqualToString:kCTFontPostScriptCIDNameKey]) {
		return 20;
	}
	/* `STYLE NAME` IS DELIBERATELY ABSENT FROM THE TABLE ABOVE AND IS NOT AN OVERSIGHT: Apple documents
	 * it as "the style name", and the OpenType `name` table has no such ID — what it has is ID 2, the
	 * subfamily, which IS the style in every font this tier has measured. Mapping the key to 2 would be
	 * an invention about a table, so the key exists (a caller may hold it, and it is an `open` ledger
	 * row like its neighbours) while the lookup answers nil. */
	return -1;
}

/* THE ONE PLACE A C STRING BECOMES AN `NSString`, so the +1 rule is written once: this function
 * returns what `[NSString stringWithUTF8String:]]` gives it, which is +1, and every door below hands
 * that straight to its caller. */
static NSString *ct_string_from_face(int name_id, CTFontRef font)
{
	char buf[512];
	void *face = cg_font_face(font->graphics);

	if (face == NULL) {
		return nil;
	}
	if (name_id >= 0 && cg_font_name_entry(font->graphics, name_id, buf, sizeof buf) > 0) {
		return [NSString stringWithUTF8String:buf];
	}
	return nil;
}

/* -------------------------------------------------------------------------------------------------
 * THE CREATE DOOR THAT TAKES A NAME.
 * ---------------------------------------------------------------------------------------------- */

CTFontRef CTFontCreateWithName(NSString *name, CGFloat size, const CGAffineTransform *matrix)
{
	const char *utf8;
	CGFontRef graphics;
	CTFontRef font;

	if (name == nil) {
		return NULL;
	}
	utf8 = [name UTF8String];
	if (utf8 == NULL) {
		return NULL;
	}
	/* THE REGISTRY'S OWN RULE, WHICH IS NOT APPLE'S: the doc block for Apple's door promises "a best
	 * match font will always be returned", and this tier's registry ANSWERS NULL for a name it cannot
	 * resolve (it "ANSWERS NULL rather than substituting a font the caller did not ask for", which is
	 * the CoreGraphics seam's own sentence). The name matches a PostScript name, a full name or
	 * `family style`, case-insensitively — and NULL is what a caller gets for anything else. */
	graphics = cg_font_create_with_name(utf8);
	if (graphics == NULL) {
		return NULL;
	}
	/* THE CT FONT TAKES ITS OWN REFERENCE, so this one goes back: the create door below retains. */
	font = CTFontCreateWithGraphicsFont(graphics, size, matrix, NULL);
	CGFontRelease(graphics);
	return font;
}

/* -------------------------------------------------------------------------------------------------
 * THE NAME DOORS. ALL FOUR BELOW RETURN +1, AND ALL FOUR ANSWER nil FOR "THE FACE HAS NO SUCH NAME",
 * which the header states where a caller meets them: a missing name is a font fact, not a failure.
 * ---------------------------------------------------------------------------------------------- */

NSString *CTFontCopyPostScriptName(CTFontRef font)
{
	if (font == NULL) {
		return nil;
	}
	return CGFontCopyPostScriptName(font->graphics);
}

NSString *CTFontCopyFullName(CTFontRef font)
{
	if (font == NULL) {
		return nil;
	}
	return CGFontCopyFullName(font->graphics);
}

NSString *CTFontCopyFamilyName(CTFontRef font)
{
	FT_Face face;

	if (font == NULL) {
		return nil;
	}
	/* THE FAMILY IS NOT A `name`-TABLE LOOKUP IN THIS TIER, and that is a measured choice rather than a
	 * shortcut: FreeType already parsed the family out of the face (`FT_Face->family_name`, UTF-8,
	 * borrowed) when the face was opened, and asking the same table a second time through nameID 1 can
	 * only agree with it or disagree because of a platform preference this tier does not model. */
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL || face->family_name == NULL) {
		return nil;
	}
	return [NSString stringWithUTF8String:face->family_name];
}

NSString *CTFontCopyDisplayName(CTFontRef font)
{
	/* APPLE'S DOC SAYS "the display name ... usually the full name", AND THIS TIER HAS NO LOCALIZATION
	 * AT ALL (no language table, no locale — `CTFontCopyLocalizedName` below says the same thing), so
	 * the local name and the display name are the same name here. The deviation is stated rather than
	 * hidden: a caller asking for a display name gets the full name, which is what the documentation
	 * says it usually is. */
	return CTFontCopyFullName(font);
}

NSString *CTFontCopyName(CTFontRef font, NSString *key)
{
	if (font == NULL || key == nil) {
		return nil;
	}
	return ct_string_from_face(ct_name_id_for_key(key), font);
}

NSString *CTFontCopyLocalizedName(CTFontRef font, NSString *key, NSString **actualLanguage)
{
	/* THE OUT-PARAMETER IS SET, ALWAYS, AND IT IS nil — which is the honest answer for a tier with no
	 * language information: Apple documents the parameter as "the language of the returned string ...
	 * not intended for use in a language-sensitive way", and nil says "no language was chosen" rather
	 * than naming one this library never looked at. */
	if (actualLanguage != NULL) {
		*actualLanguage = nil;
	}
	return CTFontCopyName(font, key);
}

/* =================================================================================================
 * CT-M1d: THE FONT'S COVERAGE — `CTFontCopyCharacterSet`.
 *
 * APPLE'S SENTENCE IS THE SPECIFICATION: "the font's character set ... covers the NOMINAL referenced by
 * the font's Unicode cmap table (or equivalent)". So this door is THE SAME CMAP `CTFontGetGlyphsForCharacters`
 * looks a character up in, WALKED RATHER THAN SEARCHED — and the walk is FreeType's own
 * (`FT_Get_First_Char`/`FT_Get_Next_Char`), so the membership answer cannot disagree with the glyph door:
 * both are one table, read by one engine.
 *
 * THREE THINGS ABOUT IT ARE THIS TIER'S AND ARE SAID RATHER THAN DISCOVERED:
 *
 *   * **THE SET IS 16-BIT.** This tree's `NSMutableCharacterSet` adds NSRanges over `unichar`, so the walk
 *     stops at U+FFFF and a SUPPLEMENTARY character is not a member. Apple's `CFCharacterSet` has planes
 *     and this class can represent them (`-longCharacterIsMember:`, `-hasMemberInPlane:`), but the MUTATORS
 *     are 16-bit, and building the bitmap representation by hand to reach planes 1-16 would be a different
 *     door's job. No face this tier resolves out of `userland/fonts` has a supplementary-plane cmap entry,
 *     so the boundary is stated rather than paid for.
 *   * **THE RANGES ARE RUN-LENGTH MERGED** as the walk proceeds, so a font with 6,000 characters costs a
 *     few hundred mutator calls rather than 6,000 — the cmap is ordered, which is what makes one pass
 *     enough.
 *   * **THE MUTABLE SET HAS NO PUBLIC INITIALISER IN THIS TREE** (the constructions are `-mutableCopy` and
 *     an internal `-initWithRange:`), so the set is seeded through a ONE-CHARACTER set the first time a
 *     character is found, and a font with NO characters answers an EMPTY set rather than nil — "this font
 *     covers nothing" is an answer, not an error.
 * =============================================================================================== */

/* THE RUN-ADDER, IN ONE PLACE SO THE SEEDING RULE IS WRITTEN ONCE: the first call SEEDS the set from a
 * one-character set (this tree's `NSMutableCharacterSet` has no public initialiser — the constructions are
 * `-mutableCopy` and an internal `-initWithRange:`), and every later call adds a range. */
static NSMutableCharacterSet *ctf_add_range(NSMutableCharacterSet *set, FT_ULong start, FT_ULong end)
{
	if (set == nil) {
		set = [[NSCharacterSet characterSetWithRange:NSMakeRange((NSUInteger)start, 1)] mutableCopy];
		if (end == start) {
			return set;
		}
		start += 1;
	}
	[set addCharactersInRange:NSMakeRange((NSUInteger)start, (NSUInteger)(end - start + 1))];
	return set;
}

NSCharacterSet *CTFontCopyCharacterSet(CTFontRef font)
{
	FT_Face face;
	FT_ULong code;
	FT_UInt index;
	FT_ULong run_start = 0;
	FT_ULong prev = 0;
	NSMutableCharacterSet *set = nil;
	BOOL have_run = NO;

	if (font == NULL) {
		return nil;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return nil;
	}
	for (code = FT_Get_First_Char(face, &index); index != 0;
	     code = FT_Get_Next_Char(face, code, &index)) {
		if (code > 0xFFFF) {
			break;	/* see the 16-bit boundary above; the cmap is ordered, so this ends the walk */
		}
		if (!have_run) {
			run_start = code;
			have_run = YES;
		} else if (code != prev + 1) {
			set = ctf_add_range(set, run_start, prev);
			run_start = code;
		}
		prev = code;
	}
	if (have_run) {
		set = ctf_add_range(set, run_start, prev);
	}
	if (set == nil) {
		return [[NSCharacterSet characterSetWithRange:NSMakeRange(0, 0)] retain];
	}
	return set;	/* +1, built here and owned by the caller */
}


/* **`CTFontCreateUIFontForLanguage` — THE UI FONT DOOR, SHIPPED WITH A MAPPING RATHER THAN A TABLE.**
 *
 * The era's header declares it and describes it in full, and it declares the uiType as a plain
 * `typedef uint32_t` with no enumerators — so a caller passes a number it cannot name, and there is no
 * vocabulary here to get wrong. What the sentence leaves open is which FACE each uiType means, and that is
 * where this tier states a divergence: **it ships one face**, so every uiType answers it and `language`
 * selects nothing further. The one part of the sentence this door honours exactly is the size: 0.0 means
 * "the default size for the requested uiType", and with one uiType family the default is one number.
 *
 * The divergence is recorded rather than hidden because Apple's later headers are where the distinctions
 * between a system font, a user font, a small-system font and the rest actually live — and a tier that
 * invented those distinctions out of its own head would be inventing the surface this ledger exists to
 * measure. */
#define CT_UI_DEFAULT_SIZE 13.0
#define CT_UI_SYSTEM_FONT_NAME @"DejaVuSans"

CTFontRef CTFontCreateUIFontForLanguage(CTFontUIFontType uiType, CGFloat size, NSString *language)
{
	(void)uiType;
	(void)language;
	if (size <= 0.0) {
		size = CT_UI_DEFAULT_SIZE;
	}
	return CTFontCreateWithName(CT_UI_SYSTEM_FONT_NAME, size, NULL);
}

/* ====================================================================================================
 * M6r: THE FONT'S ATTRIBUTES, TRAITS, NAMES, OUTLINES AND INK.
 * ================================================================================================== */

/* "Returns the value associated with an arbitrary attribute of the font" — which the font's DESCRIPTOR is what
 * holds, so this is the descriptor's own door applied to the descriptor made from this font. */
CFTypeRef CTFontCopyAttribute(CTFontRef font, NSString *attribute)
{
	CTFontDescriptorRef descriptor;

	if (font == NULL) {
		return NULL;
	}
	descriptor = ctf_descriptor_from_font(font);
	if (descriptor == NULL) {
		return NULL;
	}
	return CTFontDescriptorCopyAttribute(descriptor, attribute);
}

/* **"Returns the traits of the font reference" — AND IT IS APPLE'S FOUR KEYS AND NOTHING ELSE.** The dictionary
 * used to carry the symbolic traits, the font's SIZE (under a `kCTFontSizeTrait` Apple does not declare) and its
 * FAMILY NAME (likewise). §46 found those two names; this is the unit that removes them, because they were not
 * merely extra — **A CALLER COULD NOT TELL THIS DICTIONARY FROM A REAL ONE**, and a `kCTFontSizeTrait` that
 * happened to be read by another implementation would have been answered from the wrong thing entirely.
 *
 * The symbolic traits come from the door above; the three numbers come from the face's OS/2 table through the
 * normalizers beside this file's top. **THE PROBE CHECKS THE COUNT** — exactly four — because a fifth key is the
 * defect this class actually shipped with. */
NSDictionary *CTFontCopyTraits(CTFontRef font)
{
	NSMutableDictionary *traits = [NSMutableDictionary dictionary];
	TT_OS2 *os2;

	if (font == NULL || font->graphics == NULL) {
		return nil;
	}
	os2 = ct_os2(font);
	[traits setObject:[NSNumber numberWithUnsignedInt:(unsigned)CTFontGetSymbolicTraits(font)]
		   forKey:kCTFontSymbolicTrait];
	[traits setObject:[NSNumber numberWithDouble:ct_normalized_weight(os2 != NULL ? os2->usWeightClass : 0u)]
		   forKey:kCTFontWeightTrait];
	[traits setObject:[NSNumber numberWithDouble:ct_normalized_width(os2 != NULL ? os2->usWidthClass : 0u)]
		   forKey:kCTFontWidthTrait];
	/* **THE SLANT IS THE ONE SCALE THE ERA FIXES EXACTLY** — "0.0 corresponds to 0 degree clockwise rotation
	 * from the vertical and 1.0 corresponds to 30 degrees" — so it is the face's own slant angle over thirty,
	 * clamped to the range the sentence names. */
	[traits setObject:[NSNumber numberWithDouble:ct_clamp_unit((double)CTFontGetSlantAngle(font) / 30.0)]
		   forKey:kCTFontSlantTrait];
	return traits;
}

/* "The glyph ... for the given glyph name" — the FACE'S OWN name table, the same lookup `CTGlyphInfo` uses, so
 * a name that resolves there resolves here. */
CGGlyph CTFontGetGlyphWithName(CTFontRef font, NSString *glyphName)
{
	FT_Face face;
	const char *name;

	if (font == NULL || glyphName == nil) {
		return 0;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	name = [glyphName UTF8String];
	if (face == NULL || name == NULL) {
		return 0;
	}
	return (CGGlyph)FT_Get_Name_Index(face, (FT_String *)name);
}

/* "The path ... for the glyph" — the outline as a `CGPath`, through the bridge the stroked route also uses.
 * **THE FONT'S OWN MATRIX AND THE CALLER'S ARE COMPOSED AND HANDED TO THE OUTLINE DOOR**, which takes a matrix
 * for exactly this: composing afterwards would need a door this tier does not have, and the matrix belongs
 * where the glyph is decomposed rather than after it. */
CGPathRef CTFontCreatePathForGlyph(CTFontRef font, CGGlyph glyph, const CGAffineTransform *matrix)
{
	CGAffineTransform combined;

	if (font == NULL) {
		return NULL;
	}
	combined = font->matrix;
	if (matrix != NULL) {
		combined = CGAffineTransformConcat(combined, *matrix);
	}
	return ct_outline_path(font, glyph, font->size, combined);
}

/* "Calculates the bounding rects for an array of glyphs and returns the OVERALL bounding rect for the run" — so
 * the per-glyph boxes go through the caller's array and the union is the return. */
CGRect CTFontGetBoundingRectsForGlyphs(CTFontRef font, CTFontOrientation orientation, const CGGlyph *glyphs,
				       CGRect *boundingRects, long count)
{
	CGRect overall = CGRectNull;

	if (font == NULL || glyphs == NULL || count <= 0) {
		return CGRectNull;
	}
	if (orientation != kCTFontDefaultOrientation && orientation != kCTFontHorizontalOrientation) {
		return CGRectNull;	/* APPKIT-REFUSE: the vertical orientation is a layout this tier does not do */
	}
	for (long i = 0; i < count; i++) {
		CGPoint at = { 0.0, 0.0 };
		CGRect box = ct_ink_bounds(font, &glyphs[i], &at, 1, NSMakeRange(0, 0));

		if (boundingRects != NULL) {
			boundingRects[i] = box;
		}
		if (!CGRectIsNull(box)) {
			overall = CGRectIsNull(overall) ? box : CGRectUnion(overall, box);
		}
	}
	return overall;
}

/* AND APPLE'S FOUR TRAIT KEYS (M6r, corrected by M7f), declared in CTFontTraits.h where Apple declares them.
 * **THE TWO THAT USED TO BE HERE — `kCTFontSizeTrait` and `kCTFontFamilyNameTrait` — ARE GONE**, with the
 * argument in that header and in the plan's §53: they were names Apple does not have, and their values were
 * AppKit's own attribute spellings. */
NSString *const kCTFontSymbolicTrait = @"NSFontSymbolicTrait";
NSString *const kCTFontWeightTrait = @"NSFontWeightTrait";
NSString *const kCTFontWidthTrait = @"NSFontWidthTrait";
NSString *const kCTFontSlantTrait = @"NSFontSlantTrait";

/* **AND THE ELEVEN VARIATION AND FEATURE KEYS (M7f)**, declared in CTFont.h where Apple declares them. Values
 * are ours, as every unpublished key's are — a key is only ever compared with the constant beside it. */
NSString *const kCTFontVariationAxisIdentifierKey = @"CTFontVariationAxisIdentifier";
NSString *const kCTFontVariationAxisMinimumValueKey = @"CTFontVariationAxisMinimumValue";
NSString *const kCTFontVariationAxisMaximumValueKey = @"CTFontVariationAxisMaximumValue";
NSString *const kCTFontVariationAxisDefaultValueKey = @"CTFontVariationAxisDefaultValue";
NSString *const kCTFontVariationAxisNameKey = @"CTFontVariationAxisName";
NSString *const kCTFontFeatureTypeNameKey = @"CTFontFeatureTypeName";
NSString *const kCTFontFeatureTypeExclusiveKey = @"CTFontFeatureTypeExclusive";
NSString *const kCTFontFeatureTypeSelectorsKey = @"CTFontFeatureTypeSelectors";
NSString *const kCTFontFeatureSelectorNameKey = @"CTFontFeatureSelectorName";
NSString *const kCTFontFeatureSelectorDefaultKey = @"CTFontFeatureSelectorDefault";
NSString *const kCTFontFeatureSelectorSettingKey = @"CTFontFeatureSelectorSetting";

/* ====================================================================================================
 * M6s: THE OPTIONS AND THE FAMILY.
 * ================================================================================================== */

/* **THE OPTIONS DESCRIBE BEHAVIOUR THIS TIER ALREADY HAS**, which is why this is a door rather than a stub: the
 * prevent-auto-activation flag is satisfied because the registry never activates anything, and the
 * prefer-system-font flag because there is one font set to prefer within. Both are recorded as read, so a
 * reader can see they were considered rather than forgotten. */
CTFontRef CTFontCreateWithNameAndOptions(NSString *name, CGFloat size, const CGAffineTransform *matrix,
					 CTFontOptions options)
{
	(void)options;		/* see above: both flags' effects are absent by construction in this tier */
	return CTFontCreateWithName(name, size, matrix);
}

CTFontRef CTFontCreateWithFontDescriptorAndOptions(CTFontDescriptorRef descriptor, CGFloat size,
						   const CGAffineTransform *matrix, CTFontOptions options)
{
	(void)options;		/* the same two flags, the same argument */
	return CTFontCreateWithFontDescriptor(descriptor, size, matrix);
}

/* "A new font based on the original font with the specified family" — the family goes where this tier keeps
 * every descriptor attribute, through the descriptor's own copy door, and then the descriptor's font door
 * resolves it. A family the registry cannot resolve answers NULL, which is the era's own "or NULL". */
CTFontRef CTFontCreateCopyWithFamily(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
				     NSString *family)
{
	CTFontDescriptorRef base, changed;
	CTFontRef copy;

	if (font == NULL || family == nil) {
		return NULL;
	}
	base = ctf_descriptor_from_font(font);
	if (base == NULL) {
		return NULL;
	}
	changed = CTFontDescriptorCreateCopyWithAttributes(
		base, [NSDictionary dictionaryWithObject:family forKey:kCTFontFamilyNameAttribute]);
	if (changed == NULL) {
		return NULL;
	}
	copy = CTFontCreateWithFontDescriptor(changed, size, matrix);
	CTFontDescriptorRelease(changed);
	return copy;
}

/* ------------------------------------------------------------------------- */
/* M7b: the SFNT tables the file itself carries                             */
/* ------------------------------------------------------------------------- */

/* **ONE WALK, TWO DOORS, AND THE FACE IS THE ONLY SOURCE.** `FT_Sfnt_Table_Info` IS an enumeration — it answers
 * the tag and the length for a table INDEX and returns `FT_Err_Table_Missing` past the last one, so the loop
 * below is the file's real table directory and not a list this tree kept. `FT_Load_Sfnt_Table` then fetches a
 * table by tag from the SAME face, which is why the two doors agree by construction: a tag this array reports
 * cannot fail to fetch, and a tag it does not report cannot fetch. A second, private list of table names — the
 * vocabulary in `CTFont.h` is one — would pass every count-free check and could disagree either way.
 *
 * AND THE OPTIONS ARE THE §45 SHAPE: `kCTFontTableOptionExcludeSynthetic` asks that tables the ENGINE WOULD
 * SYNTHESIZE be left out, and this tier synthesizes none — FreeType answers with the file's own tables — so the
 * flag describes behaviour this tier already has. It is accepted and ignored in the honest sense: the probe
 * checks that the answer under the flag EQUALS the answer without it, rather than merely that no harm came of
 * it. */
NSArray *CTFontCopyAvailableTables(CTFontRef font, CTFontTableOptions options)
{
	NSMutableArray *tags;
	FT_Face face;
	FT_UInt index = 0;

	(void)options;	/* the same argument as the two flag doors: satisfied by construction */
	if (font == NULL) {
		return nil;	/* Apple's own "or NULL on error" */
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return nil;
	}
	tags = [NSMutableArray array];
	for (;;) {
		FT_ULong tag = 0;
		FT_ULong length = 0;

		if (FT_Sfnt_Table_Info(face, index, &tag, &length) != 0) {
			break;	/* FT_Err_Table_Missing: past the last table, so the walk is over */
		}
		/* APPLE'S ARRAY HOLDS THE TAG UNBOXED AND THIS ONE CANNOT (see CTFont.h): an NSArray holds
		 * objects, so the tag rides in an NSNumber. `-unsignedIntValue` is the whole of the difference. */
		[tags addObject:[NSNumber numberWithUnsignedInt:(unsigned int)tag]];
		index++;
	}
	return [tags retain];
}

NSData *CTFontCopyTable(CTFontRef font, CTFontTableTag table, CTFontTableOptions options)
{
	FT_Face face;
	FT_ULong length = 0;
	FT_Byte *buffer;
	NSData *data;

	(void)options;
	if (font == NULL) {
		return nil;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return nil;
	}
	/* **THE TWIN-STEP IS FREETYPE'S OWN DOCUMENTED PATTERN, quoted in `tttables.h`**: ask for the length with a
	 * NULL buffer first — "if error … table does not exist" — then allocate and load. So a tag the face does not
	 * carry answers NULL here, which is the case that makes this door a question: the probe pairs it with a tag
	 * the array DID report. A zero-length table is treated as missing by FreeType itself ("FreeType handles SFNT
	 * tables with length zero as missing"), so there is no second rule to state. */
	if (FT_Load_Sfnt_Table(face, (FT_ULong)table, 0, NULL, &length) != 0 || length == 0) {
		return nil;
	}
	buffer = (FT_Byte *)malloc((size_t)length);
	if (buffer == NULL) {
		return nil;
	}
	if (FT_Load_Sfnt_Table(face, (FT_ULong)table, 0, buffer, &length) != 0) {
		free(buffer);
		return nil;
	}
	/* THE BYTES ARE THE FACE'S; THE DATA OBJECT OWNS ITS OWN COPY (CTFont.h states the mapping from Apple's
	 * "not actually copied"). */
	data = [NSData dataWithBytes:buffer length:(NSUInteger)length];
	free(buffer);
	return [data retain];
}

/* -------------------------------------------------------------------------------------------------
 * M7l: the platform font identity
 * ---------------------------------------------------------------------------------------------- */

/* **A TABLE BECAUSE THE TYPE IS 32 BITS AND A FACE NAME IS NOT**, and the ids are minted ONCE PER FACE for the
 * life of the process: the same face must answer the same ref every time (that is what an identity is), and two
 * faces must never share one. Zero is the invalid ref and 1 is the first id, so a zeroed variable is never a
 * valid answer — the same rule the type-id table states.
 *
 * IT GROWS, like the font registry (§M7h) and the disabled-name list (§M7k), because a bounded table would need
 * an invented refusal for its full case and Apple's vocabulary has none. There are no locks, for the reason the
 * name cache gives: this library has none anywhere. */
static struct {
	ATSFontRef ref;
	char *name;
} *ct_ats_faces;
static int ct_ats_count;
static int ct_ats_cap;

static ATSFontRef ct_ats_ref_for_name(const char *name)
{
	int i;

	if (name == NULL || name[0] == '\0') {
		return 0;
	}
	for (i = 0; i < ct_ats_count; i++) {
		if (strcmp(ct_ats_faces[i].name, name) == 0) {
			return ct_ats_faces[i].ref;	/* the SAME face answers the SAME id, always */
		}
	}
	if (ct_ats_count == ct_ats_cap) {
		int cap = (ct_ats_cap == 0) ? 8 : ct_ats_cap * 2;
		void *grown = realloc(ct_ats_faces, (size_t)cap * sizeof ct_ats_faces[0]);

		if (grown == NULL) {
			return 0;
		}
		ct_ats_faces = grown;
		ct_ats_cap = cap;
	}
	ct_ats_faces[ct_ats_count].name = strdup(name);
	if (ct_ats_faces[ct_ats_count].name == NULL) {
		return 0;
	}
	ct_ats_faces[ct_ats_count].ref = (ATSFontRef)(ct_ats_count + 1);
	return ct_ats_faces[ct_ats_count++].ref;
}

static const char *ct_ats_name_for_ref(ATSFontRef ref)
{
	int i;

	if (ref == 0) {
		return NULL;
	}
	for (i = 0; i < ct_ats_count; i++) {
		if (ct_ats_faces[i].ref == ref) {
			return ct_ats_faces[i].name;
		}
	}
	return NULL;
}

/* AN UNKNOWN REF ANSWERS NULL, which is the case that makes this a question rather than a lookup — and it is
 * reachable, because a caller may hold a ref this process never minted (a stale one, or one from a caller that
 * invented a number). */
CTFontRef CTFontCreateWithPlatformFont(ATSFontRef platformFont, CGFloat size, const CGAffineTransform *matrix,
				       CTFontDescriptorRef attributes)
{
	const char *name = ct_ats_name_for_ref(platformFont);
	CTFontRef font;
	NSString *face;

	if (name == NULL) {
		return NULL;
	}
	face = [[NSString alloc] initWithUTF8String:name];
	font = (face != nil) ? CTFontCreateWithName(face, size, matrix) : NULL;
	[face release];
	/* **AND THE ATTRIBUTES ARE APPLIED, NOT IGNORED** — Apple's "additional attributes that should be matched"
	 * through this tier's own way of applying a descriptor to a font: the copy door, which resolves them. */
	if (font != NULL && attributes != NULL) {
		CTFontRef copied = CTFontCreateCopyWithAttributes(font, size, matrix, attributes);

		if (copied != NULL) {
			CTFontRelease(font);
			font = copied;
		}
	}
	return font;
}

ATSFontRef CTFontGetPlatformFont(CTFontRef font, CTFontDescriptorRef *attributes)
{
	NSString *name;
	ATSFontRef ref;

	if (attributes != NULL) {
		*attributes = NULL;
	}
	if (font == NULL) {
		return 0;	/* zero is not an id */
	}
	name = CTFontCopyPostScriptName(font);
	if (name == nil) {
		return 0;
	}
	ref = ct_ats_ref_for_name([name UTF8String]);
	[name release];
	/* AND THE ATTRIBUTES COME BACK AS A DESCRIPTOR OF THIS FONT, which is Apple's own reading of the
	 * out-parameter ("a font descriptor containing additional attributes") and the +1 "Must be released by
	 * caller" — this door's caller, unlike the error out-parameters §M7h documents. */
	if (ref != 0 && attributes != NULL) {
		*attributes = CTFontCopyFontDescriptor(font);
	}
	return ref;
}

/* ====================================================================================================
 * M7m: the variation and feature doors
 * ================================================================================================== */

/* THE `feat` TAG, SPELLED THE WAY FREETYPE SPELLS A TAG rather than as a literal. */
#define CT_FEAT_TAG FT_MAKE_TAG('f', 'e', 'a', 't')

/* **AND THE LIBRARY HANDLE, WHICH `FT_Done_MM_Var` NEEDS**: FreeType's multiple-master data is freed against
 * the library rather than the face, and the face carries it — a door that forgot this would leak the whole
 * axis array on every call. */
static FT_Library ct_face_library(CTFontRef font)
{
	FT_Face face = (FT_Face)cg_font_face(font->graphics);

	return (face != NULL && face->glyph != NULL) ? face->glyph->library : NULL;
}

/* **THE FACE'S MULTIPLE-MASTER DATA, OR NULL, AND THE CALLER RELEASES IT**: one place, because both variation
 * doors read the same face and `FT_Get_MM_Var` allocates. */
static FT_MM_Var *ct_mm_var(CTFontRef font)
{
	FT_Face face;
	FT_MM_Var *mm = NULL;

	if (font == NULL) {
		return NULL;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL || FT_Get_MM_Var(face, &mm) != 0) {
		return NULL;
	}
	return mm;
}

/* **"AN ARRAY OF VARIATION AXES … EACH VARIATION AXIS DICTIONARY CONTAINS THE FIVE VARIATION AXIS KEYS"** —
 * Apple's two sentences, and the face's `fvar` is the array. The values are 16.16 fixed point in FreeType and
 * the KEYS' values are `CFNumberRef`s, so the division is the whole of the conversion; the NAME comes from the
 * face's own name table entry for the axis, which FreeType has already resolved into a string. */
NSArray *CTFontCopyVariationAxes(CTFontRef font)
{
	NSMutableArray *axes = [NSMutableArray array];
	FT_MM_Var *mm = ct_mm_var(font);
	unsigned i;

	if (font == NULL) {
		return nil;
	}
	if (mm == NULL) {
		/* **A FACE WITH NO `fvar` ANSWERS AN EMPTY ARRAY, NOT NULL**: "this font has no axes" is an answer, and
		 * Apple's own result is "an array of variation axes" — possibly empty. */
		return (NSArray *)[axes retain];
	}
	for (i = 0; i < mm->num_axis; i++) {
		FT_Var_Axis *axis = &mm->axis[i];
		NSMutableDictionary *entry = [NSMutableDictionary dictionary];

		[entry setObject:[NSNumber numberWithUnsignedInt:(unsigned)axis->tag]
			  forKey:kCTFontVariationAxisIdentifierKey];
		[entry setObject:[NSNumber numberWithDouble:(double)axis->minimum / 65536.0]
			  forKey:kCTFontVariationAxisMinimumValueKey];
		[entry setObject:[NSNumber numberWithDouble:(double)axis->maximum / 65536.0]
			  forKey:kCTFontVariationAxisMaximumValueKey];
		[entry setObject:[NSNumber numberWithDouble:(double)axis->def / 65536.0]
			  forKey:kCTFontVariationAxisDefaultValueKey];
		if (axis->name != NULL) {
			NSString *name = [NSString stringWithUTF8String:axis->name];

			if (name != nil) {
				[entry setObject:name forKey:kCTFontVariationAxisNameKey];
			}
		}
		[axes addObject:entry];
	}
	FT_Done_MM_Var(ct_face_library(font), mm);
	return (NSArray *)[axes retain];
}

/* **"THE CURRENT VARIATION INSTANCE AS A DICTIONARY", KEYED BY THE AXIS IDENTIFIER** — and this tier's current
 * instance is the DEFAULT one, said out loud: a font here is made from a name, a size and a matrix, and the
 * variation attribute is one of §41's descriptor attributes that has not landed, so nothing can move a font
 * away from its face's defaults. The probe checks this dictionary's values ARE the axes' DefaultValue entries,
 * which is what makes the pair two views of one answer rather than two answers. */
NSDictionary *CTFontCopyVariation(CTFontRef font)
{
	NSMutableDictionary *instance = [NSMutableDictionary dictionary];
	FT_MM_Var *mm = ct_mm_var(font);
	unsigned i;

	if (font == NULL) {
		return nil;
	}
	if (mm == NULL) {
		return (NSDictionary *)[instance retain];
	}
	for (i = 0; i < mm->num_axis; i++) {
		FT_Var_Axis *axis = &mm->axis[i];

		[instance setObject:[NSNumber numberWithDouble:(double)axis->def / 65536.0]
			     forKey:[NSNumber numberWithUnsignedInt:(unsigned)axis->tag]];
	}
	FT_Done_MM_Var(ct_face_library(font), mm);
	return (NSDictionary *)[instance retain];
}

/* -------------------------------------------------------------------------------------------------- */
/* The `feat` table, parsed here because no engine hands it over                                      */
/* -------------------------------------------------------------------------------------------------- */

#define CT_FEAT_EXCLUSIVE	0x8000
#define CT_FEAT_NOTDEFAULT	0x4000
#define CT_FEAT_INDEXMASK	0x00FF

static unsigned ct_be16(const unsigned char *p)
{
	return ((unsigned)p[0] << 8) | (unsigned)p[1];
}

static long ct_sbe16(const unsigned char *p)
{
	return (long)(short)ct_be16(p);
}

static unsigned long ct_be32(const unsigned char *p)
{
	return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
	       ((unsigned long)p[2] << 8) | (unsigned long)p[3];
}

/* **THE FACE'S `feat` TABLE, READ THROUGH THE SAME TWIN-STEP THE TABLE DOOR USES** (ask the length, allocate,
 * read) — and a face with none answers 0, which is the case both doors turn into Apple's NULL. */
static unsigned char *ct_feat_table(CTFontRef font, FT_ULong *length)
{
	FT_Face face;
	FT_ULong len = 0;
	FT_Byte *buffer;

	*length = 0;
	if (font == NULL) {
		return NULL;
	}
	face = (FT_Face)cg_font_face(font->graphics);
	if (face == NULL) {
		return NULL;
	}
	if (FT_Load_Sfnt_Table(face, (FT_ULong)CT_FEAT_TAG, 0, NULL, &len) != 0 || len < 12) {
		return NULL;
	}
	buffer = (FT_Byte *)malloc((size_t)len);
	if (buffer == NULL) {
		return NULL;
	}
	if (FT_Load_Sfnt_Table(face, (FT_ULong)CT_FEAT_TAG, 0, buffer, &len) != 0) {
		free(buffer);
		return NULL;
	}
	*length = len;
	return buffer;
}

/* THE NAME TABLE'S ENTRY FOR ONE `feat` nameID, through the CoreGraphics seam the font's own names use. */
static NSString *ct_feat_name(CTFontRef font, long name_id)
{
	char buf[256];

	if (name_id <= 0 || cg_font_name_entry(font->graphics, (int)name_id, buf, sizeof buf) <= 0) {
		return nil;
	}
	return [NSString stringWithUTF8String:buf];
}

/* **ONE PARSE, TWO DOORS** — `features` gets Apple's array of feature dictionaries and `settings` gets the
 * non-default settings to APPLY, and either may be NULL. Two parsers would be two readings of one table, which
 * is the shape of bug this framework keeps removing. */
static void ct_feat_read(CTFontRef font, NSArray **features, NSArray **settings)
{
	unsigned char *table;
	FT_ULong length = 0;
	unsigned count, i;
	NSMutableArray *out_features = (features != NULL) ? [NSMutableArray array] : nil;
	NSMutableArray *out_settings = (settings != NULL) ? [NSMutableArray array] : nil;

	if (features != NULL) { *features = nil; }
	if (settings != NULL) { *settings = nil; }
	table = ct_feat_table(font, &length);
	if (table == NULL) {
		return;
	}
	/* APPLE'S OWN VERSION TEST IS "major == 1" (HarfBuzz's reader states it that way, and the era's header says
	 * 0x00010000), so a future version is refused rather than misread. */
	if (ct_be32(table) != 0x00010000UL) {
		free(table);
		return;
	}
	count = ct_be16(table + 4);
	for (i = 0; i < count; i++) {
		const unsigned char *rec = table + 12 + 12 * i;
		unsigned feature, n_settings, flags;
		unsigned long settings_at;
		long name_id;
		unsigned default_index = 0;
		unsigned s;

		if ((unsigned long)(12 + 12 * (i + 1)) > length) {
			break;
		}
		feature = ct_be16(rec);
		n_settings = ct_be16(rec + 2);
		settings_at = ct_be32(rec + 4);
		flags = ct_be16(rec + 8);
		name_id = ct_sbe16(rec + 10);
		if (settings_at + 4UL * n_settings > length) {
			break;
		}
		/* **AND THE DEFAULT INDEX IS THE AAT RULE, NOT A GUESS**: "if 0x4000 is clear, the setting at index 0
		 * is the default; if set, bits 0-15 hold the index". The probe's fixture has one feature of each shape,
		 * which is why the door can be held to it. */
		if (flags & CT_FEAT_EXCLUSIVE) {
			default_index = (flags & CT_FEAT_NOTDEFAULT) ? (flags & CT_FEAT_INDEXMASK) : 0;
		}
		if (out_features != nil) {
			NSMutableDictionary *entry = [NSMutableDictionary dictionary];
			NSMutableArray *selectors = [NSMutableArray array];
			NSString *type_name = ct_feat_name(font, name_id);

			[entry setObject:[NSNumber numberWithUnsignedInt:feature]
				  forKey:kCTFontFeatureTypeIdentifierKey];
			if (type_name != nil) {
				[entry setObject:type_name forKey:kCTFontFeatureTypeNameKey];
			}
			[entry setObject:[NSNumber numberWithBool:(flags & CT_FEAT_EXCLUSIVE) ? YES : NO]
				  forKey:kCTFontFeatureTypeExclusiveKey];
			for (s = 0; s < n_settings; s++) {
				const unsigned char *set = table + settings_at + 4 * s;
				NSMutableDictionary *selector = [NSMutableDictionary dictionary];
				NSString *selector_name = ct_feat_name(font, ct_sbe16(set + 2));
				BOOL is_default = (n_settings > 0) && (s == default_index);

				[selector setObject:[NSNumber numberWithUnsignedInt:ct_be16(set)]
					     forKey:kCTFontFeatureSelectorIdentifierKey];
				if (selector_name != nil) {
					[selector setObject:selector_name forKey:kCTFontFeatureSelectorNameKey];
				}
				/* **"IF PRESENT AND TRUE INDICATES THAT THIS SELECTOR IS THE DEFAULT"** — so it is written on
				 * every selector, true for the default: a false value satisfies the sentence's "and true" and
				 * tells a reader more than an absent key would. */
				[selector setObject:[NSNumber numberWithBool:is_default]
					     forKey:kCTFontFeatureSelectorDefaultKey];
				/* **AND THE CURRENT SETTING, WHOSE OWN SENTENCE IS "If this key is not present, the default
				 * setting is used"** — this tier has no per-font setting store, so the font's current state IS
				 * its designed default, and that selector is the one reported as ON. */
				[selector setObject:[NSNumber numberWithBool:is_default]
					     forKey:kCTFontFeatureSelectorSettingKey];
				[selectors addObject:selector];
			}
			[entry setObject:selectors forKey:kCTFontFeatureTypeSelectorsKey];
			[out_features addObject:entry];
		}
		if (out_settings != nil) {
			/* **THE SETTINGS TO APPLY ARE THE ONES THE FONT'S DEFAULT TURNS ON AND THE "OFF" VALUE DOES NOT**:
			 * a feature whose designed default is the zero setting needs nothing applied, and one whose default
			 * is a NON-ZERO setting must have that setting applied for the font to look as designed. With no
			 * such feature the answer is NULL, which is Apple's "if the default settings should be used". */
			const unsigned char *set = table + settings_at + 4 * default_index;

			if (n_settings > 0 && ct_be16(set) != 0) {
				NSMutableDictionary *tuple = [NSMutableDictionary dictionary];

				[tuple setObject:[NSNumber numberWithUnsignedInt:feature]
					  forKey:kCTFontFeatureTypeIdentifierKey];
				[tuple setObject:[NSNumber numberWithUnsignedInt:ct_be16(set)]
					  forKey:kCTFontFeatureSelectorIdentifierKey];
				[out_settings addObject:tuple];
			}
		}
	}
	free(table);
	if (features != NULL && [out_features count] > 0) {
		*features = (NSArray *)[out_features retain];
	}
	if (settings != NULL && [out_settings count] > 0) {
		*settings = (NSArray *)[out_settings retain];
	}
}

NSArray *CTFontCopyFeatures(CTFontRef font)
{
	NSArray *features = nil;

	if (font == NULL) {
		return nil;
	}
	ct_feat_read(font, &features, NULL);
	return features;
}

NSArray *CTFontCopyFeatureSettings(CTFontRef font)
{
	NSArray *settings = nil;

	if (font == NULL) {
		return nil;
	}
	ct_feat_read(font, NULL, &settings);
	return settings;
}
