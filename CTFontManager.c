/*
 * CTFontManager.c — the manager: what the system has, and what this process has made available to itself.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * THE READ-ONLY HALF IS THE ENGINE'S OWN LIST SEEN THREE WAYS, which is the same observation the seam records
 * one framework over: `cg_font_enumerate` walks the configured font directories AND the process's registrations,
 * so "what fonts exist" is ONE walk, and the PostScript names, the family names and the URLs are that walk
 * projected three different ways. Nothing here keeps a second list — a second list is how a registration comes
 * to be visible to one door and invisible to another.
 *
 * AND THE REGISTRATION HALF IS THE SEAM'S OWN REGISTRY, WHICH IS THE PROCESS'S. That is why this tier has ONE
 * scope and why the door for a scope it cannot honour refuses rather than substitutes: see CTFontManager.h's
 * head, where both boundaries are stated for a caller to read without coming down here.
 */
#include <CoreText/CTFontManager.h>

#include <CoreGraphics/CGFont_internal.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------------------------
 * THE TWO STRINGS, AND BOTH ARE INFERENCES RECORDED RATHER THAN VALUES READ. Apple publishes no value for
 * either, and there is no counterpart to cross to — the AppKit declares no `NSFontManager` counterpart for
 * them either. `kCTFontManagerBundleIdentifier` takes the bundle identifier the era's framework is known by
 * (it is the scope of the GLOBAL auto-activation setting, which is what Apple's sentence describes), and the
 * notification takes its own constant's name with the `k` dropped, as Apple's newer Core names do.
 * ----------------------------------------------------------------------------------------------- */
const CFStringRef kCTFontManagerBundleIdentifier = CFSTR("com.apple.CoreText");

const CFStringRef kCTFontManagerRegisteredFontsChangedNotification =
	CFSTR("CTFontManagerRegisteredFontsChangedNotification");

/* THE KEY THE AUTO-ACTIVATION SETTING IS STORED UNDER, inside the preferences domain Apple's own sentence names
 * ("the appropriate preferences location"). One key, because the domain carries the bundle identifier. */
static CFStringRef ct_manager_auto_activation_key(void)
{
	return CFSTR("CTFontManagerAutoActivationSetting");
}

/* =================================================================================================
 * THE LIST, AND THE THREE PROJECTIONS OF IT.
 * =============================================================================================== */
/* A URL'S PATH AS THE SEAM SPEAKS IT, or false when the URL is not a file URL at all — which is a refusal and
 * not an error to invent a code for. */
static bool ct_manager_path_from_url(CFURLRef url, char *buf, size_t size)
{
	return url != NULL && CFURLGetFileSystemRepresentation(url, true, (UInt8 *)buf, (CFIndex)size);
}

/* ONLY IF IT IS NOT THERE ALREADY. Apple's word for the PostScript list is "unique", and for the families it
 * is the same thing seen differently — a family with twelve faces is ONE family name. The URLs are not promised
 * unique, but a file reachable through both a directory and a registration IS one font, so the same rule is
 * applied for the same reason: a duplicate would be a caller enumerating one font twice. */
static void ct_manager_add_unique(CFMutableArrayRef list, CFTypeRef value)
{
	CFIndex i, count;

	if (value == NULL) {
		return;
	}
	count = CFArrayGetCount(list);
	for (i = 0; i < count; i++) {
		if (CFEqual(CFArrayGetValueAtIndex(list, i), value)) {
			return;
		}
	}
	CFArrayAppendValue(list, value);
}

/* ONE FACE FROM ITS FILE, OR NULL. THE CALLER OWNS IT. The engine behind CGFont opens the file's FIRST face,
 * which is the only reachable one here and is recorded as such in the URL door below. */
static CGFontRef ct_manager_face_at(const char *path)
{
	CGDataProviderRef provider;
	CGFontRef face;

	provider = CGDataProviderCreateWithFilename(path);
	if (provider == NULL) {
		return NULL;
	}
	face = CGFontCreateWithDataProvider(provider);
	/* THE PROVIDER IS NOT RELEASED, AND THAT IS THE FILE'S ONE DELIBERATE LEAK: the engine's face may still
	 * read the bytes it was handed, and this tier has no way to ask. The same choice the suite records. */
	return face;
}

/* THE `name` TABLE'S ENTRY 1 -- the family -- THROUGH THE SEAM, because Core Graphics has no door for it. */
static CFStringRef ct_manager_family_of(CGFontRef face)
{
	char buf[512];
	int n;

	if (face == NULL) {
		return NULL;
	}
	n = cg_font_name_entry(face, 1, buf, sizeof buf);
	if (n <= 0) {
		return NULL;
	}
	return CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)buf, (CFIndex)n, kCFStringEncodingUTF8,
				       false);
}

enum ct_manager_want { CT_MANAGER_WANT_PSNAME, CT_MANAGER_WANT_FAMILY, CT_MANAGER_WANT_URL };

/* THE ONE WALK. The visitor opens each face the engine can open and adds the ONE thing this projection wants;
 * a face that will not open is SKIPPED rather than reported, because a listing door has no way to report it —
 * Apple's three `Copy` doors answer an array or NULL, and one unreadable file in a directory is not a reason to
 * fail the other nine hundred. */
static void ct_manager_visit(void *info, const char *path)
{
	struct { CFMutableArrayRef list; enum ct_manager_want want; } *ctx = info;
	/* CFTypeRef AND NOT CFStringRef, because this ONE variable carries whichever of the three projections the
	 * walk was asked for — a string of names, or a URL. Typing it as a string warned on the URL branch, and the
	 * warning was right: it said the two projections are not the same type, which is exactly what this variable
	 * is here to hold. */
	CFTypeRef made = NULL;
	CGFontRef face;

	if (ctx->want == CT_MANAGER_WANT_URL) {
		made = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path,
							       (CFIndex)strlen(path), false);
	} else {
		face = ct_manager_face_at(path);
		if (face == NULL) {
			return;
		}
		if (ctx->want == CT_MANAGER_WANT_PSNAME) {
			made = CGFontCopyPostScriptName(face);
		} else {
			made = ct_manager_family_of(face);
		}
		CGFontRelease(face);
	}
	ct_manager_add_unique(ctx->list, made);
	if (made != NULL) {
		CFRelease(made);
	}
}

static CFArrayRef ct_manager_copy_wanted(enum ct_manager_want want)
{
	struct { CFMutableArrayRef list; enum ct_manager_want want; } ctx;
	CFMutableArrayRef list;

	list = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (list == NULL) {
		return NULL;
	}
	ctx.list = list;
	ctx.want = want;
	cg_font_enumerate(ct_manager_visit, &ctx);
	return list;
}

CFArrayRef CTFontManagerCopyAvailablePostScriptNames(void)
{
	return ct_manager_copy_wanted(CT_MANAGER_WANT_PSNAME);
}

/* THE FAMILIES ARRIVE IN THE ENGINE'S ENUMERATION ORDER AND NOT IN APPLE'S UI ORDER, which is the deviation
 * CTFontManager.h's head records in full: the order turns on a list of recognized foundry prefixes that Apple
 * does not publish. A CALLER THAT WANTS APPLE'S ORDER SORTS THE RESULT WITH THE COMPARATOR BELOW, which orders
 * these very names the same way this door returns them. */
CFArrayRef CTFontManagerCopyAvailableFontFamilyNames(void)
{
	return ct_manager_copy_wanted(CT_MANAGER_WANT_FAMILY);
}

CFArrayRef CTFontManagerCopyAvailableFontURLs(void)
{
	return ct_manager_copy_wanted(CT_MANAGER_WANT_URL);
}

/* ---------------------------------------------------------------------------------------------
 * THE COMPARATOR, AND THE DEVIATION IS THE WHOLE OF WHAT IS WORTH SAYING ABOUT IT. Apple documents the order as
 * "the Apple preferred order, accounting for foundry prefix. Family names with recognized prefixes are sorted
 * after the un-prefixed names in prefix order" — and the rule turns on WHICH PREFIXES ARE RECOGNIZED, a list
 * that appears in no header of the era. SO THIS IS AN ORDINARY COMPARISON, and the difference is visible: with
 * Apple's rule `"Garamond"` sorts before `"Adobe Garamond"`, and here `"Adobe Garamond"` sorts first, because
 * `'A' < 'G'`.
 *
 * WHAT IS NOT DEVIATED FROM IS THE CONTRACT OF A CFComparatorFunction: the result is one of the three
 * comparison constants, and it is CONSISTENT — equal strings compare equal, and the relation is antisymmetric —
 * which is what `CFArraySortValues` and `qsort` require to terminate and to produce a stable answer. The flags
 * are the ones a UI sort wants from Core Foundation: localized, case-insensitive, and numeric-aware so that
 * `"Foo2"` precedes `"Foo7"` precedes `"Foo25"` rather than following it.
 * ----------------------------------------------------------------------------------------------- */
CFComparisonResult CTFontManagerCompareFontFamilyNames(const void *family1, const void *family2, void *context)
{
	const CFStringCompareFlags flags = kCFCompareLocalized | kCFCompareCaseInsensitive | kCFCompareNumerically;

	(void)context;	/* Apple's own note for the parameter is "Unused. Can be NULL." */
	if (family1 == NULL || family2 == NULL || CFGetTypeID((CFTypeRef)family1) != CFStringGetTypeID() ||
	    CFGetTypeID((CFTypeRef)family2) != CFStringGetTypeID()) {
		return kCFCompareEqualTo;
	}
	return CFStringCompare((CFStringRef)family1, (CFStringRef)family2, flags);
}

/* =================================================================================================
 * ONE FILE, AS DESCRIPTORS.
 * =============================================================================================== */
/* THE ATTRIBUTES ARE THE ONES THIS TIER CAN STATE AS FACTS rather than as guesses: the URL it was asked about,
 * the PostScript name and the family, both read from the face's own tables. NOTHING INVENTED IS AMONG THEM — a
 * descriptor's other twenty attributes are not derivable from a file by this tier, and an attribute that is
 * absent is absent rather than approximated. AND NOTHING IS REGISTERED, which is Apple's own note for this
 * door: these descriptors "are not available through font descriptor matching".
 *
 * ONE FILE, ONE FACE, AND THAT IS A BOUNDARY: `CGFontCreateWithDataProvider` reaches the face the engine opens,
 * so a file holding several yields the one face this tier can see. Apple's sentence is plural ("each of the
 * fonts in the specified URL") and this door answers the singular of it, recorded rather than papered over. */
CFArrayRef CTFontManagerCreateFontDescriptorsFromURL(CFURLRef fileURL)
{
	char path[1024];
	CGFontRef face;
	CFStringRef psname, family;
	CFMutableDictionaryRef attributes;
	CTFontDescriptorRef descriptor;
	CFArrayRef answer;

	if (!ct_manager_path_from_url(fileURL, path, sizeof path)) {
		return NULL;
	}
	face = ct_manager_face_at(path);
	if (face == NULL) {
		return NULL;
	}
	attributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					       &kCFTypeDictionaryValueCallBacks);
	if (attributes == NULL) {
		CGFontRelease(face);
		return NULL;
	}
	CFDictionarySetValue(attributes, kCTFontURLAttribute, fileURL);
	psname = CGFontCopyPostScriptName(face);
	if (psname != NULL) {
		CFDictionarySetValue(attributes, kCTFontNameAttribute, psname);
		CFRelease(psname);
	}
	family = ct_manager_family_of(face);
	if (family != NULL) {
		CFDictionarySetValue(attributes, kCTFontFamilyNameAttribute, family);
		CFRelease(family);
	}
	CGFontRelease(face);
	descriptor = CTFontDescriptorCreateWithAttributes(attributes);
	CFRelease(attributes);
	if (descriptor == NULL) {
		return NULL;
	}
	answer = CFArrayCreate(kCFAllocatorDefault, (const void **)&descriptor, 1, &kCFTypeArrayCallBacks);
	CFRelease(descriptor);
	return answer;
}

/* THE SAME QUESTION REGISTRATION REFUSES ON, asked without changing anything: can the engine open this file. */
bool CTFontManagerIsSupportedFont(CFURLRef fontURL)
{
	char path[1024];
	CGFontRef face;

	if (!ct_manager_path_from_url(fontURL, path, sizeof path)) {
		return false;
	}
	face = ct_manager_face_at(path);
	if (face == NULL) {
		return false;
	}
	CGFontRelease(face);
	return true;
}

/* =================================================================================================
 * REGISTRATION. One scope, and the refusal for the others carries no code — see the head of both this file and
 * CTFontManager.h. The codes below are Apple's own, and the ONE OF ITS EIGHT THIS TIER CANNOT PRODUCE is
 * `kCTFontManagerErrorInvalidFontData`: telling "not a font file" from "a font file whose data is invalid"
 * needs a parse the engine's refusal does not expose, so a file the engine will not open answers
 * `kCTFontManagerErrorUnrecognizedFormat` and the distinction is recorded as unreachable rather than guessed.
 * =============================================================================================== */
static CFErrorRef ct_manager_error(CTFontManagerError code, CFURLRef url)
{
	CFStringRef keys[1];
	CFTypeRef values[1];
	CFArrayRef urls;
	CFDictionaryRef info;
	CFErrorRef error;

	/* APPLE'S SHAPE FOR AN ERROR ABOUT A FILE: one CFError whose user-info dictionary carries the array of the
	 * URLs that failed under kCTFontManagerErrorFontURLsKey. One here because this helper is called per URL; the
	 * plural doors below build the same thing per failure, which is why the key's value is an ARRAY. */
	urls = CFArrayCreate(kCFAllocatorDefault, (const void **)&url, 1, &kCFTypeArrayCallBacks);
	if (urls == NULL) {
		return NULL;
	}
	keys[0] = kCTFontManagerErrorFontURLsKey;
	values[0] = urls;
	info = CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)values, 1,
				  &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFRelease(urls);
	if (info == NULL) {
		return NULL;
	}
	error = CFErrorCreate(kCFAllocatorDefault, kCTFontManagerErrorDomain, (CFIndex)code, info);
	CFRelease(info);
	return error;
}

/* THE WORK ONE URL, answering true when it is REGISTERED (or was) and filling `why` when it is not. The scope
 * has already been checked by the callers, so nothing here has to know about scopes at all. */
static bool ct_manager_set_registered(CFURLRef url, bool want_registered, CFErrorRef *why)
{
	char path[1024];

	*why = NULL;
	if (!ct_manager_path_from_url(url, path, sizeof path)) {
		/* NOT A FILE URL AT ALL. Apple's codes are about a file, and there is no file to complain about, so
		 * this is the same "unrecognized" answer a file the engine will not open gets. */
		*why = ct_manager_error(kCTFontManagerErrorUnrecognizedFormat, url);
		return false;
	}
	if (want_registered) {
		if (access(path, F_OK) != 0) {
			*why = ct_manager_error(kCTFontManagerErrorFileNotFound, url);
			return false;
		}
		if (access(path, R_OK) != 0) {
			*why = ct_manager_error(kCTFontManagerErrorInsufficientPermissions, url);
			return false;
		}
		if (cg_font_is_registered(path)) {
			*why = ct_manager_error(kCTFontManagerErrorAlreadyRegistered, url);
			return false;
		}
		if (ct_manager_face_at(path) == NULL) {
			*why = ct_manager_error(kCTFontManagerErrorUnrecognizedFormat, url);
			return false;
		}
		if (!cg_font_register(path) && !cg_font_is_registered(path)) {
			*why = ct_manager_error(kCTFontManagerErrorUnrecognizedFormat, url);
			return false;
		}
		return true;
	}
	if (!cg_font_is_registered(path)) {
		*why = ct_manager_error(kCTFontManagerErrorNotRegistered, url);
		return false;
	}
	cg_font_unregister(path);
	return true;
}

/* THE SCOPE CHECK, WHICH IS THE ONE PLACE THIS TIER'S BOUNDARY IS ENFORCED. Returning false with the error left
 * UNSET is the refusal CTFontManager.h's head describes: set means the file, unset means the scope, and Apple's
 * era has no code for the second. */
static bool ct_manager_scope_supported(CTFontManagerScope scope)
{
	return scope == kCTFontManagerScopeProcess;
}

static bool ct_manager_one_url(CFURLRef url, CTFontManagerScope scope, bool want_registered,
			       CFErrorRef *error)
{
	CFErrorRef why = NULL;
	bool ok;

	if (error != NULL) {
		/* THE OUT-PARAMETER IS CLEARED FIRST, which is what makes "unset means the scope" readable at all. */
		*error = NULL;
	}
	if (!ct_manager_scope_supported(scope)) {
		return false;
	}
	ok = ct_manager_set_registered(url, want_registered, &why);
	if (!ok && error != NULL) {
		*error = why;
	} else if (why != NULL) {
		CFRelease(why);
	}
	return ok;
}

/* TRUE WHEN EVERY URL WAS HANDLED, which is Apple's own sentence for the plural result. `errors` receives a
 * RETAINED array of the failures — one CFError per URL, each naming its URL under the key — or is left NULL
 * when there were none. */
static bool ct_manager_many_urls(CFArrayRef fontURLs, CTFontManagerScope scope, bool want_registered,
				 CFArrayRef *errors)
{
	CFMutableArrayRef failures = NULL;
	bool all = true;
	CFIndex i, count;

	if (errors != NULL) {
		*errors = NULL;
	}
	if (fontURLs == NULL) {
		return false;
	}
	if (!ct_manager_scope_supported(scope)) {
		/* THE SCOPE REFUSAL, and it is ONE refusal for the whole call: no URL was looked at, so there is no
		 * per-URL error to report and the array stays unset. */
		return false;
	}
	count = CFArrayGetCount(fontURLs);
	if (count > 0) {
		failures = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
		if (failures == NULL) {
			return false;
		}
	}
	for (i = 0; i < count; i++) {
		CFErrorRef why = NULL;
		CFTypeRef value = CFArrayGetValueAtIndex(fontURLs, i);

		if (CFGetTypeID(value) != CFURLGetTypeID() ||
		    !ct_manager_set_registered((CFURLRef)value, want_registered, &why)) {
			all = false;
			if (why == NULL) {
				why = ct_manager_error(kCTFontManagerErrorUnrecognizedFormat, (CFURLRef)value);
			}
			if (why != NULL) {
				CFArrayAppendValue(failures, why);
				CFRelease(why);
			}
		}
	}
	if (errors != NULL && failures != NULL && CFArrayGetCount(failures) > 0) {
		*errors = failures;
	} else if (failures != NULL) {
		CFRelease(failures);
	}
	return all;
}

bool CTFontManagerRegisterFontsForURL(CFURLRef fontURL, CTFontManagerScope scope, CFErrorRef *error)
{
	return ct_manager_one_url(fontURL, scope, true, error);
}

bool CTFontManagerUnregisterFontsForURL(CFURLRef fontURL, CTFontManagerScope scope, CFErrorRef *error)
{
	return ct_manager_one_url(fontURL, scope, false, error);
}

bool CTFontManagerRegisterFontsForURLs(CFArrayRef fontURLs, CTFontManagerScope scope, CFArrayRef *errors)
{
	return ct_manager_many_urls(fontURLs, scope, true, errors);
}

bool CTFontManagerUnregisterFontsForURLs(CFArrayRef fontURLs, CTFontManagerScope scope, CFArrayRef *errors)
{
	return ct_manager_many_urls(fontURLs, scope, false, errors);
}

CTFontManagerScope CTFontManagerGetScopeForURL(CFURLRef fontURL)
{
	char path[1024];

	if (!ct_manager_path_from_url(fontURL, path, sizeof path)) {
		return kCTFontManagerScopeNone;
	}
	/* ONE SCOPE EXISTS, so this door has two answers and Apple's own sentence names both of them: the scope, or
	 * kCTFontManagerScopeNone "if not currently registered". */
	return cg_font_is_registered(path) ? kCTFontManagerScopeProcess : kCTFontManagerScopeNone;
}

/* =================================================================================================
 * THE AUTO-ACTIVATION SETTING, WHICH IS A PREFERENCE AND IS STORED AS ONE.
 * =============================================================================================== */
/* WHAT DOMAIN A CALL IS ABOUT. Apple's argument selects the bundle: NULL means the current application and
 * `kCTFontManagerBundleIdentifier` the global setting. THIS TIER HAS NO BUNDLES, so both collapse onto the
 * global identifier — recorded rather than faked with an invented "current application" — which means a caller
 * that passes NULL and one that passes the constant get the same answer here. */
static CFStringRef ct_manager_settings_domain(CFStringRef bundleIdentifier)
{
	return (bundleIdentifier != NULL) ? bundleIdentifier : kCTFontManagerBundleIdentifier;
}

void CTFontManagerSetAutoActivationSetting(CFStringRef bundleIdentifier,
					   CTFontManagerAutoActivationSetting setting)
{
	CFStringRef domain = ct_manager_settings_domain(bundleIdentifier);
	CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &setting);

	if (value == NULL) {
		return;
	}
	/* APPLE'S SENTENCE IS "apply the setting to the appropriate preferences location", AND THIS IS IT: the key
	 * lives in the domain the bundle identifier names. */
	CFPreferencesSetAppValue(ct_manager_auto_activation_key(), value, domain);
	CFPreferencesAppSynchronize(domain);
	CFRelease(value);
}

CTFontManagerAutoActivationSetting CTFontManagerGetAutoActivationSetting(CFStringRef bundleIdentifier)
{
	CFStringRef domain = ct_manager_settings_domain(bundleIdentifier);
	Boolean found = false;
	CFIndex stored = CFPreferencesGetAppIntegerValue(ct_manager_auto_activation_key(), domain, &found);

	/* NOTHING RECORDED IS NOT AN ERROR: Apple's own vocabulary for that case is the DEFAULT, which is the value
	 * that means "use the global setting". */
	return found ? (CTFontManagerAutoActivationSetting)stored : kCTFontManagerAutoActivationDefault;
}

/* =================================================================================================
 * OWED DOORS, EACH WITH THE REASON IT CANNOT BE ANSWERED HERE. They are DECLARED in CTFontManager.h and NOT
 * DEFINED, so a caller who reaches for one gets a link error rather than an invented answer.
 *
 *   ONLY ONE DOOR IS OWED HERE NOW, AND THE ONE THAT LEFT IS WORTH A SENTENCE BECAUSE ITS REASON WAS A CONDITION THAT
 *   CAME TRUE. `CTFontManagerEnableFontDescriptors` was owed because "matching is the thing this tier does not have ...
 *   It lands with the tier that owns matching, beside them" — THE TIER LANDED, so the door did, AND IT IS DEFINED IN
 *   `CTFontMatching.c` RATHER THAN HERE, because that is where its state lives: the disabled set is process-global
 *   mutable state, it is the FIRST such state in this library, and it is guarded by a LOCK rather than caveated because
 *   APPLE'S OWN HEADER PROMISES THAT ALL OF ITS FUNCTIONS "ARE THREAD SAFE UNLESS OTHERWISE SPECIFIED". See
 *   `CTFontMatching.c`'s visibility section, and the `owed` test, which lost this name in the same commit.
 *
 *   CTFontManagerCreateFontRequestRunLoopSource   THE CONVERSATION NOW EXISTS AND ITS TRANSPORT DOES NOT, which is a
 *                                                 much narrower reason than the one this entry used to carry. THE
 *                                                 WIRE IS IMPLEMENTED: `CTFontService.c` carries both halves — a
 *                                                 requester's encode and a responder's answer, produced by calling the
 *                                                 matching tier — and its case passes. WHAT IS MISSING IS THE PORT,
 *                                                 THE NAME AND THE PROCESS (FONT-SERVICE.md), plus one build fact:
 *                                                 THE DOOR TAKES A BLOCK, its declaration is guarded by `__BLOCKS__`
 *                                                 exactly as Apple's is, and this target does not presently compile
 *                                                 with `-fblocks`. Two checkable things, neither of them a tier.
 *
 * AND ONE THING THAT IS *NOT* OWED BUT IS WORTH SAYING HERE, because a caller will otherwise ask: THE SCOPE.
 * `kCTFontManagerScopeUser` and `kCTFontManagerScopeSession` are not owed doors — they are VALUES of the scope
 * parameter, and the registration doors above REFUSE them rather than defining them away. See this file's head
 * and CTFontManager.h's for why, and what the refusal looks like.
 * =============================================================================================== */
