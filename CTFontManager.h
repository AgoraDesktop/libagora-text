/*
 * CTFontManager.h — the font manager: the system-facing half of Core Text, which answers two questions a font
 * or a collection cannot. WHICH FACES THIS SYSTEM HAS, and MAKE THIS ONE AVAILABLE TO ME.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). It includes CTFontDescriptor.h and CTFontManagerErrors.h
 * and nothing else of the framework's — a manager speaks in DESCRIPTORS and reports in ERRORS, which is why
 * those are the two headers above it.
 *
 * A FONT, A COLLECTION AND A MANAGER ARE THREE DIFFERENT THINGS AND THE SPLIT IS APPLE'S. A font is a face at a
 * size; a collection is a QUESTION asked of the installed faces; and a manager is the INSTALLATION ITSELF — the
 * list of what is there, and the act of putting something else there. So the manager's doors divide in two: the
 * read-only ones (three `Copy` doors, `CreateFontDescriptorsFromURL`, `IsSupportedFont`, `GetScopeForURL`) that
 * report what the system has, and the registration ones that change it.
 *
 * ---------------------------------------------------------------------------------------
 * TWO BOUNDARIES THE IMPLEMENTATION RECORDS RATHER THAN PAPERING OVER. Both are Apple's own words read closely,
 * and both are stated HERE because a caller has to be able to see them without reading the `.c`.
 *
 * ONE: THERE IS ONE SCOPE, AND IT IS THE PROCESS'S. Apple's `CTFontManagerScope` names four, and the two that
 * are not the process's promise things this tier has no machinery for: `kCTFontManagerScopeUser` is "available
 * to ALL processes for the current user session and … in subsequent sessions", and `kCTFontManagerScopeSession`
 * covers the session. Both need a PERSISTENT, CROSS-PROCESS font store — a thing this tier does not have and
 * does not pretend to have, because its registry is the process's own (`cg_font_register`). SO A REQUEST FOR
 * ANY OTHER SCOPE IS REFUSED, and refused the way this tree refuses things: `false`, with the error out-parameter
 * LEFT UNSET, because Apple's era has NO error code meaning "unsupported scope" — the eight in
 * CTFontManagerErrors.h are all about a FILE (not found, no permission, not a font, invalid, already there, not
 * there, in use, required) — and INVENTING ONE IS THE ONE THING THIS LIBRARY WILL NOT DO. A caller that needs to
 * tell "the scope I asked for cannot be honoured here" from "the file was bad" reads the error: set means the
 * file, unset means the scope.
 *
 * TWO: THE FAMILY-NAME ORDER IS A RECORDED DEVIATION, AND IT IS VISIBLE IN TWO DOORS AT ONCE.
 * `CTFontManagerCopyAvailableFontFamilyNames` is documented as "sorted for UI display", and
 * `CTFontManagerCompareFontFamilyNames` is that order written down: "the Apple preferred order, accounting for
 * foundry prefix. Family names with recognized prefixes are sorted after the un-prefixed names in prefix order."
 * THE RULE TURNS ON A LIST APPLE DOES NOT PUBLISH — which prefixes are "recognized" — so the order cannot be
 * reproduced from the era's headers at all. THE USER'S DECISION (2026-10-11) IS TO ANSWER BOTH DOORS ANYWAY AND
 * RECORD THE DEVIATION, rather than to leave them owed: both are answered with an ordinary localized,
 * case-insensitive, numeric-aware string comparison, so `"Adobe Garamond"` sorts BEFORE `"Garamond"` here where
 * Apple's rule would put the un-prefixed name first. One consequence is worth stating plainly: the two doors
 * AGREE with each other — `CopyAvailableFontFamilyNames` returns the families in the same order this comparator
 * gives them — which is the property a caller actually relies on when it re-sorts the array.
 * ------------------------------------------------------------------------------------- */
#ifndef CORE_TEXT_CTFONTMANAGER_H
#define CORE_TEXT_CTFONTMANAGER_H

#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFontManagerErrors.h>

#include <CoreFoundation/CoreFoundation.h>

#include <sys/types.h>		/* pid_t, for the run loop source Apple declares under __BLOCKS__ */

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------
 * WHAT THE SYSTEM HAS. Three `Copy` doors, and the name says what the caller owes: a RETAINED reference to
 * release. All three are `NULL` on error.
 * ------------------------------------------------------------------------------------- */
/* THE UNIQUE POSTSCRIPT NAMES, which Apple's word "unique" is the whole of the contract for: a family with
 * twelve faces contributes ONE name here only if the twelve faces share it, which is why this is a set-like
 * answer and the family door below is not. NO ORDER IS SPECIFIED, and none is invented. */
CFArrayRef CTFontManagerCopyAvailablePostScriptNames(void);

/* THE VISIBLE FAMILY NAMES, "sorted for UI display" — WHICH THIS TIER CANNOT DO, see this file's head: they
 * arrive in the engine's enumeration order, and the comparator below orders them the same way. */
CFArrayRef CTFontManagerCopyAvailableFontFamilyNames(void);

/* THE URLS, one per face this system has — the configured font directories AND the process's registrations,
 * because `cg_font_enumerate` walks both. */
CFArrayRef CTFontManagerCopyAvailableFontURLs(void);

/* APPLE'S UI ORDER FOR FAMILY NAMES, WRITTEN AS A COMPARATOR so a caller can re-sort an array with it. IT IS
 * THE DEVIATION THIS FILE'S HEAD DESCRIBES: an ordinary localized, case-insensitive, numeric-aware comparison,
 * WITHOUT the foundry-prefix rule, which turns on a list Apple does not publish. The signature is Apple's and
 * the parameters arrive as `const void *` because that is what `CFArraySortValues` hands a comparator. */
CFComparisonResult CTFontManagerCompareFontFamilyNames(const void *family1, const void *family2,
						       void *context);

/* THE FACES IN ONE FILE. Apple's own note is that the answer is descriptors "for each of the fonts in the
 * specified URL" — one file may hold several — AND THAT THEY ARE "NOT AVAILABLE THROUGH FONT DESCRIPTOR
 * MATCHING", which is the one thing this door must not do: it does not register anything. */
CFArrayRef CTFontManagerCreateFontDescriptorsFromURL(CFURLRef fileURL);

/* WHETHER THE ENGINE CAN OPEN IT, which is Apple's "a valid font that can be used on the current platform".
 * It answers the same question `RegisterFontsForURL` refuses on, and changes nothing. */
bool CTFontManagerIsSupportedFont(CFURLRef fontURL);

/* ---------------------------------------------------------------------------------------
 * THE SCOPE, WHICH IS APPLE'S VOCABULARY FOR HOW LONG A REGISTRATION LASTS.
 * ------------------------------------------------------------------------------------- */
enum {
	kCTFontManagerScopeNone = 0,		/* not registered */
	kCTFontManagerScopeProcess = 1,		/* this process, until unregistered or until it exits */
	kCTFontManagerScopeUser = 2,		/* all processes, this session and later ones -- NOT HONOURED HERE */
	kCTFontManagerScopeSession = 3		/* the current session -- NOT HONOURED HERE */
};
typedef uint32_t CTFontManagerScope;

/* ---------------------------------------------------------------------------------------
 * REGISTRATION. The plural doors are not a convenience: Apple's singular doors are the plural ones with a
 * one-element array, and BOTH REPORT PER-URL FAILURES THROUGH THE SAME KEY — one CFError carrying an array of
 * the URLs that failed. The singular doors therefore answer through the plural machinery rather than beside it,
 * so a caller cannot get two different behaviours for the same broken file.
 * ------------------------------------------------------------------------------------- */
/* TRUE WHEN THE FILE IS NOW REGISTERED. False when it is not, and the two ways that happens are told apart by
 * the ERROR OUT-PARAMETER: SET means the file (Apple's own code, from CTFontManagerErrors.h), UNSET means the
 * SCOPE (this file's head). */
bool CTFontManagerRegisterFontsForURL(CFURLRef fontURL, CTFontManagerScope scope, CFErrorRef *error);
bool CTFontManagerUnregisterFontsForURL(CFURLRef fontURL, CTFontManagerScope scope, CFErrorRef *error);

/* AND THE SAME TWO FOR MANY. `errors` receives a RETAINED array of CFError references, one per URL that failed,
 * or is left NULL when every URL succeeded; it may be NULL when the caller does not care which failed, and then
 * only the BOOLEAN is answered. The return is true only when EVERY URL succeeded, which is Apple's own sentence
 * for it. */
bool CTFontManagerRegisterFontsForURLs(CFArrayRef fontURLs, CTFontManagerScope scope, CFArrayRef *errors);
bool CTFontManagerUnregisterFontsForURLs(CFArrayRef fontURLs, CTFontManagerScope scope, CFArrayRef *errors);

/* WHAT SCOPE A URL IS REGISTERED IN, or `kCTFontManagerScopeNone`. THIS TIER HAS ONE SCOPE, so the answer is
 * `kCTFontManagerScopeProcess` or nothing — which is exactly the pair Apple's sentence describes. */
CTFontManagerScope CTFontManagerGetScopeForURL(CFURLRef fontURL);

/* ---------------------------------------------------------------------------------------
 * THE AUTO-ACTIVATION HALF, WHICH IS A CONVERSATION WITH A FONT SERVER this tier does not have: a request that
 * a font be activated on demand, and a run loop source that listens for those requests from another process.
 * The SETTING pair is implemented because a preference can be recorded; the source is not (see CTFontManager.c).
 * ------------------------------------------------------------------------------------- */
/* WHICH BUNDLE'S SETTING, and Apple gives it a value of its own: "the CTFontManager bundle identifier … to be
 * used with get or set global auto-activation settings". ITS VALUE IS AN INFERENCE (see CTFontManager.c). */
extern const CFStringRef kCTFontManagerBundleIdentifier;

enum {
	kCTFontManagerAutoActivationDefault = 0,	/* whatever the global setting says */
	kCTFontManagerAutoActivationDisabled = 1,
	kCTFontManagerAutoActivationEnabled = 2,
	kCTFontManagerAutoActivationPromptUser = 3	/* ask before activating */
};
typedef uint32_t CTFontManagerAutoActivationSetting;

/* RECORDED, NOT OBEYED, and the difference matters: there is no font server here to activate anything, so a
 * setting is a preference this tier remembers and answers back. Apple's `bundleIdentifier` argument selects
 * WHICH preference — NULL means the current application, `kCTFontManagerBundleIdentifier` the global one. */
void CTFontManagerSetAutoActivationSetting(CFStringRef bundleIdentifier,
					   CTFontManagerAutoActivationSetting setting);
CTFontManagerAutoActivationSetting CTFontManagerGetAutoActivationSetting(CFStringRef bundleIdentifier);

/* AND THE CHANGE NOTIFICATION a caller subscribes to when the registry moves. Apple's own note is that the
 * process scope is announced on a LOCAL notification centre and the user and session scopes on a distributed
 * one — which is a distinction this tier cannot have, since it has one scope. THE VALUE IS AN INFERENCE. */
extern const CFStringRef kCTFontManagerRegisteredFontsChangedNotification;

/* ---------------------------------------------------------------------------------------
 * AND OF THESE TWO, ONE IS DEFINED AND ONE IS NOT — WHICH THIS COMMENT HAS TO SAY, because "declared here and not
 * defined" was true of both and is now true of one, and a comment that stops being true is a document that lies.
 *
 * `CTFontManagerEnableFontDescriptors` IS DEFINED, IN `CTFontMatching.c`, BECAUSE THAT IS WHERE ITS STATE LIVES. It
 * enables and disables fonts from a descriptor array, which is PROCESS-GLOBAL MUTABLE STATE — THE FIRST THIS LIBRARY
 * HAS, since every object in it is immutable by construction — and it is guarded by a LOCK rather than caveated, because
 * APPLE'S OWN HEADER PROMISES THAT ALL OF ITS FUNCTIONS "are thread safe unless otherwise specified". A disabled face
 * is not one the application is told about, so the matching tier's ONE walk skips it and every matching door inherits
 * the answer.
 *
 * AND THE ONE STILL OWED NOW WAITS ON ITS TRANSPORT RATHER THAN ON ITS SUBJECT: THE CONVERSATION IS BUILT — both halves
 * of it, in `CTFontService.c`, answered by the matching tier and tested in-process — and WHAT IS MISSING IS THE PORT,
 * THE NAME AND THE PROCESS that would carry it (FONT-SERVICE.md), plus the fact that the door's own signature takes a
 * BLOCK and this target does not presently compile with `-fblocks`. It stays DECLARED AND NOT DEFINED, so reaching for it
 * is a link error rather than a source that can never fire. `CTFontManager.c` carries that reason in full.
 * ------------------------------------------------------------------------------------- */
void CTFontManagerEnableFontDescriptors(CFArrayRef descriptors, bool enable);

#if defined(__BLOCKS__)
CFRunLoopSourceRef CTFontManagerCreateFontRequestRunLoopSource(
	CFIndex sourceOrder,
	CFArrayRef (^createMatchesCallback)(CFDictionaryRef requestAttributes, pid_t requestingProcess));
#endif

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTMANAGER_H */
