/*
 * CTFontDescriptor.m — a font descriptor: a dictionary of attributes, and the doors that make one.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE OBJECT IS A REFERENCE COUNT AND AN ATTRIBUTES DICTIONARY, which is what Apple's own doors
 * describe: `CTFontDescriptorCreateWithAttributes` takes "a CFDictionaryRef of arbitrary attributes"
 * and says they "will be preserved". WHAT THE FRAMEWORK DOES WITH THEM IS MATCH A FONT, and the part of
 * that this tier can do lives in `CTFontCreateWithFontDescriptor` (`CTFont.c`) — everything else is
 * preserved here and IGNORED AT FONT CREATION, which is Apple's own sentence rather than an omission:
 * "unrecognized attributes will be ignored on font creation and may not be preserved over the round
 * trip (descriptor -> font -> descriptor)".
 *
 * THE TYPES ARE THIS TREE'S, AS THEY WERE FOR THE NAME DOORS: Apple's is `CFDictionaryRef` and this tree
 * has NSDictionary, so this file is Objective-C and the header teaches a C consumer the type, exactly as
 * `CTFont.h` does for its strings. **ONE DOOR IS NOT HERE AND IS NAMED IN THE HEADER INSTEAD**:
 * `CTFontDescriptorCopyAttribute` returns Apple's `CFTypeRef` — an ARBITRARY attribute, which may be a
 * string, a number, an array, a dictionary or a colour — and this tree has no type for "any object" in a
 * C header (`CGBase.h` names six concrete ones). Guessing one would be worse than refusing it, so the
 * heterogeneous pair (`CTFontDescriptorCopyAttribute`, `CTFontDescriptorCopyLocalizedAttribute`) is the
 * unit that decides that type, and it is still `open` in the ledger.
 *
 * THE KEY VALUES ARE OURS, for the third time and the same reason: Apple publishes the names and the
 * thing each value is (`CFNumberRef`, `CFDataRef`, a dictionary) and NEVER the strings, and a key is only
 * ever compared against the constants declared beside it.
 */
#import <Foundation/Foundation.h>

#include "CTFontDescriptor_internal.h"
#include <CoreText/CTFont.h>
/* **THE KEY SETS THIS FILE'S COPY DOORS USE**: the descriptor's own attribute keys, the string-attribute keys,
 * and the SFNT feature-set keys §37 shipped — read BEFORE appending, which is §40's lesson about include sets. */
#include <CoreText/CTStringAttributes.h>
#include <CoreText/SFNTLayoutTypes.h>
/* **EVERY DOOR THIS FILE NOW CALLS, TRACED TO ITS HEADER** — which is the sharper form of §40's lesson: reading
 * the file's existing imports is not enough, because the new code calls NEW doors. `cg_font_create_with_name`
 * decides whether a descriptor already resolves to a face, and `CTFontManagerCopyAvailablePostScriptNames`
 * provides the face names a not-yet-normalized descriptor is matched against. */
#include <CoreGraphics/CGFont_internal.h>
#include <CoreText/CTFontManager.h>

/* **THE C LIBRARY, FOR THE ENABLE/DISABLE SET CT-M7k ADDED**: `strcmp` and `strdup` to keep and look up a
 * name, `realloc`/`free` for the list that grows. */
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------------------------------
 * THE ATTRIBUTE KEYS, in Apple's own order in its own header.
 * ---------------------------------------------------------------------------------------------- */
NSString *const kCTFontURLAttribute = @"CTFontURL";
NSString *const kCTFontNameAttribute = @"CTFontName";
NSString *const kCTFontDisplayNameAttribute = @"CTFontDisplayName";
NSString *const kCTFontFamilyNameAttribute = @"CTFontFamilyName";
NSString *const kCTFontStyleNameAttribute = @"CTFontStyleName";
NSString *const kCTFontTraitsAttribute = @"CTFontTraits";
NSString *const kCTFontVariationAttribute = @"CTFontVariation";
NSString *const kCTFontSizeAttribute = @"CTFontSize";
NSString *const kCTFontMatrixAttribute = @"CTFontMatrix";
NSString *const kCTFontCascadeListAttribute = @"CTFontCascadeList";
NSString *const kCTFontCharacterSetAttribute = @"CTFontCharacterSet";
NSString *const kCTFontLanguagesAttribute = @"CTFontLanguages";
NSString *const kCTFontBaselineAdjustAttribute = @"CTFontBaselineAdjust";
NSString *const kCTFontMacintoshEncodingsAttribute = @"CTFontMacintoshEncodings";
NSString *const kCTFontFeaturesAttribute = @"CTFontFeatures";
NSString *const kCTFontFeatureSettingsAttribute = @"CTFontFeatureSettings";
NSString *const kCTFontFixedAdvanceAttribute = @"CTFontFixedAdvance";
NSString *const kCTFontOrientationAttribute = @"CTFontOrientation";
NSString *const kCTFontFormatAttribute = @"CTFontFormat";
NSString *const kCTFontRegistrationScopeAttribute = @"CTFontRegistrationScope";
NSString *const kCTFontPriorityAttribute = @"CTFontPriority";
NSString *const kCTFontEnabledAttribute = @"CTFontEnabled";

/* =================================================================================================
 * THE CLASS ITSELF — `dec-35c114bf75c7c2d7` (user, 2026-10-09): A CORE TEXT OBJECT IS A REAL
 * OBJECTIVE-C OBJECT. Apple's `CTFontDescriptorRef` is a CFType, a CFType lives in a dictionary, and
 * this tree has neither CoreFoundation nor a bridge; the class is how that is spelled here, so an
 * attribute value can BE the object rather than a wrapper around it.
 *
 * **IT SUBCLASSES `NSObject` RATHER THAN BEING A HAND-WRITTEN ROOT CLASS, AND THAT IS A MEASURED
 * ECONOMY**: this library already links Foundation (the descriptor's whole dictionary half is
 * Objective-C), and a root class owes the runtime about fifteen methods — retain, release, autorelease,
 * retainCount, dealloc, class, self, zone, isProxy, isKindOfClass:, respondsToSelector:, hash, isEqual:,
 * description, copyWithZone: — every one of which NSObject already implements, on the SAME refcount. The
 * C API is untouched: `CTFontDescriptorRef` is a pointer to this class.
 *
 * **THE IVARS ARE `@public` FOR ONE REASON**: the accessors below are C functions (`ctf_descriptor_*`)
 * that the C half of this library calls, and a C caller cannot read an Objective-C ivar. `attributes` is
 * `id` here rather than the `void *` the old struct had, because inside the class there is no reason to
 * lie about what it is.
 * =============================================================================================== */
@interface CTFontDescriptor : NSObject {
	@public
	id attributes;	/* the attributes dictionary, RETAINED by this object's own lifetime */
}
@end

@implementation CTFontDescriptor

/* THE DICTIONARY'S LIFETIME ENDS HERE, which is what the hand-rolled refcount did in its own `free`
 * path — and `-dealloc` is now the only place that happens. */
- (void)dealloc
{
	[attributes release];
	[super dealloc];
}

@end

/* THE REFERENCE COUNT, WHICH IS THIS TIER'S DOOR AND NOT APPLE'S — the same pair `CTFont` carries for
 * the same reason: Apple's descriptor is a CFType and this tree has no CoreFoundation, so without this a
 * caller could not own a descriptor it created. No ledger row is claimed for it. **AND IT IS NOW A
 * WRAPPER: the count is the runtime's.** */
CTFontDescriptorRef CTFontDescriptorRetain(CTFontDescriptorRef descriptor)
{
	return (CTFontDescriptorRef)[(id)(uintptr_t)descriptor retain];
}

void CTFontDescriptorRelease(CTFontDescriptorRef descriptor)
{
	[(id)(uintptr_t)descriptor release];
}

/* THE ONE CONSTRUCTOR EVERY OTHER DOOR BELOW GOES THROUGH, so the object's invariants live in one
 * place: the dictionary is RETAINED (not copied), and a nil dictionary is an EMPTY one rather than a
 * refusal — Apple's door takes a dictionary and says nothing about NULL, while a descriptor with no
 * attributes is a perfectly ordinary descriptor ("the global cascade list is used" is what an absent
 * cascade means, and the default size is 12.0 when none is given). */
static CTFontDescriptorRef ctf_descriptor_with_dictionary(NSDictionary *attributes)
{
	CTFontDescriptor *d = [[CTFontDescriptor alloc] init];

	if (d == nil) {
		return NULL;
	}
	/* NO `retain = 1` LINE ANY MORE: `+alloc` comes back with a count of one, which is what that line
	 * was spelling by hand. */
	d->attributes = [(attributes != nil) ? attributes : [NSDictionary dictionary] retain];
	return (CTFontDescriptorRef)(uintptr_t)d;
}

CTFontDescriptorRef CTFontDescriptorCreateWithAttributes(NSDictionary *attributes)
{
	return ctf_descriptor_with_dictionary(attributes);
}

CTFontDescriptorRef CTFontDescriptorCreateWithNameAndSize(NSString *name, CGFloat size)
{
	NSMutableDictionary *attributes = [NSMutableDictionary dictionary];

	/* **APPLE'S OWN RULE, QUOTED BECAUSE IT MAKES "ABSENT" A REAL STATE**: "The point size. If 0.0, the
	 * kCTFontSizeAttribute will be OMITTED from the font descriptor." So a zero size does not store a
	 * zero — it stores nothing, and a font made from this descriptor gets the create door's default.
	 * A nil name is treated the same way, for the same reason (there is nothing to store). */
	if (name != nil) {
		[attributes setObject:name forKey:kCTFontNameAttribute];
	}
	if (size != 0.0) {
		[attributes setObject:[NSNumber numberWithDouble:(double)size] forKey:kCTFontSizeAttribute];
	}
	return ctf_descriptor_with_dictionary(attributes);
}

CTFontDescriptorRef CTFontDescriptorCreateCopyWithAttributes(CTFontDescriptorRef original,
							     NSDictionary *attributes)
{
	NSMutableDictionary *merged;
	id key;

	/* A nil original is not documented and is answered as "no original", which makes this door a plain
	 * create rather than a crash.
	 *
	 * **TWO STEPS RATHER THAN `+dictionaryWithDictionary:`, AND THE REASON IS A HEADER'S RETURN TYPE**:
	 * this tree's Foundation declares that constructor as `+ (NSDictionary *)…` where Apple's returns
	 * `instancetype`, so assigning it to an `NSMutableDictionary *` is a warning under `-Wextra` — and a
	 * warning that a future reader would have to investigate to learn that the object IS mutable, because
	 * `+[NSMutableDictionary dictionary…]` is what built it. Building the mutable dictionary and adding
	 * the entries says the same thing with the compiler able to see it. */
	merged = [NSMutableDictionary dictionary];
	[merged addEntriesFromDictionary:
	 ((original != NULL) ? (NSDictionary *)original->attributes : [NSDictionary dictionary])];
	for (key in attributes) {
		id value = [attributes objectForKey:key];
		id existing = [merged objectForKey:key];

		/* **THE DOCUMENTED MERGE EXCEPTION, AND IT IS THE ONE THING THIS DOOR DOES THAT A DICTIONARY
		 * ASSIGNMENT WOULD NOT**: "If there are conflicts between attributes, the new attributes will
		 * replace existing ones, EXCEPT FOR kCTFontVariationAttribute and kCTFontFeatureSettingsAttribute
		 * WHICH WILL BE MERGED." Both of those are dictionaries of settings, so the merge is a dictionary
		 * merge — the new entries win, the old ones the caller did not mention survive. */
		if (existing != nil && [value isKindOfClass:[NSDictionary class]] &&
		    ([key isEqual:kCTFontVariationAttribute] || [key isEqual:kCTFontFeatureSettingsAttribute])) {
			/* THE SAME TWO-STEP SHAPE AS ABOVE, for the same header reason — and the ORDER is the
			 * contract: the EXISTING entries go in first and the new ones second, because the
			 * documentation says the new attributes win the conflicts. */
			NSMutableDictionary *both = [NSMutableDictionary dictionary];

			[both addEntriesFromDictionary:existing];
			[both addEntriesFromDictionary:value];
			[merged setObject:both forKey:key];
		} else {
			[merged setObject:value forKey:key];
		}
	}
	return ctf_descriptor_with_dictionary(merged);
}

NSDictionary *CTFontDescriptorCopyAttributes(CTFontDescriptorRef descriptor)
{
	if (descriptor == NULL) {
		return nil;
	}
	/* +1, WHICH IS WHAT `Copy` MEANS. Apple adds "This dictionary will contain the MINIMUM number of
	 * attributes to fully specify this particular font descriptor"; THIS TIER DOES NOT SLIM THE
	 * DICTIONARY — it answers the attributes it was given, which do fully specify the descriptor it is
	 * answering for. That is a stated deviation rather than an oversight: slimming means knowing which
	 * attributes the matcher acts on, and this tier's matcher (the registry, by name) acts on three. */
	return [(NSDictionary *)descriptor->attributes retain];
}

/* -------------------------------------------------------------------------------------------------
 * AND THE C SIDE'S THREE QUESTIONS, which are the three attributes a font can be made from.
 * ---------------------------------------------------------------------------------------------- */

int ctf_descriptor_name_utf8(CTFontDescriptorRef descriptor, char *buf, size_t size)
{
	NSDictionary *attributes;
	id value;
	const char *utf8;
	size_t n;

	if (descriptor == NULL || buf == NULL || size == 0) {
		return 0;
	}
	attributes = (NSDictionary *)descriptor->attributes;
	value = [attributes objectForKey:kCTFontNameAttribute];
	if (value == nil || ![value isKindOfClass:[NSString class]]) {
		return 0;
	}
	utf8 = [(NSString *)value UTF8String];
	if (utf8 == NULL) {
		return 0;
	}
	n = strlen(utf8);
	if (n + 1 > size) {
		return 0;
	}
	memcpy(buf, utf8, n);
	buf[n] = '\0';
	return (int)n;
}

int ctf_descriptor_size(CTFontDescriptorRef descriptor, CGFloat *size)
{
	NSDictionary *attributes;
	id value;

	if (descriptor == NULL || size == NULL) {
		return 0;
	}
	attributes = (NSDictionary *)descriptor->attributes;
	value = [attributes objectForKey:kCTFontSizeAttribute];
	if (value == nil || ![value isKindOfClass:[NSNumber class]]) {
		return 0;
	}
	*size = (CGFloat)[(NSNumber *)value doubleValue];
	return 1;
}

int ctf_descriptor_matrix(CTFontDescriptorRef descriptor, CGAffineTransform *matrix)
{
	NSDictionary *attributes;
	id value;
	CGAffineTransform m;

	if (descriptor == NULL || matrix == NULL) {
		return 0;
	}
	attributes = (NSDictionary *)descriptor->attributes;
	value = [attributes objectForKey:kCTFontMatrixAttribute];
	if (value == nil || ![value isKindOfClass:[NSData class]] ||
	    [(NSData *)value length] < sizeof(CGAffineTransform)) {
		return 0;
	}
	memcpy(&m, [(NSData *)value bytes], sizeof m);
	/* **"OF WHICH ONLY THE a, b, c, AND d FIELDS ARE USED"** — Apple's own sentence for this attribute,
	 * so the translation part is ZEROED rather than carried: a descriptor's matrix is a scale and a
	 * shear, and a caller who packed a translation into the data does not get it applied behind their
	 * back. */
	m.tx = 0.0;
	m.ty = 0.0;
	*matrix = m;
	return 1;
}

CTFontDescriptorRef ctf_descriptor_from_font(CTFontRef font)
{
	NSMutableDictionary *attributes;
	CGAffineTransform matrix;
	NSString *name;

	if (font == NULL) {
		return NULL;
	}
	attributes = [NSMutableDictionary dictionary];
	/* THE POSTSCRIPT NAME IS THE `kCTFontNameAttribute` — Apple's definition of that key is the font's
	 * PostScript name, and it is the one name this tier's registry resolves most directly. */
	name = CTFontCopyPostScriptName(font);
	if (name != nil) {
		[attributes setObject:name forKey:kCTFontNameAttribute];
		[name release];
	}
	[attributes setObject:[NSNumber numberWithDouble:(double)CTFontGetSize(font)]
		       forKey:kCTFontSizeAttribute];
	matrix = CTFontGetMatrix(font);
	[attributes setObject:[NSData dataWithBytes:&matrix length:sizeof matrix]
		       forKey:kCTFontMatrixAttribute];
	return ctf_descriptor_with_dictionary(attributes);
}

/* ====================================================================================================
 * M6o: THE DESCRIPTOR'S COPY AND LOOKUP DOORS.
 * ================================================================================================== */

/* "A retained reference to the requested attribute, or **NULL if** the requested attribute is not present." */
CFTypeRef CTFontDescriptorCopyAttribute(CTFontDescriptorRef descriptor, NSString *attribute)
{
	if (descriptor == NULL || attribute == nil) {
		return NULL;
	}
	return (CFTypeRef)[(id)[(NSDictionary *)descriptor->attributes objectForKey:attribute] retain];
}

/* **THE ERA'S OWN FALLBACK, IMPLEMENTED AS WRITTEN**: "returns a localized attribute based on the global
 * language list. **If localization is not possible for the attribute the behavior matches
 * `CTFontDescriptorCopyAttribute()`**." The global language list is a SYSTEM input this tier does not have, and
 * the era says what to do without it — so this is the door's own answer rather than a stub, and `*language` is
 * left NULL, which the contract allows. */
CFTypeRef CTFontDescriptorCopyLocalizedAttribute(CTFontDescriptorRef descriptor, NSString *attribute,
						 NSString **language)
{
	if (language != NULL) {
		*language = NULL;
	}
	return CTFontDescriptorCopyAttribute(descriptor, attribute);
}

/* THE MERGE IS DONE HERE rather than through `CreateCopyWithAttributes`, because these two attributes are the
 * dictionary- and array-valued ones a copy must COMBINE with whatever was already there. */
static CTFontDescriptorRef ctd_copy_with(CTFontDescriptorRef original, NSString *key,
					 void (^merge)(id existing, NSMutableDictionary *target))
{
	NSMutableDictionary *merged = [NSMutableDictionary dictionary];
	NSDictionary *existing;

	if (original != NULL) {
		[merged addEntriesFromDictionary:(NSDictionary *)original->attributes];
	}
	existing = [merged objectForKey:key];
	merge(existing, merged);
	return ctf_descriptor_with_dictionary(merged);
}

/* "A copy of the original font descriptor modified with the given feature settings" — a new ENTRY appended to
 * the feature-settings array, whose two keys are the SFNT vocabulary §37 shipped. */
CTFontDescriptorRef CTFontDescriptorCreateCopyWithFeature(CTFontDescriptorRef original,
							  NSNumber *featureTypeIdentifier,
							  NSNumber *featureSelectorIdentifier)
{
	return ctd_copy_with(original, kCTFontFeatureSettingsAttribute,
			     ^(id existing, NSMutableDictionary *target) {
		NSMutableArray *settings = [NSMutableArray array];
		NSMutableDictionary *entry = [NSMutableDictionary dictionary];

		if (existing != nil) {
			[settings addObjectsFromArray:(NSArray *)existing];
		}
		if (featureTypeIdentifier != nil) {
			[entry setObject:featureTypeIdentifier forKey:kCTFontFeatureTypeIdentifierKey];
		}
		if (featureSelectorIdentifier != nil) {
			[entry setObject:featureSelectorIdentifier forKey:kCTFontFeatureSelectorIdentifierKey];
		}
		[settings addObject:entry];
		[target setObject:settings forKey:kCTFontFeatureSettingsAttribute];
	});
}

/* "The variation axis identifier ... the four character code of the variation axis as a CFNumberRef" — so the
 * identifier is the KEY of the variation dictionary and the value is the instance's. */
CTFontDescriptorRef CTFontDescriptorCreateCopyWithVariation(CTFontDescriptorRef original,
							    NSNumber *variationIdentifier,
							    CGFloat variationValue)
{
	return ctd_copy_with(original, kCTFontVariationAttribute,
			     ^(id existing, NSMutableDictionary *target) {
		NSMutableDictionary *axes = [NSMutableDictionary dictionary];

		if (existing != nil) {
			[axes addEntriesFromDictionary:(NSDictionary *)existing];
		}
		if (variationIdentifier != nil) {
			[axes setObject:[NSNumber numberWithDouble:(double)variationValue]
				 forKey:(NSString *)variationIdentifier];
		}
		[target setObject:axes forKey:kCTFontVariationAttribute];
	});
}

/* AND THE TWO KEYS §37 DECLARED, valued as every key in this tier is. */
NSString *const kCTFontFeatureTypeIdentifierKey = @"CTFontFeatureTypeIdentifier";
NSString *const kCTFontFeatureSelectorIdentifierKey = @"CTFontFeatureSelectorIdentifier";

/* **THE MATCHING DOOR, DEFINED BECAUSE A DECLARATION WITHOUT A DEFINITION IS NOT ALLOWED TO SHIP.** The era's
 * sentence gives both halves: "an array of font normalized font descriptors matching the attributes present in
 * descriptor. **If descriptor itself is normalized then the array will contain only one item, the original
 * descriptor.**" So:
 *
 *   * a descriptor that already RESOLVES to a face — its name attribute names one this tree can open — IS
 *     normalized, and the answer is that descriptor alone;
 *   * otherwise the answer is the NORMALIZED descriptors of the available faces, filtered by the mandatory
 *     attributes the caller named. Normalized here means what the era means: a descriptor resolved to a
 *     specific font, which is exactly what `CTFontDescriptorCreateWithNameAndSize` builds from each name the
 *     font manager publishes (`CTFontManagerCopyAvailablePostScriptNames`).
 *
 * The `mandatoryAttributes` keys are compared by VALUE, which is the era's own word ("required to be
 * identically matched"). An attribute a candidate does not carry cannot match a required one. */
/* **THE DISABLED NAMES, AND WHY A LIST RATHER THAN A SET** (CT-M7k): the Foundation container would be a
 * `NSSet`, but the only two operations are "add these names" and "is this name in it", the second of which runs
 * once per candidate in the matcher — and this tier's `-containsObject:` walks the same array a hand-written
 * loop does. What the list IS is a process-scoped registry, like the font registry CT-M7h added one framework
 * down, and it grows rather than being bounded: a bounded table would need an invented refusal for the full
 * case, and Apple's vocabulary has none for it either. */
static char **ctp_disabled_names;
static int ctp_disabled_count;
static int ctp_disabled_cap;

static int ctp_disabled_index(const char *name)
{
	int i;

	for (i = 0; i < ctp_disabled_count; i++) {
		if (strcmp(ctp_disabled_names[i], name) == 0) {
			return i;
		}
	}
	return -1;
}

int ctf_descriptor_name_is_enabled(const char *name)
{
	return (name == NULL || ctp_disabled_index(name) < 0) ? 1 : 0;
}

void ctf_descriptor_set_enabled(NSArray *descriptors, BOOL enable)
{
	NSUInteger i;

	if (descriptors == nil) {
		return;
	}
	for (i = 0; i < [descriptors count]; i++) {
		id one = [descriptors objectAtIndex:i];
		char name[512];
		int at;

		/* A DESCRIPTOR THIS TIER CANNOT NAME CHANGES NOTHING — it could not be recognised again either. */
		if (one == nil || ![one isKindOfClass:[CTFontDescriptor class]]) {
			continue;
		}
		if (!ctf_descriptor_name_utf8((CTFontDescriptorRef)one, name, sizeof name)) {
			continue;
		}
		at = ctp_disabled_index(name);
		if (enable) {
			if (at >= 0) {
				free(ctp_disabled_names[at]);
				for (; at + 1 < ctp_disabled_count; at++) {
					ctp_disabled_names[at] = ctp_disabled_names[at + 1];
				}
				ctp_disabled_count--;
			}
			continue;
		}
		if (at >= 0) {
			continue;	/* already disabled; disabling twice is not an error */
		}
		if (ctp_disabled_count == ctp_disabled_cap) {
			int cap = (ctp_disabled_cap == 0) ? 8 : ctp_disabled_cap * 2;
			char **grown = realloc(ctp_disabled_names,
					       (size_t)cap * sizeof ctp_disabled_names[0]);

			if (grown == NULL) {
				continue;
			}
			ctp_disabled_names = grown;
			ctp_disabled_cap = cap;
		}
		ctp_disabled_names[ctp_disabled_count] = strdup(name);
		if (ctp_disabled_names[ctp_disabled_count] != NULL) {
			ctp_disabled_count++;
		}
	}
}

NSArray *CTFontDescriptorCreateMatchingFontDescriptors(CTFontDescriptorRef descriptor, NSSet *mandatoryAttributes)
{
	NSMutableArray *matches = [NSMutableArray array];
	char name[512];
	NSArray *names;

	if (descriptor == NULL) {
		return (NSArray *)matches;
	}
	/* ALREADY NORMALIZED: the descriptor names a face this tree can open. */
	if (ctf_descriptor_name_utf8(descriptor, name, sizeof name) && cg_font_create_with_name(name) != NULL) {
		[matches addObject:(id)descriptor];
		return (NSArray *)matches;
	}
	names = CTFontManagerCopyAvailablePostScriptNames();
	for (NSUInteger i = 0; names != nil && i < [names count]; i++) {
		CTFontDescriptorRef candidate =
			CTFontDescriptorCreateWithNameAndSize((NSString *)[names objectAtIndex:i], 0.0);
		int matched = (candidate != NULL);

		if (matched && mandatoryAttributes != nil) {
			for (id key in mandatoryAttributes) {
				id required = [(NSDictionary *)descriptor->attributes objectForKey:key];
				id present = [(NSDictionary *)candidate->attributes objectForKey:key];

				if (required == nil) {
					continue;	/* nothing was required of this key */
				}
				if (present == nil || ![present isEqual:required]) {
					matched = 0;
					break;
				}
			}
		}
		/* **AND A DISABLED DESCRIPTOR IS NEVER A MATCH** (CT-M7k). Apple's word is "matching" and nothing else:
		 * a disabled face is still AVAILABLE — the enumeration, the URL list and the name lookup all still see
		 * it — and only this door stops answering it. That distinction is the one the probe checks, because a
		 * door that DISABLED by unregistering would pass every match-count check and lose the font. */
		if (matched && !ctf_descriptor_name_is_enabled([(NSString *)[names objectAtIndex:i] UTF8String])) {
			matched = 0;
		}
		if (matched) {
			[matches addObject:(id)candidate];
		}
		CTFontDescriptorRelease(candidate);
	}
	return (NSArray *)matches;
}

/* **M7l: THE SINGLE PREFERRED MATCH — THE PLURAL'S FIRST ELEMENT, BY CONSTRUCTION RATHER THAN BY A SECOND
 * RULE.** Apple's sentence names "system precedence" as the ordering, and the ordering is the ONLY thing that
 * decides which match is "preferred"; re-deciding it here would be a second matcher beside the one above, and
 * the two could disagree about a face without either being wrong about its own answer. So this asks the plural
 * and takes its first. */
CTFontDescriptorRef CTFontDescriptorCreateMatchingFontDescriptor(CTFontDescriptorRef descriptor,
								 NSSet *mandatoryAttributes)
{
	NSArray *matches = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, mandatoryAttributes);
	CTFontDescriptorRef first = NULL;

	if (matches != nil && [matches count] > 0) {
		first = [(CTFontDescriptorRef)[matches objectAtIndex:0] retain];
	}
	[matches release];
	return first;
}
