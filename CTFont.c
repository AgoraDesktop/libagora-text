/*
 * CTFont.c — the font: its keys, its type, and every door this tree can answer honestly.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * A FONT IS A FACE, A SIZE AND A MATRIX, AND THE FACE COMES FROM CORE GRAPHICS. That is the shape of this
 * file: `CGFont` already holds the face, already reports its metrics in font units, already maps a glyph name
 * to a glyph and already reports advances and bounding boxes — MEASURED, by reading CGFont.h — so almost
 * every door here is a call to it plus the one division that turns font units into points.
 *
 * THE TWO THINGS CORE GRAPHICS DOES NOT OFFER ARE THE REASON THIS LIBRARY EXISTS AT ALL. Apple deprecated
 * Core Graphics' public character-to-glyph doors, so the Unicode step lives here (CTFontGetGlyphsForCharacters)
 * and is answered by the font engine behind CGFont, which is what the internal header is for. The OTHER thing
 * is the name table: Core Graphics reports the PostScript name and the full name, and the other sixteen keys
 * have no door of their own, so they are read from the face's `name` table directly.
 *
 * THE KEYS' VALUES ARE THE ONE PLACE THIS FILE GUESSES, AND IT GUESSES BY MEASURED RULE RATHER THAN BY TASTE.
 * Fifteen keys across this header and CTFontDescriptor.h are PROVABLE: the AppKit declares the same name with
 * CT spelled NS, and a dictionary crosses between the frameworks, so the string has to be the same one. The
 * rule those fifteen establish is `kCTFontSomethingKey` = "NSFontSomethingKey". THE EIGHTEEN NAME KEYS TAKE
 * THAT FORM BY INFERENCE, because the AppKit was searched and declares NO `NSFont...NameKey` constant
 * anywhere. See CTFont.h's head and PROVENANCE.md; they are the first thing to check when something can.
 */
#include <CoreText/CTFont.h>

#include <CoreFoundation/CFRuntime.h>
#include <CoreGraphics/CGFont_internal.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 * THE NAME KEYS. EIGHT ARE PROVABLE: kCTFontVariationAxisIdentifierKey, ...MinimumValueKey,
 * ...MaximumValueKey, ...DefaultValueKey, ...NameKey, kCTFontFeatureTypeIdentifierKey and
 * kCTFontFeatureSelectorIdentifierKey all appear in Apple's AppKit NSFontDescriptor.h under exactly those
 * names with NS in place of CT -- MEASURED, not assumed, and it is how the other seven below were found.
 * THE OTHER TEN TAKE THE SAME FORM BY INFERENCE FROM THAT MEASURED RULE.
 * =============================================================================================== */
const CFStringRef kCTFontCopyrightNameKey = CFSTR("NSFontCopyrightNameKey");
const CFStringRef kCTFontFamilyNameKey = CFSTR("NSFontFamilyNameKey");
const CFStringRef kCTFontSubFamilyNameKey = CFSTR("NSFontSubFamilyNameKey");
const CFStringRef kCTFontStyleNameKey = CFSTR("NSFontStyleNameKey");
const CFStringRef kCTFontUniqueNameKey = CFSTR("NSFontUniqueNameKey");
const CFStringRef kCTFontFullNameKey = CFSTR("NSFontFullNameKey");
const CFStringRef kCTFontVersionNameKey = CFSTR("NSFontVersionNameKey");
const CFStringRef kCTFontPostScriptNameKey = CFSTR("NSFontPostScriptNameKey");
const CFStringRef kCTFontTrademarkNameKey = CFSTR("NSFontTrademarkNameKey");
const CFStringRef kCTFontManufacturerNameKey = CFSTR("NSFontManufacturerNameKey");
const CFStringRef kCTFontDesignerNameKey = CFSTR("NSFontDesignerNameKey");
const CFStringRef kCTFontDescriptionNameKey = CFSTR("NSFontDescriptionNameKey");
const CFStringRef kCTFontVendorURLNameKey = CFSTR("NSFontVendorURLNameKey");
const CFStringRef kCTFontDesignerURLNameKey = CFSTR("NSFontDesignerURLNameKey");
const CFStringRef kCTFontLicenseNameKey = CFSTR("NSFontLicenseNameKey");
const CFStringRef kCTFontLicenseURLNameKey = CFSTR("NSFontLicenseURLNameKey");
const CFStringRef kCTFontSampleTextNameKey = CFSTR("NSFontSampleTextNameKey");
const CFStringRef kCTFontPostScriptCIDNameKey = CFSTR("NSFontPostScriptCIDNameKey");

const CFStringRef kCTFontVariationAxisIdentifierKey = CFSTR("NSFontVariationAxisIdentifierKey");	/* provable */
const CFStringRef kCTFontVariationAxisMinimumValueKey = CFSTR("NSFontVariationAxisMinimumValueKey");	/* provable */
const CFStringRef kCTFontVariationAxisMaximumValueKey = CFSTR("NSFontVariationAxisMaximumValueKey");	/* provable */
const CFStringRef kCTFontVariationAxisDefaultValueKey = CFSTR("NSFontVariationAxisDefaultValueKey");	/* provable */
const CFStringRef kCTFontVariationAxisNameKey = CFSTR("NSFontVariationAxisNameKey");			/* provable */
const CFStringRef kCTFontFeatureTypeIdentifierKey = CFSTR("NSFontFeatureTypeIdentifierKey");		/* provable */
const CFStringRef kCTFontFeatureTypeNameKey = CFSTR("NSFontFeatureTypeNameKey");
const CFStringRef kCTFontFeatureTypeExclusiveKey = CFSTR("NSFontFeatureTypeExclusiveKey");
const CFStringRef kCTFontFeatureTypeSelectorsKey = CFSTR("NSFontFeatureTypeSelectorsKey");
const CFStringRef kCTFontFeatureSelectorIdentifierKey = CFSTR("NSFontFeatureSelectorIdentifierKey");	/* provable */
const CFStringRef kCTFontFeatureSelectorNameKey = CFSTR("NSFontFeatureSelectorNameKey");
const CFStringRef kCTFontFeatureSelectorDefaultKey = CFSTR("NSFontFeatureSelectorDefaultKey");
const CFStringRef kCTFontFeatureSelectorSettingKey = CFSTR("NSFontFeatureSelectorSettingKey");

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTFont {
	CFRuntimeBase base;
	CGFontRef graphics;		/* RETAINED: the face, which is what a font mostly is */
	CGFloat size;			/* the point size the metrics are scaled by */
	CGAffineTransform matrix;	/* the text matrix, which moves GLYPHS and not metrics */
	CTFontDescriptorRef descriptor;	/* RETAINED: what this font was asked for */
};

static void ct_font_finalize(CFTypeRef cf)
{
	struct __CTFont *font = (struct __CTFont *)cf;

	if (font->graphics != NULL) {
		CGFontRelease(font->graphics);
	}
	if (font->descriptor != NULL) {
		CFRelease(font->descriptor);
	}
}

/* TWO FONTS ARE EQUAL WHEN THEY ARE THE SAME FACE AT THE SAME SIZE UNDER THE SAME MATRIX. The descriptor is
 * NOT compared: it is what the font was asked for, and two different questions can have the same answer. */
static Boolean ct_font_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTFont *a = (const struct __CTFont *)cf1;
	const struct __CTFont *b = (const struct __CTFont *)cf2;

	return (a->graphics == b->graphics) && (a->size == b->size) &&
	       memcmp(&a->matrix, &b->matrix, sizeof(CGAffineTransform)) == 0;
}

static CFHashCode ct_font_hash(CFTypeRef cf)
{
	const struct __CTFont *font = (const struct __CTFont *)cf;

	return (CFHashCode)((uintptr_t)font->graphics ^ (uintptr_t)(font->size * 16.0));
}

static const CFRuntimeClass ct_font_class = {
	0,              /* version */
	"CTFont",       /* className */
	NULL,           /* init */
	NULL,           /* copy */
	ct_font_finalize, /* finalize */
	ct_font_equal,  /* equal */
	ct_font_hash,   /* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTFontGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_font_class);
	}
	return type_id;
}

/* THE ONE PLACE THAT MAKES A FONT, so that nothing else has to remember what a font owns. It takes what it
 * will hold and RETAINS it; the caller keeps whatever it had. */
static struct __CTFont *ct_font_alloc(CGFontRef graphics, CGFloat size, const CGAffineTransform *matrix,
				      CTFontDescriptorRef descriptor)
{
	struct __CTFont *font;

	if (graphics == NULL) {
		return NULL;
	}
	font = (struct __CTFont *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTFontGetTypeID(),
		(CFIndex)(sizeof(struct __CTFont) - sizeof(CFRuntimeBase)), NULL);
	if (font == NULL) {
		return NULL;
	}
	font->graphics = CGFontRetain(graphics);
	font->size = (size > 0.0) ? size : 12.0;	/* APPLE'S DEFAULT: a font is 12 points unless told */
	font->matrix = (matrix != NULL) ? *matrix : CGAffineTransformIdentity;
	font->descriptor = (descriptor == NULL) ? NULL : (CTFontDescriptorRef)CFRetain(descriptor);
	return font;
}

/* =================================================================================================
 * CREATION.
 * =============================================================================================== */
CTFontRef CTFontCreateWithName(CFStringRef name, CGFloat size, const CGAffineTransform *matrix)
{
	CGFontRef graphics;
	CTFontDescriptorRef descriptor;
	struct __CTFont *font;

	if (name == NULL) {
		return NULL;
	}
	graphics = CGFontCreateWithFontName(name);
	if (graphics == NULL) {
		return NULL;
	}
	descriptor = CTFontDescriptorCreateWithNameAndSize(name, size);
	font = ct_font_alloc(graphics, size, matrix, descriptor);
	CGFontRelease(graphics);
	if (descriptor != NULL) {
		CFRelease(descriptor);
	}
	return (CTFontRef)font;
}

/* A DESCRIPTOR IS ASKED FOR ITS NAME, and if it has none there is no font to make: a descriptor without a name
 * is a request to a font registry for a face, and this tier has no registry to answer it (see below). */
CTFontRef CTFontCreateWithFontDescriptor(CTFontDescriptorRef descriptor, CGFloat size,
					 const CGAffineTransform *matrix)
{
	CFTypeRef name;
	CTFontRef font;

	if (descriptor == NULL) {
		return NULL;
	}
	name = CTFontDescriptorCopyAttribute(descriptor, kCTFontNameAttribute);
	if (name == NULL || CFGetTypeID(name) != CFStringGetTypeID()) {
		if (name != NULL) {
			CFRelease(name);
		}
		return NULL;
	}
	font = CTFontCreateWithName((CFStringRef)name, size, matrix);
	CFRelease(name);
	return font;
}

/* THE OPTIONS ARE THE FONT SERVER'S RATHER THAN THE CALLER'S WISHES, and this tier has no server to hand them
 * to: there is no font activation to prevent, and no system font to prefer, because a face is asked for BY
 * NAME. They are accepted and RECORDED here rather than silently obeyed, which is the difference between a
 * door that cannot do something and a door that pretends it did. */
CTFontRef CTFontCreateWithNameAndOptions(CFStringRef name, CGFloat size,
					 const CGAffineTransform *matrix, CTFontOptions options)
{
	(void)options;	/* no server exists here to receive them; see this file's head */
	return CTFontCreateWithName(name, size, matrix);
}

CTFontRef CTFontCreateWithFontDescriptorAndOptions(CTFontDescriptorRef descriptor, CGFloat size,
						   const CGAffineTransform *matrix, CTFontOptions options)
{
	(void)options;
	return CTFontCreateWithFontDescriptor(descriptor, size, matrix);
}

/* THE INVERSE OF CTFontCopyGraphicsFont, AND THE DOOR THAT MAKES A FONT OUT OF A FACE THIS TIER DID NOT
 * FIND ITSELF: size, matrix and an optional descriptor to hold beside it. */
CTFontRef CTFontCreateWithGraphicsFont(CGFontRef graphicsFont, CGFloat size,
				       const CGAffineTransform *matrix, CTFontDescriptorRef attributes)
{
	return (CTFontRef)ct_font_alloc(graphicsFont, size, matrix, attributes);
}

/* A COPY DIFFERS IN SIZE, MATRIX OR DESCRIPTOR — and this library can only honour a descriptor whose name
 * names a face, because a face comes from Core Graphics BY NAME here. */
CTFontRef CTFontCreateCopyWithAttributes(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
					 CTFontDescriptorRef attributes)
{
	const struct __CTFont *original = (const struct __CTFont *)font;
	CGAffineTransform m;

	if (original == NULL) {
		return NULL;
	}
	m = (matrix != NULL) ? *matrix : original->matrix;
	if (size <= 0.0) {
		size = original->size;
	}
	if (attributes != NULL) {
		CTFontRef copy = CTFontCreateWithFontDescriptor(attributes, size, &m);

		if (copy != NULL) {
			return copy;
		}
	}
	return (CTFontRef)ct_font_alloc(original->graphics, size, &m,
					(attributes != NULL) ? attributes : original->descriptor);
}

/* =================================================================================================
 * WHAT A FONT SAYS ABOUT ITSELF.
 * =============================================================================================== */
CGFontRef CTFontCopyGraphicsFont(CTFontRef font, CTFontDescriptorRef *attributes)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	if (f == NULL) {
		return NULL;
	}
	/* THE NAME `Copy` PUTS THE BURDEN ON THE CALLER, which is why the out-parameter is RETAINED. */
	if (attributes != NULL) {
		*attributes = (f->descriptor == NULL) ? NULL
						      : (CTFontDescriptorRef)CFRetain(f->descriptor);
	}
	return CGFontRetain(f->graphics);
}

CTFontDescriptorRef CTFontCopyFontDescriptor(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	if (f == NULL || f->descriptor == NULL) {
		return NULL;
	}
	return (CTFontDescriptorRef)CFRetain(f->descriptor);
}

CFTypeRef CTFontCopyAttribute(CTFontRef font, CFStringRef attribute)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	if (f == NULL || f->descriptor == NULL) {
		return NULL;
	}
	return CTFontDescriptorCopyAttribute(f->descriptor, attribute);
}

CGFloat CTFontGetSize(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : f->size;
}

CGAffineTransform CTFontGetMatrix(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? CGAffineTransformIdentity : f->matrix;
}

/* =================================================================================================
 * THE NAMES. TWO OF THE EIGHTEEN HAVE A CORE GRAPHICS DOOR OF THEIR OWN; THE OTHER SIXTEEN ARE READ FROM THE
 * FACE'S `name` TABLE, whose entries are numbered by the OpenType specification. That numbering is PUBLISHED,
 * which is why these are facts rather than guesses -- unlike the keys that ask for them.
 * =============================================================================================== */
static int ct_font_name_id(CFStringRef nameKey, bool *known)
{
	*known = true;
	if (nameKey == NULL) {
		*known = false;
		return -1;
	}
	if (CFEqual(nameKey, kCTFontCopyrightNameKey))		return 0;
	if (CFEqual(nameKey, kCTFontFamilyNameKey))		return 1;
	if (CFEqual(nameKey, kCTFontSubFamilyNameKey))		return 2;
	/* APPLE'S STYLE NAME AND ITS SUBFAMILY NAME ARE THE SAME ENTRY in the `name` table -- id 2 is "font
	 * subfamily name", which is the style -- so both keys read it. Recorded rather than silently split. */
	if (CFEqual(nameKey, kCTFontStyleNameKey))		return 2;
	if (CFEqual(nameKey, kCTFontUniqueNameKey))		return 3;
	if (CFEqual(nameKey, kCTFontFullNameKey))		return 4;
	if (CFEqual(nameKey, kCTFontVersionNameKey))		return 5;
	if (CFEqual(nameKey, kCTFontPostScriptNameKey))		return 6;
	if (CFEqual(nameKey, kCTFontTrademarkNameKey))		return 7;
	if (CFEqual(nameKey, kCTFontManufacturerNameKey))	return 8;
	if (CFEqual(nameKey, kCTFontDesignerNameKey))		return 9;
	if (CFEqual(nameKey, kCTFontDescriptionNameKey))	return 10;
	if (CFEqual(nameKey, kCTFontVendorURLNameKey))		return 11;
	if (CFEqual(nameKey, kCTFontDesignerURLNameKey))	return 12;
	if (CFEqual(nameKey, kCTFontLicenseNameKey))		return 13;
	if (CFEqual(nameKey, kCTFontLicenseURLNameKey))		return 14;
	if (CFEqual(nameKey, kCTFontSampleTextNameKey))		return 19;
	if (CFEqual(nameKey, kCTFontPostScriptCIDNameKey))	return 20;
	*known = false;
	return -1;
}

CFStringRef CTFontCopyName(CTFontRef font, CFStringRef nameKey)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	char buf[512];
	bool known;
	int id;
	int n;

	if (f == NULL) {
		return NULL;
	}
	/* THE TWO CORE GRAPHICS ALREADY ANSWERS. */
	if (nameKey != NULL && CFEqual(nameKey, kCTFontPostScriptNameKey)) {
		return CGFontCopyPostScriptName(f->graphics);
	}
	if (nameKey != NULL && CFEqual(nameKey, kCTFontFullNameKey)) {
		return CGFontCopyFullName(f->graphics);
	}
	id = ct_font_name_id(nameKey, &known);
	if (!known) {
		return NULL;
	}
	n = cg_font_name_entry(f->graphics, id, buf, sizeof buf);
	if (n <= 0) {
		return NULL;
	}
	return CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)buf, (CFIndex)n,
				       kCFStringEncodingUTF8, false);
}

CFStringRef CTFontCopyPostScriptName(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? NULL : CGFontCopyPostScriptName(f->graphics);
}

CFStringRef CTFontCopyFullName(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? NULL : CGFontCopyFullName(f->graphics);
}

/* A FAMILY NAME IS THE `name` TABLE'S ENTRY 1, and a DISPLAY NAME IS APPLE'S "name suitable to show a user",
 * which for a face that is not localized is its full name. Both go through the same door as everything else. */
CFStringRef CTFontCopyFamilyName(CTFontRef font)
{
	return CTFontCopyName(font, kCTFontFamilyNameKey);
}

CFStringRef CTFontCopyDisplayName(CTFontRef font)
{
	return CTFontCopyFullName(font);
}

/* LOCALIZATION NEEDS A LANGUAGE LIST THIS TIER DOES NOT HAVE, so this answers the name itself and sets the
 * matched language to NULL rather than leaving the caller's variable untouched -- the same choice, for the
 * same reason, as CTFontDescriptorCopyLocalizedAttribute. */
CFStringRef CTFontCopyLocalizedName(CTFontRef font, CFStringRef nameKey, CFStringRef *language)
{
	if (language != NULL) {
		*language = NULL;
	}
	return CTFontCopyName(font, nameKey);
}

/* =================================================================================================
 * THE GLYPHS.
 * =============================================================================================== */
bool CTFontGetGlyphsForCharacters(CTFontRef font, const UniChar characters[], CGGlyph glyphs[],
				  CFIndex count)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFIndex i;
	int mapped;

	if (f == NULL || characters == NULL || glyphs == NULL || count <= 0) {
		return false;
	}
	if (count > (CFIndex)(SIZE_MAX / sizeof(uint16_t))) {
		return false;
	}
	/* APPLE'S CONTRACT IS YES OR NO: "true if the font could encode all the characters". The mapping
	 * answers HOW MANY in one batch, and a slot that could not be mapped is left at zero -- `.notdef`'s
	 * index -- which is what Apple's own door puts there. */
	mapped = cg_font_glyphs_for_unichars(f->graphics, (const uint16_t *)characters, glyphs, (size_t)count);
	for (i = 0; i < count; i++) {
		if (glyphs[i] == 0) {
			return false;
		}
	}
	return (mapped == (int)count);
}

CGGlyph CTFontGetGlyphWithName(CTFontRef font, CFStringRef glyphName)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	if (f == NULL || glyphName == NULL) {
		return 0;
	}
	return CGFontGetGlyphWithGlyphName(f->graphics, glyphName);
}

/* =================================================================================================
 * THE METRICS. Every one of these is the FACE'S number in font units, scaled by the size over the em: a face
 * says how tall its ascender is in its own units, and a font says how many points that is.
 * =============================================================================================== */
static CGFloat ct_font_scale(const struct __CTFont *f)
{
	int upem = CGFontGetUnitsPerEm(f->graphics);

	return (upem > 0) ? (f->size / (CGFloat)upem) : 0.0;
}

CGFloat CTFontGetAscent(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : (CGFloat)CGFontGetAscent(f->graphics) * ct_font_scale(f);
}

CGFloat CTFontGetDescent(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : (CGFloat)CGFontGetDescent(f->graphics) * ct_font_scale(f);
}

CGFloat CTFontGetLeading(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : (CGFloat)CGFontGetLeading(f->graphics) * ct_font_scale(f);
}

CGFloat CTFontGetCapHeight(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : (CGFloat)CGFontGetCapHeight(f->graphics) * ct_font_scale(f);
}

CGFloat CTFontGetXHeight(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0.0 : (CGFloat)CGFontGetXHeight(f->graphics) * ct_font_scale(f);
}

unsigned CTFontGetUnitsPerEm(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0 : (unsigned)CGFontGetUnitsPerEm(f->graphics);
}

CFIndex CTFontGetGlyphCount(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	return (f == NULL) ? 0 : (CFIndex)CGFontGetNumberOfGlyphs(f->graphics);
}

CGRect CTFontGetBoundingBox(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CGRect box;
	CGFloat scale;

	if (f == NULL) {
		return CGRectNull;
	}
	box = CGFontGetFontBBox(f->graphics);
	scale = ct_font_scale(f);
	box.origin.x *= scale;
	box.origin.y *= scale;
	box.size.width *= scale;
	box.size.height *= scale;
	return box;
}

/* THE ADVANCES AND THE BOUNDING BOXES OF GLYPHS, which Core Graphics reports in font units for a batch of
 * glyphs, scaled here. THE VERTICAL ORIENTATION IS ANSWERED AS THE HORIZONTAL ONE RATHER THAN GUESSED AT: a
 * vertical advance comes from the face's vertical metrics, and this seam reports none, so a caller asking for
 * vertical gets the horizontal numbers and the door is recorded as owed. */
double CTFontGetAdvancesForGlyphs(CTFontRef font, CTFontOrientation orientation, const CGGlyph glyphs[],
				  CGSize advances[], CFIndex count)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	int *units = NULL;
	double total = 0.0;
	CFIndex i;
	CGFloat scale;

	if (f == NULL || glyphs == NULL || count <= 0) {
		return 0.0;
	}
	if (count > (CFIndex)(SIZE_MAX / sizeof(int))) {
		return 0.0;
	}
	(void)orientation;	/* vertical needs the face's vertical metrics; see above */
	units = (int *)malloc((size_t)count * sizeof(int));
	if (units == NULL) {
		return 0.0;
	}
	if (!CGFontGetGlyphAdvances(f->graphics, glyphs, (size_t)count, units)) {
		free(units);
		return 0.0;
	}
	scale = ct_font_scale(f);
	for (i = 0; i < count; i++) {
		CGFloat points = (CGFloat)units[i] * scale;

		if (advances != NULL) {
			advances[i] = CGSizeMake(points, 0.0);
		}
		total += points;
	}
	free(units);
	return total;
}

CGRect CTFontGetBoundingRectsForGlyphs(CTFontRef font, CTFontOrientation orientation,
				       const CGGlyph glyphs[], CGRect boundingRects[], CFIndex count)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CGRect box;
	CGRect overall;
	CFIndex i;
	CGFloat scale;

	overall = CGRectNull;
	if (f == NULL || glyphs == NULL || count <= 0) {
		return overall;
	}
	(void)orientation;
	if (boundingRects != NULL && !CGFontGetGlyphBBoxes(f->graphics, glyphs, (size_t)count, boundingRects)) {
		return overall;
	}
	scale = ct_font_scale(f);
	for (i = 0; i < count; i++) {
		if (boundingRects == NULL) {
			continue;
		}
		box = boundingRects[i];
		box.origin.x *= scale;
		box.origin.y *= scale;
		box.size.width *= scale;
		box.size.height *= scale;
		boundingRects[i] = box;
		overall = CGRectIsNull(overall) ? box : CGRectUnion(overall, box);
	}
	/* THE OVERALL RECT IS THE WHOLE RUN'S, and when the caller asked for no per-glyph rects there is
	 * nothing here to union -- which is the caller's choice and not an error. */
	if (boundingRects == NULL) {
		CGRect *one = (CGRect *)malloc((size_t)count * sizeof(CGRect));
		CGRect whole = CGRectNull;

		if (one == NULL) {
			return overall;
		}
		if (CGFontGetGlyphBBoxes(f->graphics, glyphs, (size_t)count, one)) {
			CFIndex k;

			for (k = 0; k < count; k++) {
				CGRect b = one[k];

				b.origin.x *= scale;
				b.origin.y *= scale;
				b.size.width *= scale;
				b.size.height *= scale;
				whole = CGRectIsNull(whole) ? b : CGRectUnion(whole, b);
			}
		}
		free(one);
		return whole;
	}
	return overall;
}

/* =================================================================================================
 * THE GLYPH AS GEOMETRY.
 *
 * APPLE'S OWN SENTENCE FIXES THE COMPOSITION ORDER, and it is why this is three transforms rather than one:
 * "The path will reflect the font point size, matrix, and transform parameter, IN THAT ORDER." So the
 * outline is scaled by the font's size, then carried through THE FONT'S OWN MATRIX -- the one thing in this
 * object nothing else in this file reads -- and then through the caller's `transform`, about which Apple
 * adds that it "will most commonly be used to provide a translation to the desired glyph origin".
 *
 * THE ENGINE'S OUTLINE DOOR ALREADY COMPOSES THE FIRST TWO. `cg_font_glyph_outline` takes the em and a
 * matrix and concatenates them itself, so handing it the size and the concatenation of the font's matrix
 * with the caller's transform produces size, matrix, transform -- in exactly that order, once. THE POINTS
 * COME BACK IN USER SPACE, Y-UP, RELATIVE TO THE ORIGIN: the seam applies no flip, which is what a CGPath
 * in Core Text's coordinate system is.
 *
 * A GLYPH WITH NO OUTLINE IS AN EMPTY PATH AND NOT NULL. A space, or a bitmap-only face, has nothing to
 * draw, which is not an error -- the seam says so and succeeds with no segments. NULL is kept for the case
 * Apple's own result line names, "an error occurred": a glyph the engine will not load at all.
 * =============================================================================================== */
struct ct_outline_path {
	CGMutablePathRef path;
};

static void ct_path_move_to(void *info, CGFloat x, CGFloat y)
{
	CGPathMoveToPoint(((struct ct_outline_path *)info)->path, NULL, x, y);
}

static void ct_path_line_to(void *info, CGFloat x, CGFloat y)
{
	CGPathAddLineToPoint(((struct ct_outline_path *)info)->path, NULL, x, y);
}

/* A CONIC IS ADDED AS A QUADRATIC, because a quadratic is the only curve a CGPath holds -- the same
 * conversion Core Graphics' own glyph path makes, for the same reason (see CGContext.c). */
static void ct_path_conic_to(void *info, CGFloat cx, CGFloat cy, CGFloat x, CGFloat y)
{
	CGPathAddQuadCurveToPoint(((struct ct_outline_path *)info)->path, NULL, cx, cy, x, y);
}

static void ct_path_cubic_to(void *info, CGFloat c1x, CGFloat c1y, CGFloat c2x, CGFloat c2y, CGFloat x,
			     CGFloat y)
{
	CGPathAddCurveToPoint(((struct ct_outline_path *)info)->path, NULL, c1x, c1y, c2x, c2y, x, y);
}

static void ct_path_close(void *info)
{
	CGPathCloseSubpath(((struct ct_outline_path *)info)->path);
}

CGPathRef CTFontCreatePathForGlyph(CTFontRef font, CGGlyph glyph, const CGAffineTransform *transform)
{
	static const cg_outline_sink sink = {
		ct_path_move_to, ct_path_line_to, ct_path_conic_to, ct_path_cubic_to, ct_path_close
	};
	const struct __CTFont *f = (const struct __CTFont *)font;
	struct ct_outline_path ctx;
	CGAffineTransform m;

	if (f == NULL) {
		return NULL;
	}
	/* SIZE, THEN THE FONT'S MATRIX, THEN THE CALLER'S TRANSFORM; and a NULL transform is
	 * CGAffineTransformIdentity, which is Apple's own sentence for that parameter. */
	m = f->matrix;
	if (transform != NULL) {
		m = CGAffineTransformConcat(m, *transform);
	}
	ctx.path = CGPathCreateMutable();
	if (ctx.path == NULL) {
		return NULL;
	}
	if (!cg_font_glyph_outline(f->graphics, glyph, f->size, m, &sink, &ctx)) {
		CGPathRelease((CGPathRef)ctx.path);
		return NULL;
	}
	/* RETAINED, WHICH IS WHAT APPLE'S RESULT LINE SAYS: `Create` hands the caller the +1 reference and
	 * nothing makes a second one here. */
	return (CGPathRef)ctx.path;
}

/* =================================================================================================
 * OWED DOORS, EACH WITH THE REASON IT CANNOT BE ANSWERED HERE. They are DECLARED in CTFont.h and NOT DEFINED,
 * so a caller who reaches for one gets a link error rather than a plausible wrong answer.
 *
 *   CTFontGetSymbolicTraits, CTFontCopyTraits     the seam reports no style flags, and deriving traits from a
 *                                                 subfamily STRING would be a guess wearing a fact's clothes.
 *   CTFontCopyCharacterSet, CFStringEncoding      needs every character the face covers: the inverse of the
 *   CTFontCopySupportedLanguages                   glyph step, over the whole cmap, plus the OS/2 and
 *                                                 name-table data that languages come from.
 *   CTFontCopyVariationAxes, CTFontCopyVariation  these are the `fvar` and `avar` tables -- font variation.
 *   CTFontCopyFeatures, CTFontCopyFeatureSettings  These are the `feat` table. No seam door reads a table.
 *   CTFontCopyAvailableTables, CTFontCopyTable    the same, and a raw table needs a table-access door.
 *   CTFontGetUnderlinePosition, ...Thickness      the `post` table; the seam reports no underline metrics.
 *   CTFontGetSlantAngle                           Apple's slant is the GLYPH-SPACE slant, not the italic
 *                                                 angle Core Graphics reports; they are different numbers.
 *   CTFontGetVerticalTranslationsForGlyphs        needs the vertical metrics the advances door also lacks.
 *   CTFontCreateUIFontForLanguage                 a ROLE is a request to a system that chooses a face.
 *   CTFontCreateForString                         needs a cascade and a fallback search.
 *   CTFontCreateCopyWithSymbolicTraits            needs a face with those traits to exist and be found.
 *   CTFontCreateCopyWithFamily                    needs the family's faces to be enumerable.
 *   CTFontGetPlatformFont, ...WithPlatformFont    these speak in ATS ids, and this tree has no ATS.
 *
 * AND THE TWO MATCHING DOORS ON CTFontDescriptorRef ARE OWED FOR A RELATED REASON: Apple defines their answer
 * as "based on the original descriptor and SYSTEM PRECEDENCE", and precedence is a property of a registry of
 * INSTALLED faces with a registration scope. The engine behind CGFont can enumerate and register faces
 * (cg_font_enumerate, cg_font_register, cg_font_is_registered), so the tier that answers them is buildable --
 * it is just not this one, because the ordering it must impose is Apple's and not something to invent.
 * =============================================================================================== */
