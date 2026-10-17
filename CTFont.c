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
#include <CoreText/CTFontTraits.h>
#include <CoreText/CTFontDescriptor_internal.h>
#include <CoreText/CTFont_internal.h>
/* THE MATCHING TIER'S WALK, WHICH THE FOUR CREATORS THAT ASK A REGISTRY USE AND DO NOT REIMPLEMENT: it is the
 * one place the candidates are gathered and put into this tier's order. */
#include <CoreText/CTFontMatching_internal.h>

#include <CoreFoundation/CFRuntime.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFCharacterSet.h>
#include <CoreGraphics/CGFont_internal.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* FOR `atan2`, WHICH THE SLANT DOOR NEEDS AND WHICH IS THE ONLY THING THIS FILE ASKS OF THE C LIBRARY'S MATHS.
 * The link is already there: `libagora-drawing` composes its own transforms and has needed the same library since
 * it drew its first glyph, so this is a header and not a new dependency. */
#include <math.h>

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
	if (font != NULL) {
		/* AND THE DESCRIPTOR THE FONT WAS ASKED WITH IS KEPT, WHICH THIS DOOR USED TO THROW AWAY -- A BUG AGAINST THIS
		 * FILE'S OWN STRUCT, whose field says "RETAINED: what this font was asked for" while the code above took the
		 * NAME out of the caller's descriptor and let the rest fall. WHAT IT COST WAS EVERY ATTRIBUTE THAT IS NOT A
		 * NAME: a variation instance, a feature setting, a trait request -- all of them discarded on the way in, and
		 * `CTFontCopyVariation` is simply the first door that could show it.
		 *
		 * THE MERGE IS THE CALLER'S ATTRIBUTES PLUS THE SIZE THE FONT ACTUALLY HAS, and the size comes from the FONT
		 * rather than from the parameter because the parameter is allowed to be 0.0, in which case the size the font
		 * resolved is the truth and 0.0 is not. A descriptor for a font should describe that font.
		 *
		 * THE FIELD IS ASSIGNED AFTER CREATION, WHICH IS SAFE HERE AND NOWHERE ELSE: this font has not been returned to
		 * anybody yet, so no other thread can reach it, and fonts are immutable once they are visible -- which is why
		 * nothing else in this file assigns this field at all. */
		struct __CTFont *created = (struct __CTFont *)font;
		CFDictionaryRef given = (CFDictionaryRef)CTFontDescriptorCopyAttributes(descriptor);
		CFMutableDictionaryRef asked = (given != NULL)
						       ? CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, given)
						       : NULL;
		CFNumberRef point_size;

		if (given != NULL) {
			CFRelease(given);
		}
		if (asked != NULL) {
			point_size = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &created->size);
			if (point_size != NULL) {
				CFDictionarySetValue(asked, kCTFontSizeAttribute, point_size);
				CFRelease(point_size);
			}
			if (created->descriptor != NULL) {
				CTFontDescriptorRef merged = CTFontDescriptorCreateWithAttributes(asked);

				if (merged != NULL) {
					CFRelease(created->descriptor);
					created->descriptor = merged;
				}
			}
			CFRelease(asked);
		}
	}
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
 * THE FOUR CREATORS THAT ASK A REGISTRY RATHER THAN A NAME. Each asks a question whose answer is A FACE THE
 * SYSTEM HAS -- "same family, these traits", "this family, my traits", "a face that can encode this string", "the
 * face that plays this role" -- and the registry they ask is the engine's: THE FACES THIS PROCESS HAS REGISTERED,
 * plus the directories the engine is configured with.
 *
 * AND THAT BOUNDARY IS STATED BEFORE THE DOORS ARE, BECAUSE IT CHANGES WHAT EACH OF THEM ANSWERS. Apple's four
 * read a SYSTEM-WIDE font store. MEASURED on this machine, a fresh process sees ZERO faces through
 * `cg_font_enumerate`, so the honest description of the answer here is "a face THIS PROCESS can see" -- the same
 * boundary `CTFontManager` already records about its one scope, and the same one the font service exists to
 * remove (FONT-SERVICE.md). The cases register the faces they ask about and PRINT that they did.
 *
 * AND THE ORDER IS THE MATCHING TIER'S, TAKEN RATHER THAN RESTATED: `CTFontMatchingCopyAvailableDescriptors`
 * hands back the candidates ALREADY SORTED BY FAMILY NAME -- this tier's recorded decision where Apple leaves the
 * order to "system precedence" -- so "the first face that satisfies this" is reproducible rather than whichever
 * the directory walk happened to reach first.
 * =============================================================================================== */

/* ONE WALK, USED BY ALL FOUR: each candidate is turned into a font BY NAME -- the route the suite measures every
 * run, and the only one a descriptor offers -- and the door's own test decides whether that font is the answer. A
 * candidate that cannot be made into a font is SKIPPED rather than fatal, which is the rule the matching tier
 * already applies to a listing door: the registry names FILES, and one the engine will not open is not a reason to
 * fail the others. */
static CTFontRef ct_font_from_descriptor(CTFontDescriptorRef candidate, CGFloat size,
					 const CGAffineTransform *matrix)
{
	CFStringRef name = (CFStringRef)CTFontDescriptorCopyAttribute(candidate, kCTFontNameAttribute);
	CTFontRef font;

	if (name == NULL || CFGetTypeID(name) != CFStringGetTypeID()) {
		if (name != NULL) {
			CFRelease(name);
		}
		return NULL;
	}
	font = CTFontCreateWithName(name, size, matrix);
	CFRelease(name);
	return font;
}

typedef bool (*ct_font_accept)(CTFontRef candidate, void *info);

static CTFontRef ct_font_search(CFArrayRef candidates, CGFloat size, const CGAffineTransform *matrix,
				ct_font_accept accept, void *info)
{
	CFIndex i, count = (candidates != NULL) ? CFArrayGetCount(candidates) : 0;

	for (i = 0; i < count; i++) {
		CTFontRef candidate = ct_font_from_descriptor(
			(CTFontDescriptorRef)CFArrayGetValueAtIndex(candidates, i), size, matrix);

		if (candidate == NULL) {
			continue;
		}
		if (accept(candidate, info)) {
			return candidate;
		}
		CFRelease(candidate);
	}
	return NULL;
}

/* APPLE'S TWO DEFAULTING RULES, WHICH THESE COPY DOORS SHARE VERBATIM: "If 0.0 is specified, the original font's
 * size will be preserved", and the same sentence for the matrix. Stated once rather than four times, because four
 * copies of a defaulting rule are four chances to keep a 0.0. */
static void ct_font_copy_defaults(const struct __CTFont *original, CGFloat *size, const CGAffineTransform **matrix)
{
	if (*size <= 0.0) {
		*size = original->size;
	}
	if (*matrix == NULL) {
		*matrix = &original->matrix;
	}
}

/* THE STYLE BITS A FAMILY CAN HONOUR, which is the mask `CTFontCreateCopyWithFamily` compares under. Bold, italic,
 * condensed and expanded are REQUESTS any family can satisfy; monospace and the stylistic class are properties OF
 * the family -- asking DejaVu Sans for a fixed-pitch face asks for something no face in it can be. Apple prints
 * "with the original traits" and no rule for which of them cross a family, SO THIS IS A DECISION, recorded here
 * and asserted by the case rather than left to be discovered from a NULL. */
#define ct_font_style_mask (kCTFontBoldTrait | kCTFontItalicTrait | kCTFontCondensedTrait | kCTFontExpandedTrait)

struct ct_traits_request {
	CFStringRef family;		/* NULL when the original named no family */
	CTFontSymbolicTraits value;	/* the traits the masked bits should END UP carrying */
	CTFontSymbolicTraits mask;	/* the bits the caller asked to change */
};

static bool ct_accept_traits(CTFontRef candidate, void *info)
{
	const struct ct_traits_request *want = (const struct ct_traits_request *)info;
	CFStringRef family = NULL;
	bool ok = true;

	if (want->family != NULL) {
		family = CTFontCopyFamilyName(candidate);
		ok = (family != NULL) && CFEqual(family, want->family);
		if (family != NULL) {
			CFRelease(family);
		}
	}
	return ok && (CTFontGetSymbolicTraits(candidate) & want->mask) == (want->value & want->mask);
}

/* SAME FAMILY, THESE TRAITS. Apple's own sentence: "Returns a new font reference in the same family with the given
 * symbolic traits, or NULL if none found in the system." The two parameters are spelled out beside it -- the VALUE
 * gives the traits named by the MASK their desired setting, which "can allow for trait REMOVAL as well as
 * addition".
 *
 * SO THE REQUEST IS COMPUTED RATHER THAN GUESSED: the font's own traits with the mask's bits REPLACED by the
 * value's, which is what makes clearing bold a question an answer exists for. ONLY THE MASKED BITS ARE COMPARED,
 * because those are the only bits the caller asked to change -- comparing the whole value would fail a request to
 * clear bold over some bit nobody mentioned.
 *
 * AND A MASK OF ZERO IS NOT A SEARCH: it asks for no change, and the answer is the font's own face -- the copy
 * door with no attributes -- rather than a walk that would break a tie it was never asked to break. */
CTFontRef CTFontCreateCopyWithSymbolicTraits(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
					     CTFontSymbolicTraits symTraitValue, CTFontSymbolicTraits symTraitMask)
{
	const struct __CTFont *original = (const struct __CTFont *)font;
	struct ct_traits_request want;
	CFArrayRef candidates;
	CTFontRef found;

	if (original == NULL) {
		return NULL;
	}
	if (symTraitMask == 0) {
		return CTFontCreateCopyWithAttributes(font, size, matrix, NULL);
	}
	ct_font_copy_defaults(original, &size, &matrix);
	want.value = (CTFontGetSymbolicTraits(font) & ~symTraitMask) | (symTraitValue & symTraitMask);
	want.mask = symTraitMask;
	want.family = CTFontCopyFamilyName(font);
	candidates = CTFontMatchingCopyAvailableDescriptors();
	found = ct_font_search(candidates, size, matrix, ct_accept_traits, &want);
	if (candidates != NULL) {
		CFRelease(candidates);
	}
	if (want.family != NULL) {
		CFRelease(want.family);
	}
	return found;
}

struct ct_family_request {
	CFStringRef family;
	CTFontSymbolicTraits traits;
};

static bool ct_accept_family(CTFontRef candidate, void *info)
{
	const struct ct_family_request *want = (const struct ct_family_request *)info;
	CFStringRef family = CTFontCopyFamilyName(candidate);
	bool ok = (family != NULL) && CFEqual(family, want->family);

	if (family != NULL) {
		CFRelease(family);
	}
	return ok && (CTFontGetSymbolicTraits(candidate) & ct_font_style_mask) == (want->traits & ct_font_style_mask);
}

/* THIS FAMILY, MY TRAITS. Apple: "Returns a new font reference with the original traits in the given family. NULL
 * if non found in the system." THE COMPARISON IS UNDER `ct_font_style_mask` AND NOT OVER THE WHOLE VALUE, which
 * is the decision that makes the door answer at all -- see that mask's own comment. A family with several faces
 * under the same style (DejaVu Sans Mono's Regular and its Oblique are not the same request) is decided by the
 * TIER'S ORDER, which is declared above rather than left to the walk. */
CTFontRef CTFontCreateCopyWithFamily(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
				     CFStringRef family)
{
	const struct __CTFont *original = (const struct __CTFont *)font;
	struct ct_family_request want;
	CFArrayRef candidates;
	CTFontRef found = NULL;

	if (original == NULL || family == NULL) {
		return NULL;
	}
	ct_font_copy_defaults(original, &size, &matrix);
	want.family = family;
	want.traits = CTFontGetSymbolicTraits(font);
	candidates = CTFontMatchingCopyAvailableDescriptors();
	found = ct_font_search(candidates, size, matrix, ct_accept_family, &want);
	if (candidates != NULL) {
		CFRelease(candidates);
	}
	return found;
}

struct ct_cascade_request {
	const UniChar *characters;
	CFIndex count;
};

static bool ct_accept_covers(CTFontRef candidate, void *info)
{
	const struct ct_cascade_request *want = (const struct ct_cascade_request *)info;
	CGGlyph *glyphs = (CGGlyph *)malloc((size_t)want->count * sizeof(CGGlyph));
	bool covered;

	if (glyphs == NULL) {
		return false;
	}
	covered = CTFontGetGlyphsForCharacters(candidate, want->characters, glyphs, want->count);
	free(glyphs);
	return covered;
}

/* A FACE THAT CAN ENCODE THIS STRING, AND APPLE'S TWO HALVES ARE BOTH ANSWERED: "If the current font is capable of
 * encoding the string range then it will be RETAINED and returned" -- so the first question is asked of the font
 * the caller already has, and only a font that fails it sends the door to the registry. THE TEST IS THIS
 * LIBRARY'S OWN GLYPH DOOR, which answers true only when EVERY character mapped, so "capable of encoding" is
 * decided by the same fact a caller would use.
 *
 * AND THE RANGE RULE IS THE ONE THIS LIBRARY ALREADY APPLIES ELSEWHERE: a length of 0 continues from the start
 * index to the END OF THE STRING, and a range running past the end is CLAMPED rather than read -- the rule
 * `ct_run_clamp` carries, applied here to a string rather than to a run.
 *
 * AND WHAT APPLE LEAVES UNSAID IS SAID HERE: the answer comes from "the CASCADE LIST of the current font", which
 * in this tier is the caller's own `kCTFontCascadeListAttribute` if the font carries one and the REGISTRY
 * otherwise -- the walk above -- in the tier's order. Apple's cascade list is a system property with an order
 * nobody prints, which is the same wall the matching tier hit and the same recorded answer. */
/* **THE PRIVATE USE AREA IS NEVER SUBSTITUTED, AND THAT IS BOTH THE CORRECT ANSWER AND THE FAST ONE.** A code
 * point in U+E000..U+F8FF means whatever THE FONT says it means, so no other face's glyph is a substitute for it;
 * Unicode's own rule is that Private Use code points are not interoperable, and Apple's cascade does not fall back
 * for them either — the character stays in the original face and draws `.notdef`.
 *
 * MEASURED, and this is why it exists: drawing U+F700 — the up arrow FUNCTION KEY, which a menu writes as the key
 * equivalent for Command-Up — took **8.2 seconds per draw** and inked the wrong glyph, because the search below
 * enumerates EVERY installed font and loads each one to test coverage. A menu with one such row took sixteen
 * seconds to paint, and a menu bar is full of function-key shortcuts.
 */
static bool ct_is_private_use(UniChar c)
{
	return c >= 0xE000 && c <= 0xF8FF;
}

CTFontRef CTFontCreateForString(CTFontRef currentFont, CFStringRef string, CFRange range)
{
	const struct __CTFont *original = (const struct __CTFont *)currentFont;
	CFIndex length, i;
	CFRange asked;
	UniChar *characters;
	CGGlyph *glyphs;
	struct ct_cascade_request want;
	CFArrayRef cascade;
	CTFontRef found = NULL;

	if (original == NULL || string == NULL) {
		return NULL;
	}
	length = CFStringGetLength(string);
	asked = range;
	if (asked.location < 0) {
		asked.location = 0;
	}
	if (asked.location > length) {
		asked.location = length;
	}
	if (asked.length <= 0 || asked.location + asked.length > length) {
		asked.length = length - asked.location;
	}
	if (asked.length == 0) {
		return NULL;	/* an empty range asks for nothing, and there is no face that answers nothing */
	}
	characters = (UniChar *)malloc((size_t)asked.length * sizeof(UniChar));
	glyphs = (CGGlyph *)malloc((size_t)asked.length * sizeof(CGGlyph));
	if (characters == NULL || glyphs == NULL) {
		free(characters);
		free(glyphs);
		return NULL;
	}
	CFStringGetCharacters(string, asked, characters);
	if (CTFontGetGlyphsForCharacters(currentFont, characters, glyphs, asked.length)) {
		free(characters);
		free(glyphs);
		return (CTFontRef)CFRetain(currentFont);	/* Apple's own first half, word for word */
	}
	/* AND A STRING THAT IS ENTIRELY PRIVATE USE IS ANSWERED AT ONCE, WITH NO FACE: the caller keeps its own, which
	 * draws `.notdef` — the honest answer, and the one that does not walk the font list. */
	{
		CFIndex private_use = 0;

		for (i = 0; i < asked.length; i++) {
			if (ct_is_private_use(characters[i])) {
				private_use++;
			}
		}
		if (private_use == asked.length) {
			free(characters);
			free(glyphs);
			return NULL;
		}
	}
	want.characters = characters;
	want.count = asked.length;

	/* THE CALLER'S OWN CASCADE LIST FIRST, WHICH IS WHAT APPLE'S SENTENCE NAMES, and the registry after it. */
	cascade = (original->descriptor != NULL)
			? (CFArrayRef)CTFontDescriptorCopyAttribute(original->descriptor, kCTFontCascadeListAttribute)
			: NULL;
	if (cascade != NULL && CFGetTypeID(cascade) == CFArrayGetTypeID() && CFArrayGetCount(cascade) > 0) {
		CFMutableArrayRef kept = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

		if (kept != NULL) {
			for (i = 0; i < CFArrayGetCount(cascade); i++) {
				CFTypeRef entry = CFArrayGetValueAtIndex(cascade, i);

				if (CFGetTypeID(entry) == CTFontDescriptorGetTypeID()) {
					CFArrayAppendValue(kept, entry);
				}
			}
			found = ct_font_search(kept, original->size, &original->matrix, ct_accept_covers, &want);
			CFRelease(kept);
		}
	}
	if (cascade != NULL) {
		CFRelease(cascade);
	}
	if (found == NULL) {
		CFArrayRef candidates = CTFontMatchingCopyAvailableDescriptors();

		found = ct_font_search(candidates, original->size, &original->matrix, ct_accept_covers, &want);
		if (candidates != NULL) {
			CFRelease(candidates);
		}
	}
	free(characters);
	free(glyphs);
	return found;
}

struct ct_role_request {
	bool fixed_pitch;
};

/* **THE UI FAMILIES, IN THE ORDER THIS TIER WOULD LIKE THEM — AND THE THEME ROW THAT WAS RECORDED AS OWED.**
 * The role matcher used to take the first regular face in FAMILY-NAME order, which on this host is `C059-Roman`:
 * a URW SERIF, correct and plainly not a user interface. What it said in its own defence was that "which regular
 * family a UI uses is genuinely a theme, and this tier has no such setting" — which was true, and is no longer the
 * answer, because a theme has to be SOMEWHERE and this is the only tier that knows what a UI font is for.
 *
 * `Inter` first, by request. The rest are the faces a Linux or BSD desktop actually has, in the order a person
 * would pick them, so that a machine without Inter still gets a SANS face rather than a serif one. */
static const char *const ct_ui_families[] = {
	"Inter",
	"Noto Sans",
	"Cantarell",
	"Lato",
	"DejaVu Sans",
	"Liberation Sans",
	"FreeSans",
};

/* **AND IT IS OVERRIDABLE WITHOUT A REBUILD**, because a preference list nobody can argue with is a hardcoded
 * answer wearing a longer name. `AGORA_UI_FONT` is a colon-separated list, tried in order, and it REPLACES the
 * default rather than adding to it — a caller that names a family wants that family, not that family and then
 * whatever this file happens to like. */
static bool ct_ui_name_is_in(CFStringRef family, const char *list)
{
	char name[256];
	char wanted[512];
	char *one;
	size_t i;

	if (family == NULL) {
		return false;
	}
	if (!CFStringGetCString(family, name, (CFIndex)sizeof name, kCFStringEncodingUTF8) || name[0] == '\0') {
		return false;
	}
	if (list != NULL) {
		snprintf(wanted, sizeof wanted, "%s", list);
		for (one = strtok(wanted, ":"); one != NULL; one = strtok(NULL, ":")) {
			if (strcasecmp(one, name) == 0) {
				return true;
			}
		}
		return false;
	}
	for (i = 0; i < sizeof ct_ui_families / sizeof ct_ui_families[0]; i++) {
		if (strcasecmp(ct_ui_families[i], name) == 0) {
			return true;
		}
	}
	return false;
}

/* **THE Nth FAMILY TO TRY: THE CALLER'S LIST FIRST, AND THEN THIS FILE'S.** They are one sequence rather than
 * two alternatives, and the reason is what a caller means by naming a family: "this one, please" — not "this one,
 * and if it is not installed then anything at all". MEASURED with the lists kept apart: `AGORA_UI_FONT=Nonexistent`
 * fell past this file's whole preference list and answered `C059-Roman`, a URW SERIF, which is the one outcome the
 * list exists to prevent. A caller who names a family that is not here gets what a caller who names nothing gets. */
static bool ct_ui_family_at(int index, char *out, size_t size)
{
	const char *env = getenv("AGORA_UI_FONT");
	const int defaults = (int)(sizeof ct_ui_families / sizeof ct_ui_families[0]);
	const char *p;
	size_t len;

	if (env != NULL && env[0] != '\0') {
		int count = 1;		/* a non-empty list has one more field than it has separators */
		const char *q;

		for (q = env; *q != '\0'; q++) {
			if (*q == ':') {
				count++;
			}
		}
		if (index < count) {
			p = env;
			/* **ADVANCE exactly `index` SEPARATORS — NO MORE, NO FEWER, AND THE FIRST VERSION OF THIS GOT IT
			 * BACKWARDS.** It stepped `count - index - 1` times, which for a two-entry list and index 0 landed on
			 * the SECOND entry and for a three-entry list landed on the last; the results only looked right
			 * because the lists this was tried with happened to be short. This is a loop that must run `index`
			 * times, and writing it as a `while` that decrements its own counter is how it came to run another
			 * number instead. */
			while (index-- > 0 && (p = strchr(p, ':')) != NULL) {
				p++;
			}
			len = strcspn(p, ":");
			if (len >= size) {
				len = size - 1;
			}
			memcpy(out, p, len);
			out[len] = '\0';
			return out[0] != '\0';
		}
		index -= count;
	}
	if (index >= defaults) {
		return false;
	}
	snprintf(out, size, "%s", ct_ui_families[index]);
	return true;
}

/* **THE FIRST REGULAR FACE OF A NAMED FAMILY, ASKED OF THE DESCRIPTORS RATHER THAN OF THE FONTS.** Every candidate
 * carries its family as an ATTRIBUTE, so a family nobody wants costs a dictionary lookup instead of opening a font
 * file off the disk — WHICH IS WHAT MAKES TRYING A LIST OF FAMILIES AFFORDABLE rather than seven walks through every
 * font on the host. That distinction is the whole reason this function exists instead of a loop around
 * `ct_font_search`: the acceptance predicate there is handed a FONT, and by then the file has already been read. */
static CTFontRef ct_font_search_family(CFArrayRef candidates, CGFloat size, const CGAffineTransform *matrix,
				       const char *family)
{
	CFIndex i, count = (candidates != NULL) ? CFArrayGetCount(candidates) : 0;

	for (i = 0; i < count; i++) {
		CTFontDescriptorRef descriptor = (CTFontDescriptorRef)CFArrayGetValueAtIndex(candidates, i);
		CFStringRef got = (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
		char name[256];
		bool matches = false;
		CTFontRef font;

		if (got != NULL) {
			matches = CFStringGetCString(got, name, (CFIndex)sizeof name, kCFStringEncodingUTF8) &&
				  strcasecmp(name, family) == 0;
			CFRelease(got);
		}
		if (!matches) {
			continue;
		}
		font = ct_font_from_descriptor(descriptor, size, matrix);
		if (font == NULL) {
			continue;
		}
		if ((CTFontGetSymbolicTraits(font) & (kCTFontBoldTrait | kCTFontItalicTrait)) == 0) {
			return font;
		}
		CFRelease(font);
	}
	return NULL;
}

static bool ct_accept_role(CTFontRef candidate, void *info)
{
	const struct ct_role_request *want = (const struct ct_role_request *)info;

	/* THE ONE ROLE THAT CARRIES A REQUEST RATHER THAN A PLACE: the fixed-pitch role asks for a fact about the
	 * face, which `CTFontGetSymbolicTraits` reads out of the tables. */
	CTFontSymbolicTraits traits;
	if (want->fixed_pitch) {
		return (CTFontGetSymbolicTraits(candidate) & kCTFontMonoSpaceTrait) != 0;
	}
	/* **AND EVERY OTHER ROLE WANTS A REGULAR FACE.** The answer here was `true`, on the reasoning that "in a
	 * tier with no theme the first face in the tier's order IS that answer" — and the tier's order is FAMILY NAME,
	 * so a plain role was handed `C059-BdIta`, a BOLD ITALIC chosen because C sorts early. A bold italic for a
	 * plain role is not a theming decision; it is the absence of one. The emphasized roles are unaffected, because
	 * `CTFontCreateUIFontForLanguage` gives them their weight by copying the base face with the bold trait.
	 *
	 * WHAT REMAINS IS GENUINELY A THEME AND IS NOT INVENTED HERE: WHICH regular family a UI uses. This tier has no
	 * such setting, and a hardcoded `DejaVu Sans` would be this library choosing a distribution's fonts. */
	traits = CTFontGetSymbolicTraits(candidate);
	if ((traits & (kCTFontBoldTrait | kCTFontItalicTrait)) != 0) {
		return false;
	}
	/* **AND THE FIRST PASS ASKS FOR A FAMILY THIS TIER WOULD CHOOSE, THE SECOND FOR ANY REGULAR FACE.** A
	 * preference that could not be declined would not be a preference — a host with none of these families still
	 * gets a UI font, and it gets the first regular one rather than none at all. */
	/* **AND ANY REGULAR FACE IS THE ANSWER ONCE THE LIST HAS BEEN WALKED AND CAME BACK EMPTY.** The walk lives in
	 * `CTFontCreateUIFontForLanguage`, which is the only place that knows the ORDER; what is left here is the one
	 * fact this door can decide alone, and a preference it cannot see is not one it may invent. */
	return true;
}

/* A ROLE, WHICH IS A REQUEST TO A SYSTEM THAT CHOOSES A FACE -- AND THE DECISION IS RECORDED RATHER THAN PRETENDED
 * AWAY. Apple: "Returns the special UI font for the given language and UI type." There is no UI here to have a
 * font, and what Agora's desktop will put on its buttons is not this library's to invent, so THE ROLES ARE
 * ANSWERED AGAINST THE FACES THE REGISTRY HAS, in the matching tier's order, and the mapping is written down:
 *
 *   kCTFontNoFontType            is not a role -- it is Apple's own value for "none" -- and answers NULL.
 *   kCTFontUserFixedPitchFontType  asks for the MONOSPACED face, which is a TRAIT read out of the tables, so the
 *                                  role is answered by a fact rather than by a face this library decided to like.
 *   the EMPHASIZED roles          (System, SmallSystem, MiniSystem and SystemDetail) ask for the base face BOLD,
 *                                  THROUGH `CTFontCreateCopyWithSymbolicTraits` -- so an emphasized role is
 *                                  answered by the same search a caller could make, and a system with no bold face
 *                                  answers NULL rather than handing back a plain face under a bold role's name.
 *   every other role              asks for the FIRST face in the registry order.
 *   a value that is not one of    answers NULL. Apple's enum stops at 26, and 27 is not a role.
 *   Apple's roles
 *
 * AND THE SIZE DEFAULT IS THE DOCUMENTED ONE: "If 0.0 is specified, the default size for the requested uiType is
 * used" -- and Apple prints NO per-role size anywhere in the era's headers, so a 0.0 becomes 12.0, which is the
 * default this library already applies to a font made without a size.
 *
 * AND THE LANGUAGE IS ACCEPTED AND CHANGES NOTHING, RECORDED RATHER THAN LEFT TO BE DISCOVERED: choosing a face
 * for a localization needs the languages a face supports, and that door is still owed
 * (`CTFontCopySupportedLanguages`). A caller passing a language gets the answer a caller passing NULL gets, and
 * this paragraph is why. */
CTFontRef CTFontCreateUIFontForLanguage(CTFontUIFontType uiType, CGFloat size, CFStringRef language)
{
	struct ct_role_request want;
	CFArrayRef candidates;
	CTFontRef base;
	bool emphasized;

	(void)language;	/* recorded above: no localized face selection exists in this tier */
	if (uiType == (CTFontUIFontType)kCTFontNoFontType || uiType > (CTFontUIFontType)kCTFontControlContentFontType) {
		return NULL;
	}
	if (size <= 0.0) {
		size = 12.0;
	}
	emphasized = (uiType == kCTFontEmphasizedSystemFontType) ||
		     (uiType == kCTFontSmallEmphasizedSystemFontType) ||
		     (uiType == kCTFontMiniEmphasizedSystemFontType) ||
		     (uiType == kCTFontEmphasizedSystemDetailFontType);
	want.fixed_pitch = (uiType == kCTFontUserFixedPitchFontType);
	candidates = CTFontMatchingCopyAvailableDescriptors();
	base = NULL;
	if (!want.fixed_pitch) {
		/* **THE LIST IS WALKED IN ORDER, WHICH IS THE ONLY THING THAT MAKES IT A PREFERENCE.** MEASURED with a
		 * two-pass search that asked "is this family one of ours": the system font came back as `Cantarell`
		 * while `Inter` was installed and first in the list — because the candidate list is sorted by FAMILY NAME
		 * and `Cantarell` sorts first. A set has no order; a list does, and this is why the door takes one. */
		char family[256];
		int i;

		for (i = 0; base == NULL && ct_ui_family_at(i, family, sizeof family); i++) {
			base = ct_font_search_family(candidates, size, NULL, family);
		}
	}
	if (base == NULL) {
		/* AND ONLY THEN ANY REGULAR FACE AT ALL, so that a host with none of these families still gets a UI font
		 * rather than none — the preference being declined rather than enforced. */
		base = ct_font_search(candidates, size, NULL, ct_accept_role, &want);
	}
	if (candidates != NULL) {
		CFRelease(candidates);
	}
	if (base == NULL || !emphasized) {
		return base;
	}
	{
		CTFontRef bold = CTFontCreateCopyWithSymbolicTraits(base, size, NULL, kCTFontBoldTrait,
								    kCTFontBoldTrait);

		CFRelease(base);
		return bold;	/* NULL when no face of that family is bold, as this door's comment states */
	}
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

/* A SIGNED 16-BIT VALUE OUT OF A TABLE, IN THE BYTE ORDER sfnt SPECIFIES -- BIG-ENDIAN, WHICH IS THE ONE THING ABOUT
 * FONT TABLES THAT IS EASY TO GET WRONG AND IMPOSSIBLE TO NOTICE: a little-endian read of a two-byte field gives a
 * PLAUSIBLE number rather than an error, so nothing downstream would complain. A SHORT TABLE ANSWERS 0, which is what a
 * missing field would say too, and a caller cannot tell the two apart -- the same choice the seam's own table door makes
 * about a missing table. */
static int fn_table_s16(CFDataRef table, CFIndex offset)
{
	const UInt8 *bytes = (table != NULL) ? CFDataGetBytePtr(table) : NULL;
	unsigned raw;

	if (bytes == NULL || offset < 0 || offset + 2 > CFDataGetLength(table)) {
		return 0;
	}
	raw = ((unsigned)bytes[offset] << 8) | (unsigned)bytes[offset + 1];
	/* SIGN EXTENDED BY HAND RATHER THAN THROUGH A TYPEDEF: the two bytes are big-endian, but their SIGN is the
	 * format's, and `post`'s underline position is NEGATIVE for a baseline offset in the usual case. */
	return (raw >= 0x8000u) ? (int)raw - 0x10000 : (int)raw;
}

/* THE `post` METRICS, WHICH ARE THE FIRST DOORS IN THIS LIBRARY TO READ A TABLE AND INTERPRET IT.
 *
 * AND THE INTERPRETATION IS SMALL ON PURPOSE: `underlinePosition` and `underlineThickness` sit at FIXED OFFSETS 8 AND 10
 * IN EVERY VERSION OF THE `post` TABLE -- version 1 and 2 differ in what FOLLOWS them -- so there is no version check
 * to get wrong, unlike `OS/2`, whose version decides whether a field exists at all.
 *
 * THE SIGN IS THE TABLE'S AND THIS DOOR DOES NOT INVERT IT: a negative `underlinePosition` places the line BELOW the
 * baseline, which is the `post` table's own convention and the way the value is written down. Apple's header says the
 * door returns "the scaled underline position" and says nothing about a sign, so inverting it would be a guess about a
 * convention neither document states -- and a caller who wants the other sign has an answer that is one subtraction
 * away. AND "SCALED" IS THE POINT SIZE, WHICH IS THIS LIBRARY'S RULE FOR EVERY METRIC: the matrix moves GLYPHS and not
 * metrics (see the font object's own fields), so these are the font units times `ct_font_scale`, like every other
 * metric here. */
/* AN UNSIGNED 16-BIT VALUE AND AN UNSIGNED 32-BIT ONE, the two widths the trait tables need, in the same BIG-ENDIAN
 * order `fn_table_s16` explains. Two readers rather than one fiddly one: C has no template, and a variadic reader would
 * move a width mistake from here to a call site. */
static unsigned fn_table_u16(CFDataRef table, CFIndex offset)
{
	const UInt8 *bytes = (table != NULL) ? CFDataGetBytePtr(table) : NULL;

	if (bytes == NULL || offset < 0 || offset + 2 > CFDataGetLength(table)) {
		return 0;
	}
	return ((unsigned)bytes[offset] << 8) | (unsigned)bytes[offset + 1];
}

static unsigned fn_table_u32(CFDataRef table, CFIndex offset)
{
	const UInt8 *bytes = (table != NULL) ? CFDataGetBytePtr(table) : NULL;

	if (bytes == NULL || offset < 0 || offset + 4 > CFDataGetLength(table)) {
		return 0;
	}
	return ((unsigned)bytes[offset] << 24) | ((unsigned)bytes[offset + 1] << 16)
	     | ((unsigned)bytes[offset + 2] << 8) | (unsigned)bytes[offset + 3];
}

/* AND A 16.16 FIXED-POINT NUMBER, WHICH IS HOW THE `fvar` TABLE STATES AN AXIS BOUND AND WHICH IS NOT A WIDTH OF
 * INTEGER: the two halves are a SIGNED integer part and an UNSIGNED fraction, so the sign belongs to the whole
 * number and not to the second half -- -1.5 is 0xFFFF8000. THE SIGN IS EXTENDED BEFORE THE SCALING, because
 * dividing the unsigned word by 65536 would turn every negative bound into a large positive one, which is the same
 * class of mistake `fn_table_s16` exists to avoid and a louder one to notice. */
static double fn_table_fixed(CFDataRef table, CFIndex offset)
{
	unsigned raw = fn_table_u32(table, offset);
	double whole = (raw >= 0x80000000u) ? ((double)raw - 4294967296.0) : (double)raw;

	return whole / 65536.0;
}

/* THE BITS THIS DOOR READS, NAMED FROM THE FORMAT AND NOT FROM A CONVENTION: `fsSelection` and `macStyle` have carried
 * weight and slope in these positions since the format began. */
#define fn_os2_fs_italic      0x0001u
#define fn_os2_fs_bold        0x0020u
#define fn_head_mac_bold      0x0001u
#define fn_head_mac_italic    0x0002u
#define fn_head_mac_condensed 0x0020u
#define fn_head_mac_expanded  0x0040u

/* THE SYMBOLIC TRAITS, WHICH THIS LIBRARY PREVIOUSLY REFUSED TO ANSWER AT ALL.
 *
 * THE OWED REASON WAS "the seam reports no style flags, and deriving traits from a subfamily STRING would be a guess
 * wearing a fact's clothes" -- AND THE SECOND HALF WAS A REFUSAL RATHER THAN A SHORTAGE: the guess is still refused, and
 * THE FACT WAS IN THREE TABLES THE WHOLE TIME. This door reads them -- `OS/2.fsSelection`, `head.macStyle` and
 * `post.isFixedPitch` -- each at a fixed offset, and each present in every version of its own table for the fields read
 * here, so there is no version check to get wrong.
 *
 * AND WHERE THE FORMAT STATES THE SAME THING TWICE, EITHER SOURCE SETTING IT SETS THE TRAIT: `fsSelection` and
 * `macStyle` both mark bold and italic, and a face that says so in either is saying so. THE TWO WIDTH SOURCES ARE
 * HANDLED THE SAME WAY, AND `usWidthClass` IS CONSULTED FOR THE FIRST TIME HERE: 1 to 4 is condensed, 6 to 9 is
 * expanded, and 5 IS THE MIDDLE AND SAYS NEITHER -- which is the field's OWN meaning rather than a threshold anybody
 * picked. A `usWidthClass` of 0 means the table says nothing, which is not the same as saying "medium".
 *
 * AND FOUR TRAITS ARE DELIBERATELY NOT SET, WHICH IS RECORDED RATHER THAN LEFT TO BE ASSUMED. THE CLASS MASK is a
 * mapping from `sFamilyClass` that no header in the era prints and no table states directly -- the header even says the
 * classes "correspond CLOSELY to the font class values in the OpenType 'OS/2' table", which is a correspondence and not
 * a table; VERTICAL needs the font's own direction, which no door here reports; and `UIOptimized` -- and the traits the
 * MODERN headers add for colour and composite glyphs, which this era does not have -- have no source in these tables at
 * all. A TRAIT THIS DOOR DOES NOT KNOW IS LEFT OUT RATHER THAN GUESSED, because a caller who checks one bit can survive
 * silence where a plausible wrong bit would be believed.
 *
 * AND THE SPELLING IS THE ERA'S: this header's constants are `kCTFontBoldTrait` and `kCTFontItalicTrait`, which the
 * modern headers RENAMED to `kCTFontTraitBold` and `kCTFontTraitItalic`. The old names are the ones in `CTFontTraits.h`
 * here, and they are used rather than the new ones because a library that declared the era's surface should answer in
 * it. */
CTFontSymbolicTraits CTFontGetSymbolicTraits(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CTFontSymbolicTraits traits = 0;
	CFDataRef os2, head, post;
	unsigned fs_selection = 0, width_class = 0, mac_style = 0, fixed_pitch = 0;

	if (f == NULL) {
		return 0;
	}
	os2 = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableOS2, kCTFontTableOptionNoOptions);
	if (os2 != NULL) {
		fs_selection = fn_table_u16(os2, 62);
		width_class = fn_table_u16(os2, 6);
		CFRelease(os2);
	}
	head = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableHead, kCTFontTableOptionNoOptions);
	if (head != NULL) {
		mac_style = fn_table_u16(head, 44);
		CFRelease(head);
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	if (post != NULL) {
		fixed_pitch = fn_table_u32(post, 12);
		CFRelease(post);
	}
	if ((fs_selection & fn_os2_fs_italic) != 0 || (mac_style & fn_head_mac_italic) != 0) {
		traits |= kCTFontItalicTrait;
	}
	if ((fs_selection & fn_os2_fs_bold) != 0 || (mac_style & fn_head_mac_bold) != 0) {
		traits |= kCTFontBoldTrait;
	}
	if ((mac_style & fn_head_mac_condensed) != 0 || (width_class >= 1 && width_class <= 4)) {
		traits |= kCTFontCondensedTrait;
	}
	if ((mac_style & fn_head_mac_expanded) != 0 || width_class >= 6) {
		traits |= kCTFontExpandedTrait;
	}
	if (fixed_pitch != 0) {
		traits |= kCTFontMonoSpaceTrait;
	}
	return traits;
}

/* THE FONT'S TRAITS DICTIONARY, WHOSE FOUR KEYS THIS LIBRARY'S OWN HEADER PRINTS AND WHOSE VALUES IT DOES NOT.
 *
 * THE OWED REASON WAS "the weight, width and slant entries are floats normalised around zero, and NO HEADER IN THE
 * ERA PRINTS THE CURVE that turns a `usWeightClass` into one". THE FIRST HALF WAS WRONG -- `CTFontTraits.h` prints
 * an ANCHOR for each of the three, in its own words -- and the second half is TRUE, so the curve is a RECORDED
 * DEVIATION rather than something worked around:
 *
 *   WEIGHT   "a float value between -1.0 and 1.0 ... The value of 0.0 corresponds to the regular or medium font
 *            weight." THE TABLE'S OWN SCALE IS 100 TO 900 IN STEPS OF 100 WITH 400 THE REGULAR ONE, so the distance
 *            from regular is divided by THE WIDEST DISTANCE THE FORMAT HAS ON THAT SIDE: 300 below it and 500 above
 *            it. Thin is -1.0, Regular 0.0, Bold +0.6, Black +1.0.
 *   WIDTH    "a float between -1.0 and 1.0 ... The value of 0.0 corresponds to regular glyph spacing while negative
 *            values represent condensed glyph spacing." `usWidthClass` runs 1 to 9 WITH 5 THE NORMAL ONE -- the same
 *            field `CTFontGetSymbolicTraits` reads for its two width bits -- so the distance from 5 is divided by 4.
 *   SLANT    "0.0 corresponds to 0 degree clockwise rotation from the vertical and 1.0 corresponds to 30 degrees
 *            clockwise rotation". So the degrees are divided by the thirty Apple states, AND THE SIGN IS TURNED OVER
 *            because `post.italicAngle` is measured COUNTER-clockwise: an ordinary oblique face carries a negative
 *            number there and a positive slant here.
 *
 * THESE THREE ARE THE MAPPINGS THAT HONOUR EVERY ANCHOR THE ERA'S OWN HEADER PRINTS, and Apple's real curves live in
 * its binaries. THE DAY ONE IS FOUND, THIS FUNCTION IS WHERE IT GOES, and the cases that assert the anchors are what
 * will hold it to them.
 *
 * AND ONE BOUNDARY SITS INSIDE THE WIDTH RATHER THAN BEING LEFT OUT: a `usWidthClass` of 0 says the table STATES
 * NOTHING, which the symbolic door honours by leaving its bit unset -- but Apple defines this entry as a NUMBER, and
 * a number cannot say "unknown". It is answered 0.0, the value that claims the least, and this paragraph is why. A
 * weight outside the format's own 100..900 is the same kind of statement and is treated the same way.
 *
 * AND THE SYMBOLIC ENTRY IS THIS LIBRARY'S OWN DOOR, because Apple says so: `CTFontGetSymbolicTraits` "is equivalent
 * to the kCTFontSymbolicTrait of traits dictionary". IT IS WRITTEN AS A 64-BIT NUMBER, which is a decision and not
 * a habit: the trait word is a BITFIELD whose top four bits are the stylistic class, so a signed 32-bit entry could
 * not carry `8 << 28` without changing it. */
CFDictionaryRef CTFontCopyTraits(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFMutableDictionaryRef traits;
	CFDataRef os2, post;
	CFNumberRef number;
	CTFontSymbolicTraits symbolic;
	int64_t symbolic64;
	double weight = 0.0, width = 0.0, slant = 0.0, italic_angle = 0.0;
	unsigned weight_class = 400, width_class = 5;

	if (f == NULL) {
		return NULL;
	}
	os2 = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableOS2, kCTFontTableOptionNoOptions);
	if (os2 != NULL) {
		weight_class = fn_table_u16(os2, 4);
		width_class = fn_table_u16(os2, 6);
		CFRelease(os2);
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	if (post != NULL) {
		italic_angle = fn_table_fixed(post, 4);	/* `post`'s italic angle, counter-clockwise degrees */
		CFRelease(post);
	}
	if (weight_class >= 100 && weight_class <= 900) {
		weight = (weight_class > 400) ? ((double)weight_class - 400.0) / 500.0
					      : -((400.0 - (double)weight_class) / 300.0);
	}
	if (width_class >= 1 && width_class <= 9) {
		width = ((double)width_class - 5.0) / 4.0;
	}
	slant = -italic_angle / 30.0;
	if (slant == 0.0) {
		slant = 0.0;	/* NOT A NEGATIVE ZERO. An upright face has no lean, and `-0.0` is a value that
				 * `signbit` reads as negative and a printer shows as "-0" -- so the entry says the zero
				 * every reader expects rather than one that is only equal to it. */
	}
	if (slant > 1.0) {
		slant = 1.0;	/* Apple's range is stated as closed, and a face CAN lean further than thirty degrees */
	} else if (slant < -1.0) {
		slant = -1.0;
	}

	traits = CFDictionaryCreateMutable(kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks,
					   &kCFTypeDictionaryValueCallBacks);
	if (traits == NULL) {
		return NULL;
	}
	symbolic = CTFontGetSymbolicTraits(font);
	symbolic64 = (int64_t)(uint32_t)symbolic;
	number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &symbolic64);
	if (number != NULL) {
		CFDictionarySetValue(traits, kCTFontSymbolicTrait, number);
		CFRelease(number);
	}
	number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &weight);
	if (number != NULL) {
		CFDictionarySetValue(traits, kCTFontWeightTrait, number);
		CFRelease(number);
	}
	number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &width);
	if (number != NULL) {
		CFDictionarySetValue(traits, kCTFontWidthTrait, number);
		CFRelease(number);
	}
	number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &slant);
	if (number != NULL) {
		CFDictionarySetValue(traits, kCTFontSlantTrait, number);
		CFRelease(number);
	}
	return traits;
}

CGFloat CTFontGetUnderlinePosition(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFDataRef post;
	int value;

	if (f == NULL) {
		return 0.0;
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	if (post == NULL) {
		return 0.0;	/* A FACE WITH NO `post` TABLE STATES NOTHING, and 0 is the only honest answer. */
	}
	value = fn_table_s16(post, 8);
	CFRelease(post);
	return (CGFloat)value * ct_font_scale(f);
}

CGFloat CTFontGetUnderlineThickness(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFDataRef post;
	int value;

	if (f == NULL) {
		return 0.0;
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	if (post == NULL) {
		return 0.0;
	}
	value = fn_table_s16(post, 10);
	CFRelease(post);
	return (CGFloat)value * ct_font_scale(f);
}

/* THE SLANT ANGLE, AND WHAT APPLE PRINTS IS THE COMPOSITION RATHER THAN THE NUMBER: "Returns the transformed slant
 * angle of the font. This is equivalent to the italic or caret angle WITH ANY SKEW FROM THE TRANSFORMATION MATRIX
 * APPLIED." TWO INGREDIENTS, AND THE HEADER NAMES BOTH:
 *
 *   THE FACE'S OWN ANGLE is `post.italicAngle`, which the format measures in degrees COUNTER-CLOCKWISE from the
 *   vertical -- so an ordinary oblique face carries a NEGATIVE number there, and this door turns it over.
 *   THE MATRIX'S SKEW is the angle the transform gives the VERTICAL: the vertical direction (0,1) lands on
 *   (c, d), so its angle clockwise from that vertical is atan2(c, d).
 *
 * AND THE TWO ARE ADDED, WHICH IS A DECISION RATHER THAN A DERIVATION: a matrix leaning the glyphs ten degrees to
 * the right puts a face with an eleven-degree italic at twenty-one, and the case asserts THE SUM rather than either
 * half, because a door that reported only one of them would pass a case that checked only that one.
 *
 * AND THE UNIT IS DEGREES, WHICH APPLE DOES NOT STATE ANYWHERE IN THE ERA'S HEADERS -- SO IT IS A CHOICE, RECORDED
 * HERE. The reasons for it: every other angle in this surface is in degrees (`kCTFontSlantTrait`'s "1.0 corresponds
 * to 30 degrees", and `post.italicAngle` itself, which is where the first ingredient is read from), and the
 * alternative would make this door and that trait key disagree by a factor of fifty-seven while both claiming to be
 * the font's slant.
 *
 * AND THE TRANSFORM'S CONVENTION IS USED RATHER THAN ASSUMED: `CGAffineTransform` is Apple's own six fields and
 * (x,y) maps to (a*x + c*y + tx, b*x + d*y + ty), which is what makes (c, d) the image of the vertical direction.
 * The case measures that with `CGPointApplyAffineTransform` instead of trusting this sentence. */
CGFloat CTFontGetSlantAngle(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFDataRef post;
	double degrees = 0.0;

	if (f == NULL) {
		return 0.0;
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	if (post != NULL) {
		degrees = -fn_table_fixed(post, 4);
		CFRelease(post);
	}
	/* THE SKEW THE MATRIX APPLIES, IN THE SAME UNIT AND THE SAME DIRECTION. The constant is written out rather
	 * than taken from `math.h`: M_PI is not in the C standard, and a library whose includes are its dependencies
	 * should not reach for one. */
	degrees += atan2((double)f->matrix.c, (double)f->matrix.d) * (180.0 / 3.14159265358979323846);
	return (CGFloat)degrees;
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

/* **THE PAIR ADJUSTMENT, IN POINTS — THE NUMBER A LINE CANNOT GET FROM THE ADVANCES.** A glyph's advance says how
 * far the pen moves for that glyph and nothing about which glyph comes next; the font's `kern` table is a statement
 * about the PAIR, and it is the whole difference between "T" followed by "o" and `To`. The conversion is the same
 * one every metric here makes: the face's number in font units, times the size over the em. */
double CTFontGetKerningBetweenGlyphs(CTFontRef font, CGGlyph left, CGGlyph right)
{
	const struct __CTFont *f = (const struct __CTFont *)font;

	if (f == NULL) {
		return 0.0;
	}
	return (double)cg_font_kern(f->graphics, left, right) * (double)ct_font_scale(f);
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

/* THE OFFSET FROM THE HORIZONTAL ORIGIN TO THE VERTICAL ONE, WHICH IS THE WHOLE OF WHAT APPLE SAYS ABOUT IT:
 * "Calculates the offset from the default (horizontal) origin to the vertical origin for an array of glyphs."
 *
 * AND THE OFFSET IS THE COMPOSITION OF THREE DOORS THIS LIBRARY ALREADY HAS. A horizontal origin sits on the
 * BASELINE at the glyph's LEFT SIDE BEARING; a vertical origin sits at the TOP of the line, centred over the
 * glyph's advance. So the offset for a glyph is
 *
 *     x = advance / 2 - left side bearing          y = the face's ascent
 *
 * AND EVERY INGREDIENT IS READ RATHER THAN ASSUMED: the advance from `CTFontGetAdvancesForGlyphs`, the bearing from
 * the ORIGIN OF THE BOUNDING BOX `CTFontGetBoundingRectsForGlyphs` reports -- for the horizontal orientation that
 * origin IS the left side bearing -- and the ascent from `CTFontGetAscent`. Nothing here is a table read, because
 * nothing here needs one.
 *
 * AND THE VERTICAL METRICS ARE NOT READ, WHICH IS RECORDED AS A BOUNDARY RATHER THAN HIDDEN BY THE DOOR'S PRESENCE --
 * AND THE SENTENCE THAT FIRST STOOD HERE WAS FALSE, SO IT IS CORRECTED RATHER THAN TIDIED. It said "NO FACE INSTALLED
 * HERE CARRIES THEM, which was measured by asking the installed faces for their table directories rather than
 * assumed". THE MEASUREMENT HAD NEVER BEEN TAKEN: the scan that preceded this unit asked every installed face for
 * `fvar`, `avar`, `feat`, `morx` and `mort`, AND NOT FOR `vhea`/`vmtx` AT ALL -- so a claim about the ENVIRONMENT was
 * written from the SHAPE of the reason in front of it, which is the exact fault this tree's PROVENANCE documents at
 * length. Asked properly, TWO faces here carry BOTH tables, and they are named because a reader can go and check:
 * `DroidSansFallbackFull.ttf` (98,766 bytes of `vmtx`, `vhea` `numOfLongVerMetrics` 1) and `NotoColorEmoji.ttf`.
 *
 * SO THE CONDITION THAT WAS WRITTEN TO MOVE THIS DOOR IS TRUE ALREADY, and what is left is the WORK rather than the
 * environment: `vhea`'s ascent and line gap, `vmtx`'s per-glyph origin and advance (and `VORG` for a CFF face) are
 * what a vertical answer would read through the seam's own table door, and the unit that takes them is named here.
 * UNTIL IT IS TAKEN, A CALLER DRAWING ONE OF THOSE TWO FACES VERTICALLY GETS THE FALLBACK BELOW -- which is the
 * difference between a recorded gap and a hidden one.
 *
 * WHAT THE DOOR ANSWERS, THEREFORE, IS THE FORMAT'S OWN FALLBACK FOR A FACE WITHOUT VERTICAL METRICS: vertical
 * layout on the HORIZONTAL advances with the ascent above them, which is exactly where the two formulas above come
 * from. It is exact for every face here that has no `vmtx`, which is every face this suite draws with and all but two
 * of the faces installed.
 *
 * AND A SECOND BOUNDARY GOES WITH IT: what this door answers is SCALED BY THE POINT SIZE AND NOT BY THE MATRIX,
 * which is this library's rule for every metric -- the matrix moves GLYPHS, and these are offsets in the boxes they
 * sit in. An offset here is a translation in the font's own points, through whichever doors supply it. */
void CTFontGetVerticalTranslationsForGlyphs(CTFontRef font, const CGGlyph glyphs[], CGSize translations[],
					    CFIndex count)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CGSize *advances;
	CGFloat ascent;
	CFIndex i;

	if (f == NULL || glyphs == NULL || translations == NULL || count <= 0) {
		return;
	}
	if (count > (CFIndex)(SIZE_MAX / sizeof(CGSize))) {
		return;
	}
	advances = (CGSize *)malloc((size_t)count * sizeof(CGSize));
	if (advances == NULL) {
		return;	/* NOTHING IS WRITTEN, which is the honest failure: a half-filled array would be worse */
	}
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, glyphs, advances, count);
	ascent = CTFontGetAscent(font);
	for (i = 0; i < count; i++) {
		CGRect box;

		(void)CTFontGetBoundingRectsForGlyphs(font, kCTFontHorizontalOrientation, &glyphs[i], &box, 1);
		translations[i].width = advances[i].width / 2.0 - box.origin.x;
		translations[i].height = ascent;
	}
	free(advances);
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

/* THE RAW TABLE DOORS -- THE PAIR OF THE OWED FAMILY THAT NEEDED NO PARSING AT ALL, BECAUSE APPLE'S ANSWER FOR A TABLE
 * IS THE FILE'S BYTES. They were owed with the reason that NO SEAM DOOR READS A TABLE, and that reason is now FALSE
 * rather than lifted: `libagora-drawing` grew `cg_font_copy_table` and `cg_font_table_tags` in its own commit, and what
 * those two answer is exactly what these two owe. The owed block below is corrected in the same commit, which is this
 * tree's rule -- an owed reason is a CONDITION, and a condition that becomes true moves a door. */
CFDataRef CTFontCopyTable(CTFontRef font, CTFontTableTag table, CTFontTableOptions options)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	unsigned char *bytes;
	CFDataRef data;
	int size;

	/* THE OPTION IS OBEYED TRIVIALLY, WHICH IS THE HONEST FORM OF "NOT OBEYED": `kCTFontTableOptionExcludeSynthetic`
	 * asks for tables that are not synthesised, and NOTHING THIS DOOR RETURNS IS SYNTHESISED -- every table comes out of
	 * the file the face was opened from. There is nothing for the flag to exclude, so there is nothing to record as
	 * missing either. */
	(void)options;
	if (f == NULL) {
		return NULL;
	}
	/* THE SEAM'S TWO-CALL CONTRACT, USED AS THE SEAM DOCUMENTS IT: the size first, then the bytes. */
	size = cg_font_copy_table(f->graphics, (uint32_t)table, NULL, 0);
	if (size <= 0) {
		return NULL;
	}
	bytes = (unsigned char *)malloc((size_t)size);
	if (bytes == NULL) {
		return NULL;
	}
	if (cg_font_copy_table(f->graphics, (uint32_t)table, bytes, (size_t)size) <= 0) {
		free(bytes);
		return NULL;
	}
	data = CFDataCreate(kCFAllocatorDefault, bytes, (CFIndex)size);
	free(bytes);
	return data;
}

/* AND THE DIRECTORY, AS AN ARRAY OF NUMBERS EACH HOLDING A TAG -- which is what Apple's door answers, and the reason it
 * is numbers rather than strings is that a table tag IS four bytes and not a name anybody chose. */
CFArrayRef CTFontCopyAvailableTables(CTFontRef font, CTFontTableOptions options)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFMutableArrayRef array;
	uint32_t *tags;
	int count, i;

	(void)options;	/* as above: nothing this face carries is synthesised. */
	if (f == NULL) {
		return NULL;
	}
	array = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (array == NULL) {
		return NULL;
	}
	count = cg_font_table_tags(f->graphics, NULL, 0);
	/* A FACE WITH NO TABLE DIRECTORY ANSWERS WITH AN EMPTY ARRAY RATHER THAN NULL: "this face has no tables" is an
	 * ANSWER, and the door that could not be asked at all is the one that answers NULL. */
	if (count > 0) {
		tags = (uint32_t *)malloc((size_t)count * sizeof(uint32_t));
		if (tags == NULL || cg_font_table_tags(f->graphics, tags, (size_t)count) <= 0) {
			free(tags);
			CFRelease(array);
			return NULL;
		}
		for (i = 0; i < count; i++) {
			CFNumberRef number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &tags[i]);

			if (number != NULL) {
				CFArrayAppendValue(array, number);
				CFRelease(number);
			}
		}
		free(tags);
	}
	return array;
}

/* =================================================================================================
 * THE FEATURE AND VARIATION VOCABULARY. Three doors, and each one's reason had to be corrected before it
 * could be built -- which is this tree's rule rather than a coincidence: an owed reason is a CONDITION, and
 * a condition that turns out to be false does not move a door until it is said out loud.
 * =============================================================================================== */

/* AN ARRAY OF VARIATION AXES, WHICH IS THE `fvar` TABLE -- AND THE OWED REASON NAMED TWO TABLES WHERE ONE IS
 * NEEDED, WHICH IS THE SECOND TIME THIS FILE CORRECTS A REASON RATHER THAN QUIETLY IMPLEMENTING THE DOOR
 * (`CTFontCopyVariation`'s named the same pair). Apple's five axis keys are the axis record and nothing else:
 * `avar` MAPS A NORMALISED USER COORDINATE ONTO A NORMALISED DESIGN COORDINATE, and that mapping is applied when a
 * coordinate is SET. It is not one of the five facts an axis dictionary carries, so it is not owed BY THIS DOOR --
 * it is owed by the seam door that would apply a coordinate, which is the same door `CTFontCopyVariation` names as
 * the boundary on its own answer. THE DOOR IS THEREFORE `fvar` ALONE, and no case here needed a second table.
 *
 * AND THE LAYOUT IS READ FROM THE TABLE RATHER THAN FROM A VERSION: the header carries `offsetToData` and
 * `axisSize`, and both are used instead of the 16 and the 20 that a 1.0 file happens to have. THE VERSION IS
 * CHECKED ANYWAY, because 1.0 and 1.1 are the two the era's format defines and a face carrying something else must
 * be answered rather than read at offsets a guess would place -- and what it is answered with is NULL, since "a
 * version this parser does not know" is not the same statement as "no axes".
 *
 * A FACE WITH NO `fvar` ANSWERS AN EMPTY ARRAY, WHICH IS A CHOICE AND IS RECORDED AS ONE: "this face has no
 * variation axes" IS an answer about the face, and NULL is kept for the door that could not be asked at all --
 * exactly the rule `CTFontCopyAvailableTables` applies to a face with no table directory.
 *
 * AND THE NAME IS THE FACE'S OWN, READ THROUGH THE SEAM'S NAME DOOR: an axis record names itself by `name` table
 * index and not by a string, so `kCTFontVariationAxisNameKey` is answered from there and is OMITTED rather than
 * invented when the face has no such entry. Apple calls it the localized name; a localized name needs a language,
 * which this tier has no list of, and the face's own entry is the only source there is. */
CFArrayRef CTFontCopyVariationAxes(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFMutableArrayRef axes;
	CFDataRef fvar;
	unsigned version, offset_to_data, axis_count, axis_size, i;

	if (f == NULL) {
		return NULL;
	}
	axes = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (axes == NULL) {
		return NULL;
	}
	fvar = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableFvar, kCTFontTableOptionNoOptions);
	if (fvar == NULL) {
		return axes;
	}
	version = fn_table_u32(fvar, 0);
	offset_to_data = fn_table_u16(fvar, 4);
	axis_count = fn_table_u16(fvar, 8);
	axis_size = fn_table_u16(fvar, 10);
	if ((version != 0x00010000u && version != 0x00010001u) || axis_size < 20 ||
	    (CFIndex)(offset_to_data + (unsigned)axis_count * axis_size) > CFDataGetLength(fvar)) {
		CFRelease(fvar);	/* a version, a stride or a count this parse cannot hold: answered, not invented */
		CFRelease(axes);
		return NULL;
	}
	for (i = 0; i < axis_count; i++) {
		CFIndex at = (CFIndex)offset_to_data + (CFIndex)i * (CFIndex)axis_size;
		CFMutableDictionaryRef axis;
		CFNumberRef number;
		CFStringRef name;
		int32_t tag = (int32_t)fn_table_u32(fvar, at);
		double minimum = fn_table_fixed(fvar, at + 4);
		double deflt = fn_table_fixed(fvar, at + 8);
		double maximum = fn_table_fixed(fvar, at + 12);
		unsigned name_id = fn_table_u16(fvar, at + 18);
		char buffer[512];
		int written;

		axis = CFDictionaryCreateMutable(kCFAllocatorDefault, 5, &kCFTypeDictionaryKeyCallBacks,
						 &kCFTypeDictionaryValueCallBacks);
		if (axis == NULL) {
			continue;
		}
		/* THE IDENTIFIER IS THE FOUR-CHARACTER TAG AS A NUMBER, which is Apple's own description of it. */
		number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &tag);
		if (number != NULL) {
			CFDictionarySetValue(axis, kCTFontVariationAxisIdentifierKey, number);
			CFRelease(number);
		}
		number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &minimum);
		if (number != NULL) {
			CFDictionarySetValue(axis, kCTFontVariationAxisMinimumValueKey, number);
			CFRelease(number);
		}
		number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &maximum);
		if (number != NULL) {
			CFDictionarySetValue(axis, kCTFontVariationAxisMaximumValueKey, number);
			CFRelease(number);
		}
		number = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &deflt);
		if (number != NULL) {
			CFDictionarySetValue(axis, kCTFontVariationAxisDefaultValueKey, number);
			CFRelease(number);
		}
		written = cg_font_name_entry(f->graphics, (int)name_id, buffer, sizeof buffer);
		if (written > 0) {
			name = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)buffer, (CFIndex)written,
						       kCFStringEncodingUTF8, false);
			if (name != NULL) {
				CFDictionarySetValue(axis, kCTFontVariationAxisNameKey, name);
				CFRelease(name);
			}
		}
		CFArrayAppendValue(axes, axis);
		CFRelease(axis);
	}
	CFRelease(fvar);
	return axes;
}

/* THE FEATURE SETTINGS THE FONT WAS ASKED FOR, WHICH IS THE SAME SHAPE OF ANSWER `CTFontCopyVariation` GIVES: a
 * settings array is a CHOICE, and it has been held in the font's own descriptor since the font was created. Apple's
 * sentence names both halves -- "only the non-default settings that should be applied to the font, or NULL if the
 * default settings should be used" -- AND THE SECOND HALF IS THE ONE THIS DOOR HOLDS EXACTLY: a font asked for no
 * setting answers NULL rather than an empty array, because "the default settings should be used" is precisely what
 * NULL means.
 *
 * AND THE HALF THAT IS A BOUNDARY IS RECORDED RATHER THAN CLAIMED: the FILTER that would drop a setting equal to
 * the face's own default needs `feat`'s per-selector default flags, which are `CTFontCopyFeatures`' business and
 * not this door's -- and no face installed here carries a `feat` table at all. What is answered is the settings the
 * font REPRESENTS, each naming a type and a selector and carrying the setting the caller wrote down; a setting that
 * states the default value is a caller's mistake rather than a door's invention. */
CFArrayRef CTFontCopyFeatureSettings(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFTypeRef settings;

	if (f == NULL || f->descriptor == NULL) {
		return NULL;
	}
	settings = CTFontDescriptorCopyAttribute(f->descriptor, kCTFontFeatureSettingsAttribute);
	if (settings == NULL) {
		return NULL;
	}
	if (CFGetTypeID(settings) != CFArrayGetTypeID() || CFArrayGetCount((CFArrayRef)settings) == 0) {
		CFRelease(settings);	/* a value that is not settings, or an empty one, is the default answer */
		return NULL;
	}
	return (CFArrayRef)settings;	/* +1, from the descriptor's own copy door. */
}

/* THE SETTINGS MERGE, WHICH IS THE DESCRIPTOR'S COPY DOOR'S RULE AND IS DEFINED HERE BECAUSE OF THE INCLUDE GRAPH
 * (`CTFontDescriptor_internal.h` carries the seam and says why). It is the fix this commit's door exposed rather
 * than a tidy-up: Apple names `kCTFontFeatureSettingsAttribute` among the keys a copy MERGES, its value is an ARRAY
 * of setting dictionaries, and the descriptor's copy door merged only DICTIONARIES and replaced everything else --
 * so a caller who toggled one feature dropped every other setting it had asked for.
 *
 * AND THE MERGE IS BY (TYPE, SELECTOR), which is what makes Apple's own description of the door -- "toggle the
 * state of individual features" -- do what it says: a setting for a pair already present REPLACES it, and a setting
 * for a new pair is APPENDED. A dictionary that names neither is APPENDED rather than dropped, because dropping a
 * caller's value is the failure this function exists to avoid.
 *
 * IT ANSWERS A NEW ARRAY THE CALLER OWNS, never the array it was given: the caller is a copy door that will store
 * the result in a dictionary and then release it, and a function that sometimes answered its argument would make
 * two files disagree about who releases what. */
CFArrayRef CTFontDescriptorMergeFeatureSettings(CFArrayRef original, CFArrayRef additions)
{
	CFMutableArrayRef merged;
	CFIndex i, j, count;

	if (original == NULL || additions == NULL) {
		return NULL;
	}
	merged = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, original);
	if (merged == NULL) {
		return NULL;
	}
	count = CFArrayGetCount(additions);
	for (i = 0; i < count; i++) {
		CFDictionaryRef addition = (CFDictionaryRef)CFArrayGetValueAtIndex(additions, i);
		CFNumberRef type, selector;
		CFIndex found = -1;

		if (CFGetTypeID(addition) != CFDictionaryGetTypeID()) {
			continue;
		}
		type = (CFNumberRef)CFDictionaryGetValue(addition, kCTFontFeatureTypeIdentifierKey);
		selector = (CFNumberRef)CFDictionaryGetValue(addition, kCTFontFeatureSelectorIdentifierKey);
		if (type != NULL && selector != NULL) {
			for (j = 0; j < CFArrayGetCount(merged); j++) {
				CFDictionaryRef candidate = (CFDictionaryRef)CFArrayGetValueAtIndex(merged, j);
				CFNumberRef have_type, have_selector;

				if (CFGetTypeID(candidate) != CFDictionaryGetTypeID()) {
					continue;
				}
				have_type = (CFNumberRef)CFDictionaryGetValue(candidate,
									      kCTFontFeatureTypeIdentifierKey);
				have_selector = (CFNumberRef)CFDictionaryGetValue(candidate,
										  kCTFontFeatureSelectorIdentifierKey);
				if (have_type != NULL && have_selector != NULL && CFEqual(have_type, type) &&
				    CFEqual(have_selector, selector)) {
					found = j;
					break;
				}
			}
		}
		if (found >= 0) {
			CFArraySetValueAtIndex(merged, found, addition);
		} else {
			CFArrayAppendValue(merged, addition);
		}
	}
	return merged;
}

/* THE FEATURE COPY IS DEFINED HERE RATHER THAN IN `CTFontDescriptor.c`, AND THE REASON IS THE INCLUDE GRAPH: the
 * door is declared in that header and is a spelling of a feature-SETTINGS dictionary, whose two keys belong to
 * `CTFont.h`, which INCLUDES `CTFontDescriptor.h` and so cannot be included back. The precedent is the matching
 * tier's -- those doors are defined in `CTFontMatching.c` because that is where the tier that answers them lives --
 * and the same reasoning applies: the definition sits where its keys are visible. `CTFontDescriptor.c`'s owed
 * comment said this door "lands with CTFont.h"; CTFont.h has landed, so the door has.
 *
 * AND WHAT IT SPELLS IS A TUPLE, which is Apple's own word for a feature setting: a dictionary carrying the type,
 * the selector, and the SETTING -- which this door states as ON, because a door whose purpose is "to more easily
 * toggle the state of individual features" and which is handed a selector to toggle is being asked for that
 * selector enabled. The alternative, a tuple with no setting at all, would be a tuple that says nothing about the
 * state it exists to carry. It is handed to the attributes copy door, so the MERGE rule -- where this same commit
 * found a real bug -- is stated once rather than twice. */
CTFontDescriptorRef CTFontDescriptorCreateCopyWithFeature(CTFontDescriptorRef original,
							  CFNumberRef featureTypeIdentifier,
							  CFNumberRef featureSelectorIdentifier)
{
	CFMutableDictionaryRef setting, wrapper;
	CFArrayRef settings;
	CFDictionaryRef additions;
	CTFontDescriptorRef copy;

	if (original == NULL || featureTypeIdentifier == NULL || featureSelectorIdentifier == NULL) {
		return NULL;
	}
	setting = CFDictionaryCreateMutable(kCFAllocatorDefault, 3, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	if (setting == NULL) {
		return NULL;
	}
	CFDictionarySetValue(setting, kCTFontFeatureTypeIdentifierKey, featureTypeIdentifier);
	CFDictionarySetValue(setting, kCTFontFeatureSelectorIdentifierKey, featureSelectorIdentifier);
	CFDictionarySetValue(setting, kCTFontFeatureSelectorSettingKey, kCFBooleanTrue);
	settings = CFArrayCreate(kCFAllocatorDefault, (const void **)&setting, 1, &kCFTypeArrayCallBacks);
	CFRelease(setting);
	if (settings == NULL) {
		return NULL;
	}
	wrapper = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	if (wrapper == NULL) {
		CFRelease(settings);
		return NULL;
	}
	CFDictionarySetValue(wrapper, kCTFontFeatureSettingsAttribute, settings);
	CFRelease(settings);
	additions = wrapper;
	copy = CTFontDescriptorCreateCopyWithAttributes(original, additions);
	CFRelease(additions);
	return copy;
}

/* THE CURRENT VARIATION INSTANCE, WHICH NEEDS NO TABLE AT ALL -- AND THE OWED REASON NAMED TWO OF THEM, WHICH IS WHY
 * THIS COMMIT CORRECTS IT RATHER THAN QUIETLY IMPLEMENTING THE DOOR.
 *
 * APPLE'S OWN WORDS SETTLE WHICH DOOR NEEDS WHAT: `CTFontCopyVariation` "returns the CURRENT VARIATION INSTANCE as a
 * dictionary", while `CTFontCopyVariationAxes` returns "an ARRAY of variation AXES". AN INSTANCE IS A CHOICE AND AN AXIS
 * IS A CAPABILITY: the axes come out of the `fvar` table, which this library still owes, and the INSTANCE is the
 * variation dictionary the font was ASKED FOR, which this tier has been storing since it began holding the descriptor it
 * was made from. The keys are CFNumbers holding four-character axis codes, which is what Apple documents and what a
 * caller passes in.
 *
 * AND ONE BOUNDARY IS RECORDED HERE RATHER THAN IMPLIED BY THE DOOR'S PRESENCE: THIS TIER CAN REPORT THE INSTANCE IT WAS
 * ASKED FOR AND CANNOT APPLY IT. Nothing in the seam sets a variation coordinate on the face, so the glyphs a caller
 * draws from such a font are the DEFAULT instance's -- this door answers what the font REPRESENTS, which is honest, and
 * that is not the same as what the font RENDERS, which is a capability the engine here does not have. A caller who needs
 * the applied instance needs a seam door that sets coordinates, and there is none.
 *
 * AND A FONT WITH NO INSTANCE ANSWERS NULL RATHER THAN AN EMPTY DICTIONARY, WHICH IS A CHOICE AND IS RECORDED AS ONE:
 * Apple's header describes the answer for a font that HAS an instance and says nothing about one that has not, and
 * "there is none" is said more plainly by NULL than by a dictionary with no keys in it. */
CFDictionaryRef CTFontCopyVariation(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFTypeRef variation;

	if (f == NULL || f->descriptor == NULL) {
		return NULL;
	}
	variation = CTFontDescriptorCopyAttribute(f->descriptor, kCTFontVariationAttribute);
	if (variation == NULL) {
		return NULL;
	}
	if (CFGetTypeID(variation) != CFDictionaryGetTypeID()) {
		CFRelease(variation);	/* AN INSTANCE THAT IS NOT A DICTIONARY is not one this door can answer with. */
		return NULL;
	}
	return (CFDictionaryRef)variation;	/* +1, from the descriptor's own copy door. */
}

/* THE FACE'S CHARACTER SET: EVERY CODE UNIT THE FACE MAPS TO A GLYPH, AND IT IS ANSWERED BY THE SAME STEP
 * `CTFontGetGlyphsForCharacters` USES, ASKED EXHAUSTIVELY RATHER THAN ONE CHARACTER AT A TIME. The seam maps a BATCH
 * of code units in one call, so the whole basic multilingual plane is SIXTEEN calls and no table access is needed --
 * which is why this door did not have to wait for the seam to grow a table door, where the rest of its family does.
 *
 * A CODE UNIT IS IN THE SET WHEN THE FACE MAPS IT TO A **NON-ZERO** GLYPH, and glyph 0 is `.notdef` -- "this face has
 * nothing here" -- which is the same rule the mapping door reports as failure. Surrogates therefore fall out of the
 * answer by themselves rather than needing a special case, and a face that maps U+0000 keeps it.
 *
 * AND THE SCAN IS BOUNDED AT U+FFFF, WHICH IS RECORDED RATHER THAN SILENT: the seam takes UTF-16 code units, so the
 * planes above the basic multilingual plane are not reachable THROUGH THIS DOOR -- a surrogate pair is two code units
 * and the second of them alone maps to nothing. Apple reads a cmap and would include them. See PROVENANCE.md for the
 * condition that would close it. */
CFCharacterSetRef CTFontCopyCharacterSet(CTFontRef font)
{
	const struct __CTFont *f = (const struct __CTFont *)font;
	CFMutableCharacterSetRef set;
	uint16_t *characters;
	CGGlyph *glyphs;
	const size_t chunk = 4096;
	uint32_t start;

	if (f == NULL) {
		return NULL;
	}
	set = CFCharacterSetCreateMutable(kCFAllocatorDefault);
	characters = (uint16_t *)malloc(chunk * sizeof(uint16_t));
	glyphs = (CGGlyph *)malloc(chunk * sizeof(CGGlyph));
	if (set == NULL || characters == NULL || glyphs == NULL) {
		if (set != NULL) {
			CFRelease(set);
		}
		free(characters);
		free(glyphs);
		return NULL;
	}
	for (start = 0; start <= 0xFFFF; start += (uint32_t)chunk) {
		size_t i, count = ((0x10000u - start) < chunk) ? (size_t)(0x10000u - start) : chunk;

		for (i = 0; i < count; i++) {
			characters[i] = (uint16_t)(start + i);
		}
		if (cg_font_glyphs_for_unichars(f->graphics, characters, glyphs, count) > 0) {
			for (i = 0; i < count; i++) {
				if (glyphs[i] != 0) {
					CFCharacterSetAddCharactersInRange(set, CFRangeMake((CFIndex)(start + i), 1));
				}
			}
		}
	}
	free(characters);
	free(glyphs);
	return (CFCharacterSetRef)set;
}

/* =================================================================================================
 * WHAT IS STILL OWED, AND WHY. Every door below is DECLARED in a header and NOT DEFINED, so a caller who reaches for
 * one gets a LINK ERROR rather than a plausible wrong answer -- and `tests/CMakeLists.txt` names every one of them,
 * because a comment asserting an absence is worth little: the `owed` test reads `nm -D` and FAILS if a door on that
 * list acquires a definition.
 *
 * TEN DOORS HAVE LEFT THIS LIST IN ONE NIGHT, AND THE LIST WAS WRONG ABOUT NINE OF THEM IN A WAY THAT IS WORTH
 * READING AS ONE THING: THEIR REASONS WERE CONDITIONS THAT HAD GONE FALSE WITHOUT THE LIST BEING READ AGAIN.
 *
 *   CTFontCopyVariation           its reason named `fvar` and `avar`; Apple's own words say an INSTANCE is a CHOICE
 *                                and an AXIS is a CAPABILITY, and the choice was already in the descriptor.
 *   CTFontCopyVariationAxes       `avar` is not one of the five facts an axis dictionary carries, and the note that
 *                                "no variable font is installed on this machine" was written WITHOUT LOOKING: one is.
 *   CTFontCopyFeatureSettings     it was owed as half of "the `feat` table", and it is not that door at all -- it
 *                                answers the settings the font was ASKED FOR.
 *   CTFontDescriptorCreateCopyWithFeature   landed with `CTFont.h`, where its two keys live -- and found a REAL BUG
 *                                in the descriptor's copy door on the way (see CTFontDescriptor.c).
 *   CTFontCopyCharacterSet        the glyph step could be asked exhaustively instead of a cmap being read.
 *   CTFontCopyTable, ...Tables    `libagora-drawing` grew the seam doors that read a table (another repository's work).
 *   The `post` metrics,           the trait tables, and the three doors that ASK A REGISTRY rather than a name: the
 *   GetSymbolicTraits,            matching tier's walk enumerates the faces, so "a face with those traits to exist
 *   CopyTraits, SlantAngle,       and be found" stopped being a shortage. The trait and slant and vertical doors are
 *   VerticalTranslations          the last of them, and each one's curve or formula is a RECORDED DEVIATION rather
 *                                than a guess: see the doors themselves.
 *
 * AND WHAT REMAINS IS SIX, AND NOT ONE OF THEM IS WAITING ON WORK THIS LIBRARY CAN DO TONIGHT:
 *
 *   CTFontCopyFeatures                            the `feat` table -- and its reason is NOT "the seam cannot read a
 *                                                table" (it can) but that NO FACE INSTALLED HERE CARRIES ONE, which
 *                                                was measured by asking every installed face for its table directory.
 *                                                A parser nothing can hold is not shipped, and the condition that
 *                                                moves this door is a face that carries `feat`.
 *   CTFontCopySupportedLanguages,                 A MAPPING NO ERA HEADER PRINTS: which BCP 47 languages a face
 *   CTFontGetStringEncoding                       supports, and which legacy encoding is "best" for it, are Apple's
 *                                                own tables rather than a field's value. `OS/2`'s code-page ranges are
 *                                                the nearest data and they answer for WINDOWS code pages, not for
 *                                                languages; the user's decision is that these two STAY OWED rather than
 *                                                be answered by a mapping this library would have to invent.
 *   CTFontGetPlatformFont, ...WithPlatformFont    PERMANENT, AND THE DECISION IS RECORDED: these speak in ATSFontRef
 *                                                and Agora HAS NO ATS. The alternative -- an Agora id space standing
 *                                                in for ATS's -- would be this library answering in a vocabulary the
 *                                                era's framework does not have, which is the one thing it will not do.
 *   CTFontManagerCreateFontRequestRunLoopSource   the Agora font service: THE WIRE IS BUILT (`CTFontService.c` holds
 *                                                both halves of the conversation and its case drives them in
 *                                                process) and THE TRANSPORT IS NOT -- a socket, a port name derived
 *                                                from the process, a name registry with a runtime behind it, and the
 *                                                daemon that can find another process's port. `FONT-SERVICE.md` has
 *                                                the survey and names which half is which. It is a SYSTEM and not a
 *                                                door, which is why it is its own milestone.
 *
 * AND ONE DOOR OF APPLE'S IS NOT EVEN DECLARED HERE, WHICH IS THE ONLY ABSENCE OF THAT KIND: `CTFontCreateWithQuickdraw-
 * Instance`. Apple's 10.6 header declares it plainly, with no guard, and this library does not declare it at all -- so
 * a caller does not get a link error, they get nothing to compile against, WHICH IS A LOUDER ABSENCE AND A WORSE ONE TO
 * LEAVE UNSAID. THE REASON IS A DRAWING SYSTEM AND NOT A TIER: QuickDraw is the Macintosh-era drawing system, Agora is
 * a desktop environment that does not have it, and the door speaks in its terms.
 *
 * RECORDED HERE RATHER THAN LEFT OUT, because of what this tree's rules actually say: owed doors are DECLARED, with a
 * reason each, so that reaching for one gets a link error rather than a guess. A door that is not declared has left the
 * mechanism entirely, and an omission with no note is the one kind of absence that cannot be checked by anybody later.
 * AND THE REASON ABOVE EACH DOOR IS A CONDITION, WHICH IS WHY THIS PAGE KEEPS BEING REWRITTEN: the rule that has held
 * all night is that a reason which becomes false moves a door, and the only way to notice is to READ THIS LIST AGAINST
 * THE LIBRARY rather than trusting it.
 * =============================================================================================== */
