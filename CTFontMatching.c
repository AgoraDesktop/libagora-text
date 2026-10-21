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

#include <pthread.h>

#include <string.h>

/* =================================================================================================
 * ONE FACE, AS A DESCRIPTOR.
 * =============================================================================================== */
/* =================================================================================================
 * THE VISIBILITY STATE, WHICH IS THIS LIBRARY'S FIRST MUTABLE GLOBAL, AND ITS LOCK.
 *
 * CTFontManagerEnableFontDescriptors enables and disables fonts "based on the provided array of font descriptors", and a
 * DISABLED face is one the application may not use — so what it changes is PROCESS-GLOBAL AND MUTABLE, which nothing
 * else in this library is: every object here is immutable by construction, which is why the registration doors, the
 * collections and the lines all answer without a lock. APPLE'S HEADER PROMISES THAT ALL OF ITS FUNCTIONS "are thread
 * safe unless otherwise specified", SO THIS ONE IS LOCKED RATHER THAN CAVEATED — a statically initialised mutex guards
 * every read and every write, and THE LIST IS NEVER HANDED OUT (a caller gets a boolean), so the lock is held for a few
 * instructions and no caller can hold it at all.
 *
 * THE LIST HOLDS RETAINED DESCRIPTORS, AND THE QUESTION ASKED OF IT IS THE MATCHER'S OWN RULE RATHER THAN CFEqual —
 * WHICH IS A CORRECTION THE CASE FOUND RATHER THAN A CHOICE. The first draft compared the two descriptors with CFEqual,
 * and the case failed at once: the descriptors THIS TIER BUILDS for a face carry THREE attributes (its URL, its name and
 * its family) while the descriptor a CALLER hands the door carries only what the caller wrote down, so the two are
 * never equal and a disabled face stayed matchable. THE DISABLED LIST IS THEREFORE A LIST OF QUERIES, exactly as a font
 * collection is: a face is disabled when some descriptor IN THE LIST DESCRIBES IT — the same test
 * `ct_match_satisfies` makes for a matching query — so a caller disables a font by NAMING it and re-enables it by
 * naming it again.
 * =============================================================================================== */
static pthread_mutex_t ct_match_visibility_lock = PTHREAD_MUTEX_INITIALIZER;
/* **AND A GENERATION FOR THE VISIBILITY STATE TOO**, because the available-descriptor cache is keyed on the facts that
 * change its answer and VISIBILITY IS ONE OF THEM — a font that is disabled leaves the list, and a cache that could not
 * notice would keep offering a face the caller just removed. The font set's own generation (`cg_font_registry_generation`)
 * covers registrations and the search path; this one covers what only this file knows. MEASURED by the suite: without it
 * "a disabled face leaves every matching answer, and comes back" failed, because the cache answered from before the
 * disabliing. */
static unsigned long ct_match_visibility_generation = 1;
static CFMutableArrayRef ct_match_disabled = NULL;

static CFDictionaryRef ct_match_query_attributes(CTFontDescriptorRef descriptor);	/* THE TWO HELPERS THIS ONE NEEDS
										 * ARE DEFINED BELOW THE WALK THAT CALLS IT, so
										 * they are named here. */
static bool ct_match_satisfies(CFDictionaryRef query, CFDictionaryRef candidate);

static CFIndex ct_match_locked_match(CFDictionaryRef what)	/* THE LOCK MUST BE HELD BY THE CALLER. */
{
	CFIndex i, count;

	if (ct_match_disabled == NULL || what == NULL) {
		return -1;
	}
	count = CFArrayGetCount(ct_match_disabled);
	for (i = 0; i < count; i++) {
		CFDictionaryRef held = ct_match_query_attributes(
			(CTFontDescriptorRef)CFArrayGetValueAtIndex(ct_match_disabled, i));

		if (held != NULL) {
			bool describes = ct_match_satisfies(held, what);

			CFRelease(held);
			if (describes) {
				return i;
			}
		}
	}
	return -1;
}

static bool ct_match_is_disabled(CTFontDescriptorRef descriptor)
{
	CFDictionaryRef what;
	bool found;

	if (descriptor == NULL) {
		return false;
	}
	what = ct_match_query_attributes(descriptor);
	if (what == NULL) {
		return false;
	}
	pthread_mutex_lock(&ct_match_visibility_lock);
	found = (ct_match_locked_match(what) >= 0);
	pthread_mutex_unlock(&ct_match_visibility_lock);
	CFRelease(what);
	return found;
}

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
	/* **THE TYPOGRAPHIC FAMILY FIRST (nameID 16), THEN THE LEGACY ONE (nameID 1), AND THE ORDER IS WHAT MAKES A
	 * FAMILY ACTUALLY A FAMILY.** nameID 1 is allowed to carry the style — Inter's Medium face calls its family
	 * `Inter Medium`, and Libre Baskerville's Italic calls its family `Libre Baskerville Italic` — so a matcher
	 * built only on nameID 1 sees several one-child families instead of one family with several faces. MEASURED
	 * against the bundled set: `Inter` resolved to its Regular and Bold faces but not to Medium or SemiBold, and
	 * `CTFontCreateCopyWithSymbolicTraits` could not find the italic of a family whose nameID 1 spelled it out.
	 * nameID 16 exists exactly to answer this question; nameID 1 is the fallback for the faces that do not carry
	 * one, and a face with neither is a face whose family this tier still knows less about. */
	written = cg_font_name_entry(face, 16, buffer, sizeof buffer);	/* typographic family */
	if (written <= 0) {
		written = cg_font_name_entry(face, 1, buffer, sizeof buffer);	/* legacy family */
	}
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
	CFURLRef wanted;
	char wanted_path[1024];

	if (descriptor == NULL) {
		return;
	}
	/* **THE DUPLICATE CHECK IS BY FILE, AND THAT IS BOTH THE CORRECT QUESTION AND THE ONLY ONE THAT CAN BE ASKED
	 * HERE.** The comment above says what a duplicate IS — "a file reached through both a directory and a
	 * registration IS one font" — so the file is what to compare. It used to compare the whole DESCRIPTOR with
	 * `CFEqual`, which is a much broader claim, AND WHICH DOES NOT WORK IN THIS TREE: MEASURED, with Foundation
	 * loaded, `CFEqual` on two `CTFontDescriptor`s with DIFFERENT families answers TRUE, so 118 faces collapsed
	 * to ONE, the role matcher found no regular face, and `+[NSFont systemFontOfSize:]` answered nil.
	 *
	 * THE CAUSE IS ONE LEVEL DOWN AND IS NOT FIXED HERE (see the note in `libagora-core`'s `CFEqual` row): the
	 * dispatch to `-[__NSCFType isEqual:]` fires for a Core Foundation object that should be compared by Core
	 * Foundation, and that path then crashes in `objc_retain` under `-[NSDictionary isEqualToDictionary:]`. Asking
	 * about the FILE instead of the whole object is the smaller question, the one this walk actually has, and it
	 * does not go near that path.
	 *
	 * **AND THE QUADRATIC SCAN IS MEASURED RATHER THAN ASSUMED TO BE WORTH FIXING.** Each candidate is compared
	 * against every one already in the list, which over this host's 118 faces is about seven thousand comparisons.
	 * MEASURED: ONE FULL ENUMERATION OF THE REGISTRY COSTS 52 ms, and the duplicate check is a small part of it —
	 * the cost is opening and reading each font FILE, which no amount of list arithmetic removes. The AppKit
	 * caches a UI font by (type, size), so this is paid once per distinct face rather than once per frame. It is
	 * recorded here because a number nobody wrote down is a number somebody will guess at. */
	wanted = (CFURLRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontURLAttribute);
	if (wanted == NULL ||
	    !CFURLGetFileSystemRepresentation(wanted, false, (UInt8 *)wanted_path, (CFIndex)sizeof wanted_path)) {
		if (wanted != NULL) {
			CFRelease(wanted);
		}
		CFArrayAppendValue(list, descriptor);
		return;
	}
	CFRelease(wanted);
	count = CFArrayGetCount(list);
	for (i = 0; i < count; i++) {
		CFURLRef other = (CFURLRef)CTFontDescriptorCopyAttribute(
			(CTFontDescriptorRef)CFArrayGetValueAtIndex(list, i), kCTFontURLAttribute);
		char other_path[1024];
		bool same = false;

		if (other != NULL) {
			if (CFURLGetFileSystemRepresentation(other, false, (UInt8 *)other_path,
							     (CFIndex)sizeof other_path)) {
				same = (strcmp(other_path, wanted_path) == 0);
			}
			CFRelease(other);
		}
		if (same) {
			return;
		}
	}
	CFArrayAppendValue(list, descriptor);
}

static void ct_match_visit(void *info, const char *path)
{
	CTFontDescriptorRef descriptor = ct_match_descriptor_for_path(path);

	/* A DISABLED FACE IS NOT ONE THE APPLICATION MAY USE, so it is not one the application is told about: the ONE walk
	 * that answers every matching question SKIPS it here, rather than each door remembering to. */
	if (descriptor != NULL && !ct_match_is_disabled(descriptor)) {
		ct_match_add((CFMutableArrayRef)info, descriptor);
	}
	if (descriptor != NULL) {
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

/* **AND THE LIST IS BUILT ONCE PER FONT SET, NOT ONCE PER CALL, WHICH WAS THE TREE'S MOST EXPENSIVE DEFECT.** Building
 * it walks the font directories and OPENS EVERY FONT FILE to read its names; per-character font fallback calls this door
 * for every character no installed face covers, so the cost was a directory walk per character. MEASURED: a suite that
 * takes NO MEASURABLE TIME with a single-directory search path ran for MORE THAN 280 SECONDS without this cache.
 *
 * **AND THE CACHE CANNOT GO STALE, BECAUSE ITS KEY IS THE FONT SET'S OWN GENERATION** (`cg_font_registry_generation`,
 * which moves on a registration, an unregistration and an `FN_FONT_PATH` change — the three things that change which
 * faces exist). A cache that could not notice a font registered after it was built would answer for a machine that no
 * longer exists; the generation is what makes reuse sound rather than merely fast.
 *
 * THE CONTRACT IS STILL `Copy`: the caller gets a `+1` array and may keep or release it as it likes, so the cache's own
 * reference is separate from the caller's and the array handed out is immutable. */
static pthread_mutex_t ct_match_list_lock = PTHREAD_MUTEX_INITIALIZER;
static CFArrayRef ct_match_list_cache = NULL;
static unsigned long ct_match_list_generation = 0;
static unsigned long ct_match_list_visibility_generation = 0;

CFArrayRef CTFontMatchingCopyAvailableDescriptors(void)
{
	unsigned long generation = cg_font_registry_generation();
	CFArrayRef answer;

	pthread_mutex_lock(&ct_match_list_lock);
	/* **THE KEY IS THE FONT SET AND THE VISIBILITY STATE BOTH**, because the list is a function of the two: a face
	 * that is disabled leaves it, and the registry's generation knows nothing about that. Reading the visibility
	 * generation under THIS lock rather than the visibility one is deliberate — it is written under that lock by the
	 * door above, and a torn read of a `unsigned long` would at worst rebuild the cache once. */
	if (ct_match_list_cache == NULL
	    || generation != ct_match_list_generation
	    || ct_match_visibility_generation != ct_match_list_visibility_generation) {
		CFMutableArrayRef list = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

		if (list != NULL) {
			cg_font_enumerate(ct_match_visit, list);
			if (CFArrayGetCount(list) > 1) {
				CFArraySortValues(list, CFRangeMake(0, CFArrayGetCount(list)), ct_match_compare,
						  NULL);
			}
			if (ct_match_list_cache != NULL) {
				CFRelease(ct_match_list_cache);
			}
			ct_match_list_cache = list;	/* the cache's own reference */
			ct_match_list_generation = generation;
			ct_match_list_visibility_generation = ct_match_visibility_generation;
		}
	}
	answer = (ct_match_list_cache != NULL) ? (CFArrayRef)CFRetain(ct_match_list_cache) : NULL;
	pthread_mutex_unlock(&ct_match_list_lock);
	return answer;
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

/* =================================================================================================
 * THE VISIBILITY DOOR, DEFINED HERE BECAUSE THIS IS WHERE ITS STATE IS. It is a `CTFontManager` door in a file about
 * matching, which is the arrangement the four matching doors already have with the descriptor and the collection: THE
 * TIER THAT OWNS THE STATE OWNS THE DOORS THAT REACH IT. AND `enable` IS THE SENSE OF APPLE'S VERB: true ENABLES a
 * font, which REMOVES it from the disabled list; false DISABLES it, which adds it. The door returns VOID, as Apple's
 * signature does, so a descriptor that was never disabled and one that was are indistinguishable to the caller — the
 * state is readable only through the matching doors, which is what Apple's own sentence describes ("Disabled fonts are
 * not available for use by the application").
 * =============================================================================================== */
void CTFontManagerEnableFontDescriptors(CFArrayRef descriptors, bool enable)
{
	CFIndex i, count;

	if (descriptors == NULL) {
		return;
	}
	pthread_mutex_lock(&ct_match_visibility_lock);
	ct_match_visibility_generation++;
	if (ct_match_disabled == NULL) {
		ct_match_disabled = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	}
	if (ct_match_disabled != NULL) {
		count = CFArrayGetCount(descriptors);
		for (i = 0; i < count; i++) {
			CFTypeRef descriptor = CFArrayGetValueAtIndex(descriptors, i);
			CFDictionaryRef what = ct_match_query_attributes((CTFontDescriptorRef)descriptor);
			CFIndex found = ct_match_locked_match(what);

			/* ADD AND REMOVE ASK THE SAME QUESTION, so a caller who disables a font by naming it can re-enable it by
			 * naming it again — even though neither descriptor is the other, and neither is the one this tier builds
			 * for the face. */
			if (enable) {
				if (found >= 0) {
					CFArrayRemoveValueAtIndex(ct_match_disabled, found);
				}
			} else if (found < 0) {
				CFArrayAppendValue(ct_match_disabled, descriptor);
			}
			if (what != NULL) {
				CFRelease(what);
			}
		}
	}
	pthread_mutex_unlock(&ct_match_visibility_lock);
}
