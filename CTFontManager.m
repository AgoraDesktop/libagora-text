/*
 * CTFontManager.m — the read doors, over the same enumeration the collection uses.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **ONE VISITOR PER QUESTION, AND EACH IS A PLAIN C FUNCTION** because `cg_font_enumerate` is a C door. The
 * name visitors differ in ONE line (which name they take from the face), which is the point: the list of faces
 * is the registry's, and these doors are readings of it rather than enumerations of their own. The URL door is
 * the same walk with a different accumulator (the path itself), and the two single-file doors (is-it-supported,
 * what-scope) never enumerate at all.
 *
 * AND THE DEDUPLICATION IS `-containsObject:` RATHER THAN A SET, because the list is short (a handful of
 * faces in this tree, a few hundred on a real Mac) and the order of the answer is part of it: a set would
 * answer a family name in whatever order it felt like, and this tier's answer is "the order the directories
 * are searched" (CTFontManager.h says so).
 */
#import <Foundation/Foundation.h>

#include "CTFontManager.h"
#include <CoreText/CTFont.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGFont.h>
#include <CoreGraphics/CGFont_internal.h>

#include <string.h>	/* strcmp, for the comparator's ordinal clause */
#include <unistd.h>	/* access, for the "file does not exist" code */
/* **AND LIBCONFIG, BECAUSE CORE TEXT NOW HAS A SETTINGS DOMAIN OF ITS OWN** (CT-M7j). CoreGraphics refuses
 * libconfig for the reason `cg_font_create_with_name` records: it has no domain of its own, so reading one
 * would be reading someone else's. This framework HAS one — `system.fontmanager` — which is the whole
 * difference. */
/* AND THE PREFERENCE STORE IS CORE FOUNDATION'S, WHICH IS BOTH WHAT APPLE DOES AND WHAT THIS LIBRARY'S
 * LAYERING REQUIRES (user, `dec-750b32baf39ba5cb`, corrected the same day). Core Text sits BELOW Foundation:
 * Apple's own Core Text depends on Core Foundation and Core Graphics and on nothing else, and it keeps its
 * font-manager state through `CFPreferences` -- which is what the doors below now call, and which is
 * plist-backed, so Apple's FORMAT is what lands on disk either way.
 *
 * THE INCLUDE THAT STOOD HERE WAS FIWIX'S, NOT THIS TREE'S: `<libconfig.h>`, "universal configuration access
 * for the FNX OS userland", four files and about 135 KB of Configuration/-directory machinery. It was the ONE
 * thing the first build of this import stopped on, out of sixteen implementation files. */
#include <CoreFoundation/CoreFoundation.h>

/* **THE MATCHER'S ENABLE/DISABLE SET LIVES WITH THE MATCHER** (CT-M7k) — see `CTFontDescriptor_internal.h`
 * for why the list is there rather than here. */
#include "CTFontDescriptor_internal.h"

/* WHAT EVERY NAME VISITOR NEEDS: the list it is filling in. */
struct ct_face_names {
	NSMutableArray *names;
};

static CTFontRef ct_face_at(const char *path)
{
	CGDataProviderRef provider = CGDataProviderCreateWithFilename(path);
	CGFontRef graphics;
	CTFontRef font;

	if (provider == NULL) {
		return NULL;
	}
	graphics = CGFontCreateWithDataProvider(provider);
	CGDataProviderRelease(provider);
	if (graphics == NULL) {
		return NULL;
	}
	font = CTFontCreateWithGraphicsFont(graphics, 12.0, NULL, NULL);
	CGFontRelease(graphics);
	return font;
}

static void ct_family_visitor(void *info, const char *path)
{
	struct ct_face_names *ctx = (struct ct_face_names *)info;
	CTFontRef font = ct_face_at(path);
	NSString *name;

	if (font == NULL) {
		return;
	}
	name = CTFontCopyFamilyName(font);
	CTFontRelease(font);
	if (name != nil && ![ctx->names containsObject:name]) {
		[ctx->names addObject:name];
	}
	[name release];
}

static void ct_postscript_visitor(void *info, const char *path)
{
	struct ct_face_names *ctx = (struct ct_face_names *)info;
	CTFontRef font = ct_face_at(path);
	NSString *name;

	if (font == NULL) {
		return;
	}
	name = CTFontCopyPostScriptName(font);
	CTFontRelease(font);
	if (name != nil) {
		[ctx->names addObject:name];	/* one per FACE, and a PostScript name is unique per face */
	}
	[name release];
}

/* **AND THE URL VISITOR, WHICH IS THE SAME WALK WITH THE PATH ITSELF AS THE ANSWER.** It does not open the
 * face at all: the door's contract is "an array of font URLs", and the enumeration has already decided which
 * paths are faces (it filters by suffix). Opening each file to re-ask a question the walk has answered would
 * make this door the only one that could disagree with its neighbours about what is available — which is the
 * two-answers bug the plan's §7 warns about, arriving from the inside. A path that cannot be made into a URL
 * is skipped rather than guessed at. */
static void ct_url_visitor(void *info, const char *path)
{
	NSMutableArray *urls = (NSMutableArray *)info;
	NSString *string = [NSString stringWithUTF8String:path];
	NSURL *url;

	if (string == nil) {
		return;
	}
	url = [NSURL fileURLWithPath:string];
	if (url != nil) {
		[urls addObject:url];
	}
}

NSArray *CTFontManagerCopyAvailableFontFamilyNames(void)
{
	struct ct_face_names ctx;

	ctx.names = [NSMutableArray array];
	cg_font_enumerate(ct_family_visitor, &ctx);
	return [(NSArray *)ctx.names retain];
}

NSArray *CTFontManagerCopyAvailablePostScriptNames(void)
{
	struct ct_face_names ctx;

	ctx.names = [NSMutableArray array];
	cg_font_enumerate(ct_postscript_visitor, &ctx);
	return [(NSArray *)ctx.names retain];
}

NSArray *CTFontManagerCopyAvailableFontURLs(void)
{
	NSMutableArray *urls = [NSMutableArray array];

	cg_font_enumerate(ct_url_visitor, urls);
	return [urls retain];
}

NSArray *CTFontManagerCreateFontDescriptorsFromURL(NSURL *url)
{
	NSMutableArray *list = [NSMutableArray array];
	NSString *path;
	CTFontRef font;

	if (url == nil) {
		return [(NSArray *)list retain];	/* an empty array, which is this door's answer for "nothing" */
	}
	path = [url path];
	if (path == nil) {
		return [(NSArray *)list retain];
	}
	font = ct_face_at([path UTF8String]);
	if (font != NULL) {
		CTFontDescriptorRef descriptor = CTFontCopyFontDescriptor(font);

		CTFontRelease(font);
		if (descriptor != NULL) {
			[list addObject:(id)descriptor];
			CTFontDescriptorRelease(descriptor);
		}
	}
	return [(NSArray *)list retain];
}

/* **IS THE FILE A FONT?** The question is about the file's DATA, so the answer is "did a face come back" —
 * the same `ct_face_at` the create door's loop uses, which is what makes this door and the registry agree by
 * construction rather than by coincidence. A URL that is not a file URL, or names nothing, is false. */
bool CTFontManagerIsSupportedFont(NSURL *fontURL)
{
	NSString *path;
	CTFontRef font;

	if (fontURL == nil) {
		return false;
	}
	path = [fontURL path];
	if (path == nil || [path length] == 0) {
		return false;
	}
	font = ct_face_at([path UTF8String]);
	if (font == NULL) {
		return false;
	}
	CTFontRelease(font);
	return true;
}

/* **THE SCOPE IS NO LONGER INDEPENDENT OF THE ARGUMENT, AND THIS CODE USED TO BE.** Until CT-M7h there was no
 * registration to report, so the door answered `kCTFontManagerScopeNone` for every URL and the comment above it
 * said so proudly. **THE ARGUMENT IT MADE IS STILL RIGHT AND IS NOW THE WHOLE TEST** — this door reports
 * REGISTRATION and not AVAILABILITY, so a face that merely lives in a font directory still answers `None` while
 * a path a caller REGISTERED answers `kCTFontManagerScopeProcess`, which is the era's own sentence for that
 * lifetime ("available to the current process for the duration of the process unless directly unregistered").
 * A door that answered by availability gets the first wrong; a door that answered a constant gets the second
 * wrong; the probe checks both. */

CTFontManagerScope CTFontManagerGetScopeForURL(NSURL *fontURL)
{
	NSString *path;
	const char *cpath;

	if (fontURL == nil) {
		return kCTFontManagerScopeNone;
	}
	path = [fontURL path];
	cpath = (path != nil) ? [path UTF8String] : NULL;
	/* ONLY THE PROCESS SCOPE EXISTS IN THIS TIER (the registering doors refuse the other two), so a registered
	 * path's scope IS the process's — which is why this is one comparison rather than a stored per-path scope
	 * that could drift from what the door of record actually did. */
	if (cpath != NULL && cg_font_is_registered(cpath)) {
		return kCTFontManagerScopeProcess;
	}
	return kCTFontManagerScopeNone;
}

/* ------------------------------------------------------------------------- */
/* The comparator: Apple's order, as far as anything published fixes it       */
/* ------------------------------------------------------------------------- */

/* IS THIS FAMILY NAME PREFIXED? The era's rule is that names "with recognized prefixes" sort after the
 * un-prefixed ones — and it does not say WHICH prefixes (see CTFontManager.h for why that half cannot be
 * implemented). What the rule's own examples fix is the shape: every hidden family Apple's system ships begins
 * with a character that is not a letter or a digit (".Helvetica Neue", ".SF NS"). So that is the test, and it
 * is the SHAPE rather than a guessed table. **A UTF-8 LEAD BYTE COUNTS AS A LETTER**: a family whose first
 * character is non-ASCII (an accented name, a CJK name) is a NAME and not a prefix, and testing the byte
 * naively would file every such family under the prefixed group. */
static int ct_family_is_prefixed(const char *name)
{
	unsigned char c;

	if (name == NULL || name[0] == '\0') {
		return 0;
	}
	c = (unsigned char)name[0];
	if (c >= 0x80) {
		return 0;
	}
	return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

NSInteger CTFontManagerCompareFontFamilyNames(const void *family1, const void *family2, void *context)
{
	const char *a, *b;
	int prefixed_a, prefixed_b;
	int order;

	(void)context;	/* Apple's own doc block says "unused, can be NULL" */

	if (family1 == family2) {
		return 0;
	}
	if (family1 == NULL) {
		return -1;
	}
	if (family2 == NULL) {
		return 1;
	}
	a = [(NSString *)family1 UTF8String];
	b = [(NSString *)family2 UTF8String];
	if (a == NULL) {
		return -1;
	}
	if (b == NULL) {
		return 1;
	}
	/* THE FIRST CLAUSE IS THE ONE THE ERA FIXES: un-prefixed before prefixed. */
	prefixed_a = ct_family_is_prefixed(a);
	prefixed_b = ct_family_is_prefixed(b);
	if (prefixed_a != prefixed_b) {
		return prefixed_a ? 1 : -1;
	}
	/* AND WITHIN A GROUP IT IS THE ORDINAL COMPARISON, because the sequence Apple sorts prefixes in is not
	 * published (the header says so). Byte order, which is what this tree's Foundation states for its own
	 * comparisons rather than a folding this door would have to invent. */
	order = strcmp(a, b);
	if (order < 0) {
		return -1;
	}
	return order > 0 ? 1 : 0;
}

/* **AND THE NOTIFICATION NAME.** Nothing posts it yet — adding a font to the registry is the write half's
 * act — but the NAME is what a subscriber registers with, and `NSNotificationCenter` is the mechanism this
 * tier has. The value is ours, as every unpublished string constant in this framework is. */
NSString *const kCTFontManagerRegisteredFontsChangedNotification = @"CTFontManagerRegisteredFontsChanged";

/* ====================================================================================================
 * M7h: the registering half — four doors over the CoreGraphics registry
 * ================================================================================================== */

/* **EVERY ERROR THIS FILE MAKES IS ONE OF THE ERA'S SIX CODES**, because that is what the era publishes (they
 * are Apple's own numbers, §46 corrected them) and because a code no caller can interpret is worse than no
 * error at all: the domain and the key shipped in §M7a with nothing producing them, and this is the unit that
 * produces them. Apple's sentence for the user-info key is "Each error will contain a CFArray of font URLs
 * corresponding to kCTFontManagerErrorFontURLsKey" — the URLs that FAILED — so the one-URL doors put the one
 * URL in and the array doors build the list as they go. */
static NSError *ct_manager_error(int code, NSURL *url)
{
	NSDictionary *info = nil;

	if (url != nil) {
		info = [NSDictionary dictionaryWithObject:[NSArray arrayWithObject:url]
						   forKey:kCTFontManagerErrorFontURLsKey];
	}
	return [NSError errorWithDomain:kCTFontManagerErrorDomain code:(NSInteger)code userInfo:info];
}

/* ONE URL, ONE ANSWER, AND THE SAME RULE FOR BOTH DIRECTIONS. `register_it` is the only thing the two doors do
 * differently, which is what keeps "already registered" and "not registered" from being two implementations of
 * one question. */
static BOOL ct_manager_set_registered(NSURL *url, CTFontManagerScope scope, BOOL register_it, NSError **error)
{
	NSString *path;
	const char *cpath;

	if (error != NULL) {
		*error = nil;
	}
	/* **THE SCOPE FIRST, BECAUSE IT IS THE ONE REFUSAL THAT IS ABOUT THE REQUEST RATHER THAN THE FILE** — and
	 * it sets no error, for the reason the header gives: the era's vocabulary has no code for it. */
	if (scope != kCTFontManagerScopeProcess) {
		return NO;
	}
	if (url == nil) {
		if (error != NULL) {
			*error = ct_manager_error(kCTFontManagerErrorFileNotFound, nil);
		}
		return NO;
	}
	path = [url path];
	cpath = (path != nil) ? [path UTF8String] : NULL;
	if (cpath == NULL) {
		if (error != NULL) {
			*error = ct_manager_error(kCTFontManagerErrorFileNotFound, url);
		}
		return NO;
	}
	/* APPLE'S OWN FIRST CODE, "the file does not exist at the specified URL", asked with `access` rather than
	 * by opening the face — because a file that exists and is not a font is the NEXT code, and one call should
	 * not answer both questions. */
	if (access(cpath, F_OK) != 0) {
		if (error != NULL) {
			*error = ct_manager_error(kCTFontManagerErrorFileNotFound, url);
		}
		return NO;
	}
	if (!CTFontManagerIsSupportedFont(url)) {
		if (error != NULL) {
			*error = ct_manager_error(kCTFontManagerErrorUnrecognizedFormat, url);
		}
		return NO;
	}
	if (register_it) {
		if (cg_font_is_registered(cpath)) {
			if (error != NULL) {
				*error = ct_manager_error(kCTFontManagerErrorAlreadyRegistered, url);
			}
			return NO;
		}
		if (cg_font_register(cpath) != 1) {
			return NO;	/* only an allocation failure, which the era has no code for */
		}
		return YES;
	}
	if (!cg_font_is_registered(cpath)) {
		if (error != NULL) {
			*error = ct_manager_error(kCTFontManagerErrorNotRegistered, url);
		}
		return NO;
	}
	cg_font_unregister(cpath);
	return YES;
}

bool CTFontManagerRegisterFontsForURL(NSURL *fontURL, CTFontManagerScope scope, NSError **error)
{
	return ct_manager_set_registered(fontURL, scope, YES, error) ? true : false;
}

bool CTFontManagerUnregisterFontsForURL(NSURL *fontURL, CTFontManagerScope scope, NSError **error)
{
	return ct_manager_set_registered(fontURL, scope, NO, error) ? true : false;
}

/* **AND THE TWO ARRAY DOORS, WHICH DIFFER FROM THE SINGLE ONES IN EXACTLY THE WAY APPLE SAYS**: "Returns true if
 * registration of all font URLs was successful. Otherwise false", with `errors` collecting one entry per
 * FAILURE rather than one for the call — so a caller learns WHICH of its files were refused and not merely that
 * something was. A NULL `errors` is legal (Apple's own "Can be NULL") and the work is still done. */
static bool ct_manager_set_registered_urls(NSArray *fontURLs, CTFontManagerScope scope, BOOL register_it,
					   NSArray **errors)
{
	NSMutableArray *failed = nil;
	NSUInteger i;
	bool all = true;

	if (errors != NULL) {
		*errors = nil;
	}
	if (fontURLs == nil) {
		return false;
	}
	if (errors != NULL) {
		failed = [NSMutableArray array];
	}
	for (i = 0; i < [fontURLs count]; i++) {
		NSError *one = nil;

		if (!ct_manager_set_registered([fontURLs objectAtIndex:i], scope, register_it,
					       (errors != NULL) ? &one : NULL)) {
			all = false;
			if (failed != nil && one != nil) {
				[failed addObject:one];
			}
		}
	}
	if (errors != NULL) {
		*errors = ([failed count] > 0) ? (NSArray *)failed : nil;
	}
	return all;
}

bool CTFontManagerRegisterFontsForURLs(NSArray *fontURLs, CTFontManagerScope scope, NSArray **errors)
{
	return ct_manager_set_registered_urls(fontURLs, scope, YES, errors);
}

bool CTFontManagerUnregisterFontsForURLs(NSArray *fontURLs, CTFontManagerScope scope, NSArray **errors)
{
	return ct_manager_set_registered_urls(fontURLs, scope, NO, errors);
}

/* ====================================================================================================
 * M7j: the auto-activation settings — one preferences domain
 * ================================================================================================== */

/* **THE GLOBAL KEY'S VALUE IS ITS OWN NAME**, which is this framework's convention for a key Apple publishes
 * the NAME of and no string for — the same value `kCTFontManagerErrorFontURLsKey` carries. It has no dot in it,
 * which matters here: this config model nests dotted keys, and the global setting is a flat one. */
NSString *const kCTFontManagerBundleIdentifier = @"kCTFontManagerBundleIdentifier";

/* **THE DOMAIN, AND WHY IT IS `system.fontmanager`**: the tree's config model reserves `system.<app>` for
 * FIRST-PARTY software and gives third parties reverse-DNS domains (`libconfig.h`, `config-design.md` §2) —
 * and `system.fonts` is already the X11 stack's fontconfig domain, so a font MANAGER setting cannot go there
 * without the two reading each other's files. */
#define CT_FONT_MANAGER_SETTINGS_DOMAIN "system.fontmanager"

/* **APPLE'S nil CASE, HONOURED RATHER THAN REFUSED**: "If NULL, the current application bundle will be used."
 * This tree has `+[NSBundle mainBundle]` and `-bundleIdentifier`, so the identifier is ASKED FOR rather than
 * invented — and a process that has none (a command-line tool) answers nil, which is the one case with nowhere
 * to put a setting. */
static NSString *ct_auto_activation_key(NSString *bundleIdentifier)
{
	if (bundleIdentifier != nil) {
		return bundleIdentifier;
	}
	return [[NSBundle mainBundle] bundleIdentifier];
}

/* WRITTEN AT THE USER SCOPE, READ THROUGH THE LIBRARY'S OWN PRECEDENCE. A font-manager preference is a USER's
 * preference (Apple's own "preferences location" is a per-user store), so the write goes to the user's
 * `Configuration/`; the read uses the ordinary resolution, which is what lets a system-wide default pinned by
 * an administrator be overridden by the user's own setting rather than the other way round. */
void CTFontManagerSetAutoActivationSetting(NSString *bundleIdentifier, CTFontManagerAutoActivationSetting setting)
{
	NSString *key = ct_auto_activation_key(bundleIdentifier);

	if (key == nil) {
		return;	/* no key to write under; see the header */
	}
	/* THE DOMAIN IS THE APPLICATION ID AND THE KEY IS THE PER-BUNDLE ONE, which is how Fiwix's model -- a file
	 * per domain, a key per application -- maps onto Core Foundation's, where the application id IS the
	 * domain. A WRITE GOES TO THE USER'S OWN STORE and the read below goes through Core Foundation's ordinary
	 * search, so an administrator's default is overridden by the user's setting rather than the other way
	 * round, which is the property the comment above this function has always claimed. */
	CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &setting);

	if (value != NULL) {
		CFPreferencesSetAppValue((CFStringRef)key, value, (CFStringRef)CT_FONT_MANAGER_SETTINGS_DOMAIN);
		CFPreferencesAppSynchronize((CFStringRef)CT_FONT_MANAGER_SETTINGS_DOMAIN);
		CFRelease(value);
	}
}

CTFontManagerAutoActivationSetting CTFontManagerGetAutoActivationSetting(NSString *bundleIdentifier)
{
	NSString *key = ct_auto_activation_key(bundleIdentifier);
	long value = 0;

	if (key == nil) {
		return kCTFontManagerAutoActivationDefault;
	}
	{
		CFNumberRef stored = (CFNumberRef)CFPreferencesCopyAppValue(
			(CFStringRef)key, (CFStringRef)CT_FONT_MANAGER_SETTINGS_DOMAIN);

		if (stored == NULL || CFGetTypeID(stored) != CFNumberGetTypeID() ||
		    !CFNumberGetValue(stored, kCFNumberLongType, &value)) {
			/* "Default is false" for the disable flag and "default" for this setting: a key that was never written
			 * is the era's own DEFAULT, not an error a caller has to interpret. A stored value that is somehow
			 * not a number is treated the same way rather than trusted. */
			if (stored != NULL) {
				CFRelease(stored);
			}
			return kCTFontManagerAutoActivationDefault;
		}
		CFRelease(stored);
	}
	return (CTFontManagerAutoActivationSetting)value;
}

/* **AND THE ONE DOOR LEFT IN THIS CLASS'S WRITE HALF THAT IS NEITHER A FILE NOR A PREFERENCE** (CT-M7k): it
 * changes what MATCHING answers and nothing else. The set it maintains is the matcher's — this door is a
 * spelling of it — so the body is one call, and the argument is validated where the set is kept. */
void CTFontManagerEnableFontDescriptors(NSArray *descriptors, bool enable)
{
	ctf_descriptor_set_enabled(descriptors, enable ? YES : NO);
}
