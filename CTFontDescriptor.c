/*
 * CTFontDescriptor.c — the attribute keys, which are the part of this header with a value at run time.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE TWENTY-TWO STRINGS ARE A RECORDED GUESS AND THE HEADER SAYS WHY AT LENGTH: Apple publishes no value
 * for any of these keys anywhere, eight of them are provably "NSFont<Suffix>Attribute" because the AppKit
 * declares the same eight and a dictionary has to cross between the frameworks, and the other fourteen have
 * no counterpart to check against. The user's decision is that all twenty-two take that form
 * (`dec-4795696b09d362f6`), recorded here where the values are rather than asserted as a fact.
 */
#include <CoreText/CTFontDescriptor.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdlib.h>

const CFStringRef kCTFontURLAttribute = CFSTR("NSFontURLAttribute");
const CFStringRef kCTFontNameAttribute = CFSTR("NSFontNameAttribute");
const CFStringRef kCTFontDisplayNameAttribute = CFSTR("NSFontDisplayNameAttribute");
const CFStringRef kCTFontFamilyNameAttribute = CFSTR("NSFontFamilyNameAttribute");
const CFStringRef kCTFontStyleNameAttribute = CFSTR("NSFontStyleNameAttribute");
const CFStringRef kCTFontTraitsAttribute = CFSTR("NSFontTraitsAttribute");
const CFStringRef kCTFontVariationAttribute = CFSTR("NSFontVariationAttribute");
const CFStringRef kCTFontSizeAttribute = CFSTR("NSFontSizeAttribute");
const CFStringRef kCTFontMatrixAttribute = CFSTR("NSFontMatrixAttribute");
const CFStringRef kCTFontCascadeListAttribute = CFSTR("NSFontCascadeListAttribute");
const CFStringRef kCTFontCharacterSetAttribute = CFSTR("NSFontCharacterSetAttribute");
const CFStringRef kCTFontLanguagesAttribute = CFSTR("NSFontLanguagesAttribute");
const CFStringRef kCTFontBaselineAdjustAttribute = CFSTR("NSFontBaselineAdjustAttribute");
const CFStringRef kCTFontMacintoshEncodingsAttribute = CFSTR("NSFontMacintoshEncodingsAttribute");
const CFStringRef kCTFontFeaturesAttribute = CFSTR("NSFontFeaturesAttribute");
const CFStringRef kCTFontFeatureSettingsAttribute = CFSTR("NSFontFeatureSettingsAttribute");
const CFStringRef kCTFontFixedAdvanceAttribute = CFSTR("NSFontFixedAdvanceAttribute");
const CFStringRef kCTFontOrientationAttribute = CFSTR("NSFontOrientationAttribute");
const CFStringRef kCTFontFormatAttribute = CFSTR("NSFontFormatAttribute");
const CFStringRef kCTFontRegistrationScopeAttribute = CFSTR("NSFontRegistrationScopeAttribute");
const CFStringRef kCTFontPriorityAttribute = CFSTR("NSFontPriorityAttribute");
const CFStringRef kCTFontEnabledAttribute = CFSTR("NSFontEnabledAttribute");

/* =================================================================================================
 * THE OBJECT. A descriptor is its attributes dictionary and nothing else: Apple's own sentence for
 * `CTFontDescriptorCopyAttributes` is "this dictionary will contain the minimum number of attributes to
 * fully specify this particular font descriptor", so the dictionary IS the value — which is what makes a
 * descriptor comparable with `CFEqual` and hashable with `CFHash`.
 *
 * IT IS REGISTERED WITH CORE FOUNDATION'S RUNTIME AND IS NOT A CLASS, which is what Apple's is and what
 * this library is for. `libagora-drawing`'s `CGColor` is the pattern, carried into pure C: `CFRuntimeBase`
 * first, a finalizer that releases what the object owns, and an identity that comes from the runtime rather
 * than from a table of numbers kept beside it.
 * =============================================================================================== */
struct __CTFontDescriptor {
	CFRuntimeBase base;
	CFDictionaryRef attributes;	/* RETAINED: the descriptor owns its attributes */
};

static void ct_descriptor_finalize(CFTypeRef cf)
{
	struct __CTFontDescriptor *descriptor = (struct __CTFontDescriptor *)cf;

	if (descriptor->attributes != NULL) {
		CFRelease(descriptor->attributes);
	}
}

/* TWO DESCRIPTORS ARE EQUAL WHEN THEIR ATTRIBUTES ARE — the value IS the dictionary — and a descriptor
 * hashes as its attributes do. Without these two a descriptor would compare by pointer, which would make
 * `CFEqual` on two descriptors made from the same attributes answer false. */
static Boolean ct_descriptor_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTFontDescriptor *a = (const struct __CTFontDescriptor *)cf1;
	const struct __CTFontDescriptor *b = (const struct __CTFontDescriptor *)cf2;

	if (a->attributes == NULL || b->attributes == NULL) {
		return (a->attributes == b->attributes);
	}
	return CFEqual(a->attributes, b->attributes);
}

static CFHashCode ct_descriptor_hash(CFTypeRef cf)
{
	const struct __CTFontDescriptor *descriptor = (const struct __CTFontDescriptor *)cf;

	return (descriptor->attributes == NULL) ? 0 : CFHash(descriptor->attributes);
}

static const CFRuntimeClass ct_descriptor_class = {
	0,                      /* version */
	"CTFontDescriptor",     /* className */
	NULL,                   /* init */
	NULL,                   /* copy */
	ct_descriptor_finalize, /* finalize */
	ct_descriptor_equal,    /* equal */
	ct_descriptor_hash,     /* hash */
	NULL,                   /* copyFormattingDesc */
	NULL,                   /* copyDebugDesc */
	NULL,                   /* reclaim */
	NULL,                   /* refcount */
	0                       /* requiredAlignment */
};

/* THE IDENTITY COMES FROM THE RUNTIME, so `CFGetTypeID(descriptor) == CTFontDescriptorGetTypeID()` — which
 * is Apple's contract and the reason this is not a table of constants kept in a file beside the objects. */
CFTypeID CTFontDescriptorGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_descriptor_class);
	}
	return type_id;
}

static struct __CTFontDescriptor *ct_descriptor_alloc(CFDictionaryRef attributes)
{
	struct __CTFontDescriptor *descriptor = (struct __CTFontDescriptor *)_CFRuntimeCreateInstance(
		kCFAllocatorDefault, CTFontDescriptorGetTypeID(),
		(CFIndex)(sizeof(struct __CTFontDescriptor) - sizeof(CFRuntimeBase)), NULL);

	if (descriptor == NULL) {
		return NULL;
	}
	descriptor->attributes = (attributes == NULL) ? NULL : (CFDictionaryRef)CFRetain(attributes);
	return descriptor;
}

CTFontDescriptorRef CTFontDescriptorCreateWithAttributes(CFDictionaryRef attributes)
{
	return (CTFontDescriptorRef)ct_descriptor_alloc(attributes);
}

/* THE SIZE IS OMITTED RATHER THAN DEFAULTED WHEN IT IS 0.0, which is Apple's own sentence for this door:
 * "If 0.0, the kCTFontSizeAttribute will be omitted from the font descriptor". */
CTFontDescriptorRef CTFontDescriptorCreateWithNameAndSize(CFStringRef name, CGFloat size)
{
	CFMutableDictionaryRef attributes = NULL;
	CFNumberRef point_size = NULL;
	struct __CTFontDescriptor *descriptor;

	if (name == NULL) {
		return NULL;
	}
	attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					       &kCFTypeDictionaryValueCallBacks);
	if (attributes == NULL) {
		return NULL;
	}
	CFDictionarySetValue(attributes, kCTFontNameAttribute, name);
	if (size > 0.0) {
		point_size = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &size);
		if (point_size != NULL) {
			CFDictionarySetValue(attributes, kCTFontSizeAttribute, point_size);
			CFRelease(point_size);
		}
	}
	descriptor = ct_descriptor_alloc(attributes);
	CFRelease(attributes);
	return (CTFontDescriptorRef)descriptor;
}

/* THE MERGE EXCEPTION IS PART OF THIS DOOR RATHER THAN A DETAIL: Apple says the new attributes replace the
 * old "except for kCTFontVariationAttribute and kCTFontFeatureSettingsAttribute WHICH WILL BE MERGED", and a
 * descriptor that quietly replaced those two would be wrong in a way nothing else would notice. */
static CFDictionaryRef ct_descriptor_merged(CFDictionaryRef original, CFDictionaryRef additions)
{
	CFMutableDictionaryRef merged;
	CFIndex i, count;
	const void **keys = NULL, **values = NULL;

	if (original == NULL) {
		return (additions == NULL) ? NULL : (CFDictionaryRef)CFRetain(additions);
	}
	if (additions == NULL) {
		return (CFDictionaryRef)CFRetain(original);
	}
	merged = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, original);
	if (merged == NULL) {
		return NULL;
	}
	count = CFDictionaryGetCount(additions);
	keys = (const void **)malloc((size_t)count * sizeof(void *));
	values = (const void **)malloc((size_t)count * sizeof(void *));
	if (keys == NULL || values == NULL) {
		free(keys);
		free(values);
		CFRelease(merged);
		return NULL;
	}
	CFDictionaryGetKeysAndValues(additions, keys, values);
	for (i = 0; i < count; i++) {
		CFDictionaryRef addition = (CFDictionaryRef)values[i];
		Boolean merges = CFEqual(keys[i], kCTFontVariationAttribute) ||
				 CFEqual(keys[i], kCTFontFeatureSettingsAttribute);
		CFTypeRef existing = CFDictionaryGetValue(original, keys[i]);

		if (merges && existing != NULL && CFGetTypeID(existing) == CFDictionaryGetTypeID() &&
		    CFGetTypeID(addition) == CFDictionaryGetTypeID()) {
			CFMutableDictionaryRef both =
				CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0,
							      (CFDictionaryRef)existing);
			CFIndex k, n;

			if (both == NULL) {
				continue;
			}
			n = CFDictionaryGetCount(addition);
			{
				const void **ak = (const void **)malloc((size_t)n * sizeof(void *));
				const void **av = (const void **)malloc((size_t)n * sizeof(void *));

				if (ak != NULL && av != NULL) {
					CFDictionaryGetKeysAndValues(addition, ak, av);
					for (k = 0; k < n; k++) {
						CFDictionarySetValue(both, ak[k], av[k]);
					}
				}
				free(ak);
				free(av);
			}
			CFDictionarySetValue(merged, keys[i], both);
			CFRelease(both);
		} else {
			CFDictionarySetValue(merged, keys[i], values[i]);
		}
	}
	free(keys);
	free(values);
	return merged;
}

CTFontDescriptorRef CTFontDescriptorCreateCopyWithAttributes(CTFontDescriptorRef original,
							     CFDictionaryRef attributes)
{
	const struct __CTFontDescriptor *descriptor = (const struct __CTFontDescriptor *)original;
	CFDictionaryRef merged;
	struct __CTFontDescriptor *copy;

	if (descriptor == NULL) {
		return NULL;
	}
	merged = ct_descriptor_merged(descriptor->attributes, attributes);
	if (merged == NULL) {
		return NULL;
	}
	copy = ct_descriptor_alloc(merged);
	CFRelease(merged);
	return (CTFontDescriptorRef)copy;
}

/* THE TWO CONVENIENCE COPIES build the setting dictionary they are a spelling of, and hand it to the
 * copy door above — so the merge rule is stated once rather than three times. */
CTFontDescriptorRef CTFontDescriptorCreateCopyWithVariation(CTFontDescriptorRef original,
							    CFNumberRef variationIdentifier,
							    CGFloat variationValue)
{
	CFMutableDictionaryRef variation;
	CFDictionaryRef wrapper;
	CFNumberRef value;
	CTFontDescriptorRef copy;

	if (variationIdentifier == NULL) {
		return NULL;
	}
	variation = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
					      &kCFTypeDictionaryValueCallBacks);
	if (variation == NULL) {
		return NULL;
	}
	value = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &variationValue);
	if (value == NULL) {
		CFRelease(variation);
		return NULL;
	}
	CFDictionarySetValue(variation, variationIdentifier, value);
	CFRelease(value);
	wrapper = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&kCTFontVariationAttribute,
				     (const void **)&variation, 1, &kCFTypeDictionaryKeyCallBacks,
				     &kCFTypeDictionaryValueCallBacks);
	CFRelease(variation);
	if (wrapper == NULL) {
		return NULL;
	}
	copy = CTFontDescriptorCreateCopyWithAttributes(original, wrapper);
	CFRelease(wrapper);
	return copy;
}

/* AND THE FEATURE COPY IS OWED FOR A REASON THAT IS THE INCLUDE GRAPH ITSELF: it is a spelling of a
 * feature-SETTINGS dictionary, whose two keys — kCTFontFeatureTypeIdentifierKey and
 * kCTFontFeatureSelectorIdentifierKey — belong to CTFont.h, and CTFont.h includes THIS header, so this file
 * cannot include it back. Declaring the two keys here and defining them elsewhere would leave the library
 * with an undefined DATA symbol, which fails at LOAD rather than at link -- the eager resolution that bit
 * Core Foundation's own library once. So the door is declared and not defined, like the two matchers, and a
 * caller who reaches for it gets a link error instead of a wrong answer. It lands with CTFont.h.
 */

/* THE ACCESSORS. `Copy` names a +1 result in this API, which is what the two below return and what the
 * header declares. AND THE ATTRIBUTES COME BACK AS THEY WERE GIVEN: Apple's contract says the dictionary
 * contains "the minimum number of attributes to fully specify this particular font descriptor", and
 * reducing an arbitrary attribute set to that minimum means asking an installed face what it is — a
 * registry this tier does not have yet. Recorded rather than approximated. */
CFDictionaryRef CTFontDescriptorCopyAttributes(CTFontDescriptorRef descriptor)
{
	const struct __CTFontDescriptor *d = (const struct __CTFontDescriptor *)descriptor;

	if (d == NULL || d->attributes == NULL) {
		return NULL;
	}
	return (CFDictionaryRef)CFRetain(d->attributes);
}

CFTypeRef CTFontDescriptorCopyAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute)
{
	const struct __CTFontDescriptor *d = (const struct __CTFontDescriptor *)descriptor;
	CFTypeRef value;

	if (d == NULL || d->attributes == NULL || attribute == NULL) {
		return NULL;
	}
	value = CFDictionaryGetValue(d->attributes, attribute);
	return (value == NULL) ? NULL : CFRetain(value);
}

/* LOCALIZATION NEEDS A GLOBAL LANGUAGE LIST THIS TIER DOES NOT HAVE, so this answers from the attribute
 * itself — which is what Apple's documentation says the function does when localization is not possible —
 * and sets the matched language to NULL rather than leaving the caller's variable untouched. */
CFTypeRef CTFontDescriptorCopyLocalizedAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute,
						 CFStringRef *language)
{
	if (language != NULL) {
		*language = NULL;
	}
	return CTFontDescriptorCopyAttribute(descriptor, attribute);
}
