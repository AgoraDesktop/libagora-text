/*
 * CTFontDescriptor.h — a font, described rather than loaded: the attributes dictionary, its keys, and the
 * three vocabularies that classify a face.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). This is the second leaf of Apple's include graph: it
 * includes CTFontTraits.h, five Core Foundation headers and CGAffineTransform.h, and no other Core Text
 * header, so it can be declared completely before anything above it exists.
 *
 * AND IT IS WHERE THE FIRST OBJECT ARRIVES. `CTFontDescriptorRef` is an opaque Core Foundation object, which
 * is what Apple's is — `typedef const struct __CTFontDescriptor *` — and the type is registered through
 * <CoreFoundation/CFRuntime.h> rather than being a class, because Apple's Core Text has no classes at all.
 *
 * THE KEYS' STRINGS ARE A RECORDED GUESS, WHICH IS THE MOST IMPORTANT SENTENCE IN THIS FILE. Apple publishes
 * NO VALUE FOR ANY OF THEM — not in its headers, which declare them all `extern const CFStringRef`, not in
 * its documentation, and not in its documentation index, which answers only `let kCTFontNameAttribute:
 * CFString`. They live in Apple's binaries. Of these twenty-two keys, EIGHT pair by name with a constant the
 * AppKit declares — kCTFontNameAttribute with NSFontNameAttribute, and likewise CharacterSet, Traits,
 * Size, Matrix, Variation, CascadeList and FeatureSettings — and for those eight the value is provably
 * "NSFont<Suffix>Attribute", because a traits or attributes dictionary CROSSES between the two frameworks
 * and the string a dictionary is written with has to be the string it is read with.
 *
 * THE OTHER FOURTEEN HAVE NO COUNTERPART TO CHECK AGAINST, and the pairing is not even mechanical: Apple's
 * AppKit calls its family attribute NSFontFamilyAttribute while Core Text calls the key
 * kCTFontFamilyNameAttribute. USER'S DECISION (`dec-4795696b09d362f6`): ALL TWENTY-TWO TAKE THE
 * "NSFont<Suffix>Attribute" FORM, as a guess recorded where it was made rather than as a fact. The eight are
 * right; the fourteen are the best available reading of Apple's own naming, and they are the first thing to
 * check the day a real macOS or a working AppKit can answer for them.
 */
#ifndef CORE_TEXT_CTFONTDESCRIPTOR_H
#define CORE_TEXT_CTFONTDESCRIPTOR_H

#include <CoreText/CTFontTraits.h>

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFSet.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>
#include <CoreGraphics/CGAffineTransform.h>
#include <CoreGraphics/CGBase.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE OBJECT, and it is opaque: a descriptor's contents are an attributes dictionary, and a caller reaches
 * it through the accessors below rather than through the layout. */
typedef const struct __CTFontDescriptor *CTFontDescriptorRef;

CFTypeID CTFontDescriptorGetTypeID(void);

/* ---------------------------------------------------------------------------------------
 * THE ATTRIBUTE KEYS, IN APPLE'S ORDER, WITH THE TYPE OF THE VALUE EACH ONE CARRIES — which Apple's header
 * states in its own documentation for each key and which is carried here because it is the part of a key
 * that a caller actually has to know.
 * ------------------------------------------------------------------------------------- */
extern const CFStringRef kCTFontURLAttribute;			/* CFURLRef */
extern const CFStringRef kCTFontNameAttribute;			/* CFStringRef, the PostScript name */
extern const CFStringRef kCTFontDisplayNameAttribute;		/* CFStringRef */
extern const CFStringRef kCTFontFamilyNameAttribute;		/* CFStringRef */
extern const CFStringRef kCTFontStyleNameAttribute;		/* CFStringRef */
extern const CFStringRef kCTFontTraitsAttribute;		/* CFDictionaryRef, see CTFontTraits.h */
extern const CFStringRef kCTFontVariationAttribute;		/* CFDictionaryRef, by axis */
extern const CFStringRef kCTFontSizeAttribute;			/* CFNumberRef */
extern const CFStringRef kCTFontMatrixAttribute;		/* CFDataRef holding a CGAffineTransform */
extern const CFStringRef kCTFontCascadeListAttribute;		/* CFArrayRef of CTFontDescriptorRef */
extern const CFStringRef kCTFontCharacterSetAttribute;		/* CFCharacterSetRef */
extern const CFStringRef kCTFontLanguagesAttribute;		/* CFArrayRef of CFStringRef, BCP 47 */
extern const CFStringRef kCTFontBaselineAdjustAttribute;	/* CFNumberRef, a float */
extern const CFStringRef kCTFontMacintoshEncodingsAttribute;	/* CFNumberRef, a legacy bitfield */
extern const CFStringRef kCTFontFeaturesAttribute;		/* CFArrayRef of feature dictionaries */
extern const CFStringRef kCTFontFeatureSettingsAttribute;	/* CFArrayRef of setting dictionaries */
extern const CFStringRef kCTFontFixedAdvanceAttribute;		/* CFNumberRef */
extern const CFStringRef kCTFontOrientationAttribute;		/* CFNumberRef, a CTFontOrientation */
extern const CFStringRef kCTFontFormatAttribute;		/* CFNumberRef, a CTFontFormat */
extern const CFStringRef kCTFontRegistrationScopeAttribute;	/* CFNumberRef, a CTFontManagerScope */
extern const CFStringRef kCTFontPriorityAttribute;		/* CFNumberRef, a CTFontPriority */
extern const CFStringRef kCTFontEnabledAttribute;		/* CFNumberRef, a boolean */

/* WHICH WAY A FACE'S GLYPHS RUN, for the metrics door that asks. */
enum {
	kCTFontDefaultOrientation    = 0,
	kCTFontHorizontalOrientation = 1,
	kCTFontVerticalOrientation   = 2
};
typedef uint32_t CTFontOrientation;

/* WHAT A FACE IS MADE OF, as the attribute answers it. */
enum {
	kCTFontFormatUnrecognized       = 0,
	kCTFontFormatOpenTypePostScript = 1,
	kCTFontFormatOpenTypeTrueType   = 2,
	kCTFontFormatTrueType           = 3,
	kCTFontFormatPostScript         = 4,
	kCTFontFormatBitmap             = 5
};
typedef uint32_t CTFontFormat;

/* HOW EARLY A FACE IS CONSULTED, for the duplicates and the sorting a registry does. THE VALUES ARE
 * APPLE'S AND THEY ARE SPACED RATHER THAN CONSECUTIVE — a priority is a rank a caller may compare, not an
 * index — so the gaps are part of the contract and the suite holds them. */
enum {
	kCTFontPrioritySystem   = 10000,
	kCTFontPriorityNetwork  = 20000,
	kCTFontPriorityComputer = 30000,
	kCTFontPriorityUser     = 40000,
	kCTFontPriorityDynamic  = 50000,
	kCTFontPriorityProcess  = 60000
};
typedef uint32_t CTFontPriority;

/* ---------------------------------------------------------------------------------------
 * CREATION. A descriptor is made from a name and a size, or from an arbitrary attributes dictionary, and a
 * copy augments an existing one: the new attributes REPLACE the old except for the variation and the
 * feature settings, which MERGE — Apple says so in as many words and it is the one exception.
 * ------------------------------------------------------------------------------------- */
CTFontDescriptorRef CTFontDescriptorCreateWithNameAndSize(CFStringRef name, CGFloat size);
CTFontDescriptorRef CTFontDescriptorCreateWithAttributes(CFDictionaryRef attributes);
CTFontDescriptorRef CTFontDescriptorCreateCopyWithAttributes(CTFontDescriptorRef original,
							     CFDictionaryRef attributes);
CTFontDescriptorRef CTFontDescriptorCreateCopyWithVariation(CTFontDescriptorRef original,
							    CFNumberRef variationIdentifier,
							    CGFloat variationValue);
CTFontDescriptorRef CTFontDescriptorCreateCopyWithFeature(CTFontDescriptorRef original,
							  CFNumberRef featureTypeIdentifier,
							  CFNumberRef featureSelectorIdentifier);

/* AND THE TWO THAT NEED A REGISTRY OF INSTALLED FACES, WHICH THIS TIER NOW HAS. Apple defines their answer as "based
 * on the original descriptor and SYSTEM PRECEDENCE"; the SET is the engine's own list tested against the descriptor,
 * and THE ORDER is this tier's recorded decision (by family name — see CTFontMatching_internal.h). BOTH ARE DEFINED,
 * in `CTFontMatching.c`, because the tier that owns matching owns the doors that ask a matching question. AND THE
 * SECOND PARAMETER IS A SET OF ATTRIBUTE NAMES — every returned descriptor must CARRY them — not an options
 * dictionary, which its own name says. */
CFArrayRef CTFontDescriptorCreateMatchingFontDescriptors(CTFontDescriptorRef descriptor,
							 CFSetRef mandatoryAttributes);
CTFontDescriptorRef CTFontDescriptorCreateMatchingFontDescriptor(CTFontDescriptorRef descriptor,
								 CFSetRef mandatoryAttributes);

/* ---------------------------------------------------------------------------------------
 * ACCESS. The dictionary itself, one attribute of it, and a LOCALIZED attribute — the last being the one
 * door whose answer depends on a global language list this tier does not have yet, so it answers from the
 * attribute itself, which is what Apple's documentation says to do when localization is not possible.
 * ------------------------------------------------------------------------------------- */
CFDictionaryRef CTFontDescriptorCopyAttributes(CTFontDescriptorRef descriptor);
CFTypeRef CTFontDescriptorCopyAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute);
CFTypeRef CTFontDescriptorCopyLocalizedAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute,
						 CFStringRef *language);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTDESCRIPTOR_H */
