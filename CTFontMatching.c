/*
 * CTFontMatching.c — the matching tier: the faces a system has, and which of them a query describes.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE SET IS THE ENGINE'S OWN LIST SEEN THROUGH A QUERY, which is the same observation the manager's three `Copy` doors
 * record one file over: `cg_font_enumerate` walks the configured font directories AND the process's registrations, so
 * "what faces exist" is ONE walk, and what this file adds is the two things a matching door needs on top of it —
 * A DESCRIPTOR PER FACE, and A TEST FOR WHETHER A QUERY DESCRIBES ONE.
 *
 * THE DESCRIPTORS ARE BUILT FROM WHAT THE FACE ITSELF SAYS: its file, its PostScript name and its family, read from the
 * `name` table through the engine's own door (`cg_font_name_entry`), because Core Graphics has no door for a family.
 * NO ATTRIBUTE IS INVENTED — the other twenty a descriptor can carry are not derivable from a file by this tier.
 *
 * AND THE ORDER IS RECORDED RATHER THAN GUESSED: Apple defers it to "system precedence" and prints no rule for it, so
 * the array is sorted by FAMILY NAME with `CTFontManagerCompareFontFamilyNames` — the user's decision, and the reason
 * it is the right one for THIS tier is that a stable order is what a caller can rely on. See
 * `CTFontMatching_internal.h`.
 */
#include <CoreText/CTFontMatching_internal.h>

#include <CoreText/CTFont.h>
#include <CoreText/CTFontManager.h>

#include <CoreGraphics/CGFont_internal.h>

#include <CoreFoundation/CFArray.h>

#include <string.h>

/* =================================================================================================
 * ONE FACE, AS A DESCRIPTOR.
 * =============================================================================================== */
static CTFontDescriptorRef ct_match_descriptor_for_path(const char *path)
{
	CGDataProviderRef provider;
	CGFontRef face;
	CFMutableDictionaryRef attributes;
	CFTypeRef made;	/* a URL for one attribute, a STRING for the next: CFTypeRef, as the manager's own
				 * walk found the hard way — typed as a string, the URL branch WARNS. */
	CTFontDescriptorRef descriptor;
	char buffer[512];
	int written;

	provider = CGDataProviderCreateWithFilename(path);
	if (provider == NULL) {
		return NULL;
	}
	face = CGFontCreateWithDataProvider(provider);
	if (face == NULL) {
		return NULL;	/* a file the engine will not open is SKIPPED rather than reported: a listing door answers an
				 * array or NULL, and one unreadable file is not a reason to fail the other nine hundred. */
	}
	attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					       &kCFTypeDictionaryValueCallBacks);
	if (attributes == NULL) {
		CGFontRelease(face);
		return NULL;
	}
	made = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path, (CFIndex)strlen(path),
						       false);
	if (made != NULL) {
		CFDictionarySetValue(attributes, kCTFontURLAttribute, made);
		CFRelease(made);
	}
	made = CGFontCopyPostScriptName(face);
	if (made != NULL) {
		CFDictionarySetValue(attributes, kCTFontNameAttribute, made);
		CFRelease(made);
	}
	written = cg_font_name_entry(face, 1, buffer, sizeof buffer);	/* nameID 1 is the family */
	if (written > 0) {
		made = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)buffer, (CFIndex)written,
					       kCFStringEncodingUTF8, false);
		if (made != NULL) {
			CFDictionarySetValue(attributes, kCTFontFamilyNameAttribute, made);
			CFRelease(made);
		}
	}
	CGFontRelease(face);
	descriptor = CTFontDescriptorCreateWithAttributes(attributes);
	CFRelease(attributes);
	return descriptor;
}

/* =================================================================================================
 * THE WALK. One visit per face, with a DUPLICATE CHECK because a file reached through both a directory and a
 * registration IS one font — the same rule the manager's own listing doors apply, for the same reason.
 * =============================================================================================== */
static void ct_match_add(CFMutableArrayRef list, CTFontDescriptorRef descriptor)
{
	CFIndex i, count;

	if (descriptor == NULL) {
		return;
	}
	count = CFArrayGetCount(list);
	for (i = 0; i < count; i++) {
		if (CFEqual(CFArrayGetValueAtIndex(list, i), descriptor)) {
			return;
		}
	}
	CFArrayAppendValue(list, descriptor);
}

static void ct_match_visit(void *info, const char *path)
{
	CTFontDescriptorRef descriptor = ct_match_descriptor_for_path(path);

	if (descriptor != NULL) {
		ct_match_add((CFMutableArrayRef)info, descriptor);
		CFRelease(descriptor);
	}
}

/* THE ORDER, WHICH IS THIS TIER'S (see this file's head): by FAMILY NAME, with the comparator the manager already
 * offers. A descriptor with no family sorts AFTER one that has a family, because a face whose family cannot be read is
 * a face this tier knows less about. */
static CFComparisonResult ct_match_compare(const void *first, const void *second, void *context)
{
	CFStringRef family_a = (CFStringRef)CTFontDescriptorCopyAttribute((CTFontDescriptorRef)first,
									 kCTFontFamilyNameAttribute);
	CFStringRef family_b = (CFStringRef)CTFontDescriptorCopyAttribute((CTFontDescriptorRef)second,
									 kCTFontFamilyNameAttribute);
	CFComparisonResult result;

	(void)context;
	if (family_a == NULL && family_b == NULL) {
		result = kCFCompareEqualTo;
	} else if (family_a == NULL) {
		result = kCFCompareGreaterThan;
	} else if (family_b == NULL) {
		result = kCFCompareLessThan;
	} else {
		result = CTFontManagerCompareFontFamilyNames(family_a, family_b, NULL);
	}
	if (family_a != NULL) {
		CFRelease(family_a);
	}
	if (family_b != NULL) {
		CFRelease(family_b);
	}
	return result;
}

CFArrayRef CTFontMatchingCopyAvailableDescriptors(void)
{
	CFMutableArrayRef list;

	list = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (list == NULL) {
		return NULL;
	}
	cg_font_enumerate(ct_match_visit, list);
	if (CFArrayGetCount(list) > 1) {
		CFArraySortValues(list, CFRangeMake(0, CFArrayGetCount(list)), ct_match_compare, NULL);
	}
	return list;
}

/* =================================================================================================
 * THE TEST: DOES A QUERY DESCRIBE A CANDIDATE?
 * =============================================================================================== */
/* APPLE'S RULE AS FAR AS ITS HEADER PRINTS IT, AND THIS TIER'S READING WHERE IT DOES NOT: every attribute the query
 * NAMES must be present in the candidate with an EQUAL value, and a query with NO attributes matches everything. THE
 * SUBTLER HALF OF APPLE'S MATCHING — traits, family-name matching, substitution — is not attempted, and that is a
 * boundary rather than a gap: half a matcher that returned plausible wrong faces would be worse than one whose rule a
 * caller can read here. */
static bool ct_match_satisfies(CFDictionaryRef query, CFDictionaryRef candidate)
{
	CFIndex i, count;
	const void **keys;
	const void **values;

	if (candidate == NULL) {
		return false;
	}
	if (query == NULL || CFDictionaryGetCount(query) == 0) {
		return true;	/* a question with no conditions describes every face */
	}
	count = CFDictionaryGetCount(query);
	keys = (const void **)malloc((size_t)count * sizeof(void *));
	values = (const void **)malloc((size_t)count * sizeof(void *));
	if (keys == NULL || values == NULL) {
		free(keys);
		free(values);
		return false;
	}
	CFDictionaryGetKeysAndValues(query, keys, values);
	for (i = 0; i < count; i++) {
		CFTypeRef theirs = CFDictionaryGetValue(candidate, (CFStringRef)keys[i]);

		if (theirs == NULL || !CFEqual(theirs, (CFTypeRef)values[i])) {
			free(keys);
			free(values);
			return false;
		}
	}
	free(keys);
	free(values);
	return true;
}

/* A DESCRIPTOR'S ATTRIBUTES, read through the public door rather than by reaching into the object: this file is one
 * tier ABOVE the descriptor, and it has no business knowing how one is built. */
static CFDictionaryRef ct_match_query_attributes(CTFontDescriptorRef descriptor)
{
	/* THE PUBLIC DOOR FOR A DESCRIPTOR'S WHOLE DICTIONARY, asked rather than reached into: this file sits one tier
	 * ABOVE the descriptor and has no business knowing how one is built. THE REFERENCE IS RETAINED, so every caller
	 * below releases it. */
	return (CFDictionaryRef)CTFontDescriptorCopyAttributes(descriptor);
}

CFArrayRef CTFontMatchingCopyMatches(CFArrayRef queries)
{
	CFArrayRef available;
	CFMutableArrayRef result;
	CFIndex i, j, count;

	available = CTFontMatchingCopyAvailableDescriptors();
	if (available == NULL) {
		return NULL;
	}
	result = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (result == NULL) {
		CFRelease(available);
		return NULL;
	}
	count = CFArrayGetCount(available);
	for (i = 0; i < count; i++) {
		CFTypeRef candidate = CFArrayGetValueAtIndex(available, i);
		CFDictionaryRef candidate_attributes =
			(CFDictionaryRef)CTFontDescriptorCopyAttributes((CTFontDescriptorRef)candidate);
		CFIndex query_count = (queries == NULL) ? 0 : CFArrayGetCount(queries);
		bool matched = false;

		if (candidate_attributes != NULL) {
			if (query_count == 0) {
				matched = true;	/* NO QUERY AT ALL IS THAT QUESTION WITH NO CONDITIONS */
			}
			for (j = 0; !matched && j < query_count; j++) {
				CFDictionaryRef wanted = ct_match_query_attributes(
					(CTFontDescriptorRef)CFArrayGetValueAtIndex(queries, j));

				if (wanted != NULL) {
					matched = ct_match_satisfies(wanted, candidate_attributes);
					CFRelease(wanted);
				}
			}
			CFRelease(candidate_attributes);
		}
		if (matched) {
			ct_match_add(result, (CTFontDescriptorRef)candidate);
		}
	}
	CFRelease(available);
	return result;
}

/* =================================================================================================
 * THE TWO PUBLIC DOORS `CTFontDescriptor.h` HAS DECLARED SINCE THE DESCRIPTOR WAS WRITTEN, AND DEFINED HERE BECAUSE
 * THIS IS THE TIER THAT OWNS MATCHING. Both files that declare a matching door wait on this one — the descriptor's two
 * and the collection's two ask the SAME question — so the answer has one home and the doors are its front.
 *
 * APPLE'S OWN WORDS FOR THEM ARE "based on the original descriptor and SYSTEM PRECEDENCE", and the second half of that
 * is the order this file records as its own (see its head). AND ONE BOUNDARY IS RECORDED: the descriptors returned
 * carry the three attributes this tier can state as facts about a face — its URL, its PostScript name and its family —
 * where Apple's answer is "an array of NORMALIZED font descriptors", whose normalization a caller could compare
 * against. That normalization is not attempted, and the reason is the one the set's own rule already carries: half a
 * matcher that returned plausible wrong faces would be worse than one whose rule is written down.
 * =============================================================================================== */
/* THE SECOND PARAMETER IS A SET OF ATTRIBUTE NAMES AND NOT AN OPTIONS DICTIONARY — which is a correction, because the
 * first draft of this door took a `CFDictionaryRef` on the assumption that every Core Text door with a second parameter
 * takes options. APPLE'S HEADER SAYS `CFSetRef mandatoryAttributes`, and the door's own comment says what it is for:
 * every returned descriptor must CARRY those attributes. SO IT FILTERS THE RESULT rather than shaping the query, and
 * THE FILTER IS ANSWERED HONESTLY RATHER THAN APPROXIMATELY: this tier's descriptors carry the three facts it can state
 * about a face (URL, name, family), so a caller demanding an attribute this tier does not build gets AN EMPTY ARRAY —
 * the requirement cannot be met, and an empty answer says so. */
CFArrayRef CTFontDescriptorCreateMatchingFontDescriptors(CTFontDescriptorRef descriptor, CFSetRef mandatoryAttributes)
{
	CFMutableArrayRef queries, kept;
	CFArrayRef matches;
	CFIndex i, count;

	if (descriptor == NULL) {
		return NULL;
	}
	queries = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (queries == NULL) {
		return NULL;
	}
	CFArrayAppendValue(queries, descriptor);
	matches = CTFontMatchingCopyMatches(queries);
	CFRelease(queries);
	if (matches == NULL || mandatoryAttributes == NULL || CFSetGetCount(mandatoryAttributes) == 0) {
		return matches;
	}
	kept = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (kept == NULL) {
		CFRelease(matches);
		return NULL;
	}
	count = CFArrayGetCount(matches);
	for (i = 0; i < count; i++) {
		CTFontDescriptorRef candidate = (CTFontDescriptorRef)CFArrayGetValueAtIndex(matches, i);
		CFDictionaryRef attributes = ct_match_query_attributes(candidate);
		bool complete = (attributes != NULL);

		if (attributes != NULL) {
			CFIndex j, wanted = CFSetGetCount(mandatoryAttributes);
			const void **names = (const void **)malloc((size_t)wanted * sizeof(void *));

			if (names == NULL) {
				complete = false;
			} else {
				CFSetGetValues(mandatoryAttributes, names);
				for (j = 0; j < wanted; j++) {
					if (CFDictionaryGetValue(attributes, (CFStringRef)names[j]) == NULL) {
						complete = false;
						break;
					}
				}
				free(names);
			}
			CFRelease(attributes);
		}
		if (complete) {
			CFArrayAppendValue(kept, candidate);
		}
	}
	CFRelease(matches);
	return kept;
}

/* THE SINGULAR FORM IS THE FIRST OF THE SAME SET — a consequence of the ordering decision and not a second rule: a
 * caller who wants to know WHICH descriptor matched asks the plural door and reads its first, and this door is that
 * sentence written once. The result is RETAINED, because the door is a `Create`. */
CTFontDescriptorRef CTFontDescriptorCreateMatchingFontDescriptor(CTFontDescriptorRef descriptor, CFSetRef mandatoryAttributes)
{
	CFArrayRef matches = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, mandatoryAttributes);
	CTFontDescriptorRef first = NULL;

	if (matches != NULL && CFArrayGetCount(matches) > 0) {
		first = (CTFontDescriptorRef)CFRetain(CFArrayGetValueAtIndex(matches, 0));
		CFRelease(matches);
	} else if (matches != NULL) {
		CFRelease(matches);
	}
	return first;
}
