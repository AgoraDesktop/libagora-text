/*
 * CTFontManager.h — what fonts this system has, and what it can be asked about them.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE READ HALF OF THE FONT MANAGER ANSWERS FROM ONE ENUMERATION** — `cg_font_enumerate`, the CoreGraphics
 * seam's walk of the FSH font directories and `FN_FONT_PATH`'s override. So the family names a caller is told
 * about, the PostScript names it can open, the files those faces live in, and the descriptors a collection
 * reports are ONE list seen four ways, which is what the plan's M5 proof asks for ("the shipped DejaVu faces
 * enumerated by family name, through the SAME registry").
 *
 * **AND THE REGISTERING HALF IS HERE NOW (CT-M7h), WHICH IS A REVERSAL OF WHAT THIS COMMENT SAID.** It used to
 * record the whole write half as absent — "they write to a font directory and need a settings domain to
 * remember what was enabled, which is the FSH/config unit's work rather than Core Text's" — and THAT REASONING
 * WAS WRONG FOR FOUR OF THEM: registering a font does not write to a directory, it adds a file to the
 * PROCESS'S registry, and the enumeration above was always going to have to see it. `…RegisterFontsForURL(s)`
 * and `…UnregisterFontsForURL(s)` therefore land here, over a `cg_font_register`/`cg_font_unregister` pair
 * beside the directory list they join.
 *
 * **AND THE AUTO-ACTIVATION SETTINGS LAND HERE TOO (CT-M7j)**, which is the ONE part of the write half the old
 * comment was right about: they really are a preferences location, and a preferences location is a config
 * DOMAIN. So `…Get/SetAutoActivationSetting` and their `kCTFontManagerBundleIdentifier` key ship over
 * `system.fontmanager` — this framework's own first-party domain, in the tree's config model
 * (`docs/design/config-design.md`), which is why `libcoretext` now links `libconfig`.
 *
 * **AND `…EnableFontDescriptors` LANDS WITH THEM (CT-M7k)**, over a set the descriptor MATCHER consults — which
 * is the whole of what Apple's sentence promises ("enables or disables the matching font descriptors for font
 * descriptor matching") and nothing more: a disabled face is still AVAILABLE, and only MATCHING stops answering
 * it.
 *
 * **STILL OWED, WITH ITS REASON.** `…CreateFontRequestRunLoopSource` needs a `CFRunLoopSourceRef`, a block and a
 * request port, none of which this tier has.
 *
 * **AND THE SCOPE IS REFUSED RATHER THAN FAKED**: only `kCTFontManagerScopeProcess` is honoured. `User` and
 * `Session` mean "available in subsequent sessions" (the era's own sentences), which needs a persistent font
 * store this tier does not have — so the doors answer false rather than registering something that would
 * silently vanish with the process. The era's error vocabulary has NO code for an unsupported scope (that
 * arrives later, as the post-10.6 `kCTFontManagerErrorUnsupportedScope`), so none is set: the refusal is the
 * return value and the reason is here.
 */
#ifndef CORE_TEXT_CTFONTMANAGER_H
#define CORE_TEXT_CTFONTMANAGER_H

#include <CoreGraphics/CGBase.h>
#include <CoreText/CTFontDescriptor.h>
/* **APPLE'S INCLUDE GRAPH AS WELL AS APPLE'S PLACEMENT**: the error vocabulary lives in
 * `CTFontManagerErrors.h` (see that header), and this header includes it. */
#include <CoreText/CTFontManagerErrors.h>

/* **THE TWO ENUMS COME FIRST, AND THEY HAVE TO**: `CTFontManagerGetScopeForURL` below returns one of them, and
 * a C typedef is visible only from where it is declared. Apple's header has the same ordering constraint (its
 * scope enum precedes its doors) and this one keeps it. */
typedef enum {
	kCTFontManagerScopeNone = 0,
	kCTFontManagerScopeProcess = 1,
	kCTFontManagerScopeUser = 2,
	kCTFontManagerScopeSession = 3
} CTFontManagerScope;

/* The auto-activation settings (CTFontManager), in Apple's own home. THE ENUM SHIPS WITH ITS DOORS' ABSENCE
 * RECORDED: the pair that reads and writes it is a preferences store, so the vocabulary is here for a caller
 * that names the type while the doors stay owed with the write half. */
typedef enum {
	kCTFontManagerAutoActivationDefault = 0,
	kCTFontManagerAutoActivationDisabled = 1,
	kCTFontManagerAutoActivationEnabled = 2,
	kCTFontManagerAutoActivationPromptUser = 3
} CTFontManagerAutoActivationSetting;

#ifdef __cplusplus
extern "C" {
#endif

/* THE FAMILY NAMES, RETAINED (+1) — one per family the enumeration found, DEDUPLICATED (a family with four
 * faces is one name) and in the order the directories are searched, because sorting is a presentation choice
 * and Apple's own order here is unspecified. */
NSArray *CTFontManagerCopyAvailableFontFamilyNames(void) CG_RETURNS_RETAINED;

/* AND THE POSTSCRIPT NAMES, one per FACE: these are the names `CTFontCreateWithName` (and
 * `CGFontCreateWithFontName` before it) resolve, so this door answers "what can I ask for" rather than "what
 * families exist". */
NSArray *CTFontManagerCopyAvailablePostScriptNames(void) CG_RETURNS_RETAINED;

/* **AND THE FILES THEMSELVES** — Apple's own sentence is "an array of font URLs" and the era's return type is
 * a `CFURLRef`, so this is the third reading of the same walk, one URL per FACE rather than one per family.
 * It is the door a caller uses to hand a font to something that takes a path, and it is why the enumeration
 * answers PATHS rather than names. */
NSArray *CTFontManagerCopyAvailableFontURLs(void) CG_RETURNS_RETAINED;

/* **AND THE DESCRIPTORS OF ONE FILE, WHICH IS HOW A FACE THAT IS NOT IN A FONT DIRECTORY GETS ONE.** Apple's
 * door takes a `CFURLRef`; this tree's `NSURL` is what `CGBase.h` lets a C header name, and the file-URL to
 * path rule is the Foundation's (§F8). A URL that is not a readable face answers an EMPTY array rather than
 * NULL: "no descriptors in that file" is an answer, and Apple's own door returns a (possibly empty) array. */
NSArray *CTFontManagerCreateFontDescriptorsFromURL(NSURL *url) CG_RETURNS_RETAINED;

/* **M7h: THE REGISTERING HALF.** Four doors over one registry, and each is one of the era's sentences:
 * "Registers fonts from the specified font URL with the font manager. Registered fonts are discoverable through
 * font descriptor matching" — which is why the registry lives in the CoreGraphics seam beside the directory
 * list: DISCOVERABLE means the enumeration and the name lookup must both see it, or the caller has remembered
 * something nothing can find.
 *
 * `error` and `errors` ARE `NSError *`/`NSArray **` HERE WHERE THE ERA TAKES `CFErrorRef *`/`CFArrayRef *`, the
 * same mapping every other door in this framework makes. **AND THEY ARE AUTORELEASED, WHICH IS A STATED
 * DEVIATION**: Apple's out-parameter says "must be released by caller", and Foundation's convention for an
 * `NSError **` — which is what this spelling IS — is not to. Autoreleased is the safer half of the difference
 * (a caller that forgets to release does not leak), and it is written down because a caller porting Apple
 * source would otherwise release something it does not own.
 *
 * `bool`, not `BOOL`: this is a C header, and the era's doors return `bool`. */
bool CTFontManagerRegisterFontsForURL(NSURL *fontURL, CTFontManagerScope scope, NSError **error);
bool CTFontManagerRegisterFontsForURLs(NSArray *fontURLs, CTFontManagerScope scope, NSArray **errors);
bool CTFontManagerUnregisterFontsForURL(NSURL *fontURL, CTFontManagerScope scope, NSError **error);
bool CTFontManagerUnregisterFontsForURLs(NSArray *fontURLs, CTFontManagerScope scope, NSArray **errors);

/* **M7k: ENABLING AND DISABLING FOR MATCHING.** Apple's signature returns `void`, takes `CFArrayRef
 * descriptors` and a `bool enable`, and says exactly one thing about the effect: it "enables or disables the
 * matching font descriptors for font descriptor matching". So the set is the MATCHER's — `CTFontDescriptor.m`
 * owns it, because that is where the only reader is — and the descriptors a caller passes are the ones a match
 * gave it: a descriptor this tier cannot name changes nothing, since an identity that cannot be stated cannot
 * be remembered. `false` when `descriptors` is NULL, which is Apple's own "an array of font descriptors" read
 * strictly. */
void CTFontManagerEnableFontDescriptors(NSArray *descriptors, bool enable);

/* **IS THIS FILE A FONT THIS PLATFORM CAN USE?** Apple's sentence is "determines whether the referenced font
 * data (usually by file URL) is supported on the current platform", so the answer is about the FILE's data
 * rather than about whether the registry knows it: this door opens the file the way the create door would and
 * answers whether a face came back. A URL that is not a file at all, or a file that is not a face, is false —
 * which is the case that makes it a question rather than a tautology. `bool`, not `BOOL`: this is a C header,
 * and `CGBase.h` takes `<stdbool.h>` for exactly this (the era's own declaration returns `bool`). */
bool CTFontManagerIsSupportedFont(NSURL *fontURL);

/* **AND THE REGISTRATION SCOPE OF A URL — WHICH CT-M7h TURNED FROM A CONSTANT INTO A QUESTION.** When this door
 * shipped (§M7a) the read half was all there was and the answer was `kCTFontManagerScopeNone` for every URL,
 * with the argument that "this tier registers nothing". **THAT ARGUMENT IS NOW HALF WRONG, AND THE DOOR IS
 * BETTER FOR IT**: a font directory's face is still NOT registered — it is read at load and available to the
 * process without any registration step — while a path a caller registered answers
 * `kCTFontManagerScopeProcess`, which is the era's own sentence for exactly that lifetime ("available to the
 * current process for the duration of the process unless directly unregistered").
 *
 * **SO THE DISCRIMINATING PAIR IS NOW THE WHOLE TEST**: an AVAILABLE face out of a directory answers None, a
 * REGISTERED one answers Process, and any other URL answers None. A door that answered by availability would
 * get the first one wrong; a door that answered a constant would get the second one wrong. */
CTFontManagerScope CTFontManagerGetScopeForURL(NSURL *fontURL);

/* **APPLE'S COMPARATOR FOR FAMILY NAMES, AND ITS CONTRACT IS TWO SENTENCES**: it sorts "according to Apple
 * guidelines", and "family names with RECOGNIZED PREFIXES are sorted after the un-prefixed names in prefix
 * order". The signature is Apple's own — a `CFComparatorFunction` takes `const void *` because it is handed to
 * `CFArraySortValues`, and the argument is a family name — and the result is a `CFComparisonResult`, which is
 * this tree's `NSInteger` (-1 / 0 / +1, `NSComparisonResult`).
 *
 * **AND THE SECOND SENTENCE IS ONLY HALF IMPLEMENTABLE, WHICH IS STATED RATHER THAN PAPERED OVER.** The SET of
 * recognized prefixes, and the ORDER among them, is a table Apple does not publish anywhere this tree can read
 * (10.6's header names the concept and gives no list). What the sentence DOES fix is the observable shape, and
 * that is what this door implements: an un-prefixed family sorts before a prefixed one, and within each group
 * the comparison is the ordinal one. "Prefixed" is therefore defined by the only evidence there is — a name
 * whose first character is not a letter or a digit, which is the shape of every hidden family Apple's own
 * system ships (`.Helvetica Neue`, `.SF NS`). A caller that needs Apple's internal prefix SEQUENCE needs a
 * table this tier would have to invent, and inventing it is how a surface stops being Apple's. */
NSInteger CTFontManagerCompareFontFamilyNames(const void *family1, const void *family2, void *context);

/* **AND THE NOTIFICATION A CLIENT SUBSCRIBES TO** — "the string to use as the notification name when
 * subscribing to CTFontManager notifications", in Apple's own words, posted "when fonts are added to the font
 * registry". The name is the whole of the constant: `NSNotificationCenter` is the mechanism this tier has, and
 * a subscriber names the notification with this. The value is ours (Apple publishes no string), which is what
 * makes it safe — a notification name is only ever compared with the constant declared beside it. */
extern NSString *const kCTFontManagerRegisteredFontsChangedNotification;

/* **M7j: THE AUTO-ACTIVATION SETTINGS — A REAL PREFERENCES DOMAIN.** Apple's sentence for the key is "The
 * CTFontManager bundle identifier to be used with get or set global auto-activation settings", and for the
 * doors: "Sets the auto-activation for the specified bundle identifier ... If NULL, the current application
 * bundle will be used. If kCTFontManagerBundleIdentifier is specified, will set the global auto-activation
 * settings", with the result that the function "will apply the setting to the appropriate preferences
 * location".
 *
 * **SO THERE ARE TWO THINGS TO MAP AND BOTH ARE MAPPED RATHER THAN GUESSED**: the preferences location is the
 * tree's own config model — a first-party domain `system.fontmanager`, written at `CONFIG_SCOPE_USER` and READ
 * through the library's ordinary resolution (system → user → shared, `config-design.md` §2), so an
 * administrator can pin a system-wide default that a user's own setting overrides; and a NULL bundle
 * identifier is **Apple's own case, honoured rather than refused**, because this tree HAS a main bundle
 * (`+[NSBundle mainBundle]` and `-bundleIdentifier`).
 *
 * **AND THE ONE CASE WITH NOWHERE TO PUT THE SETTING IS SAID OUT LOUD**: a NULL identifier for a process with
 * no bundle identifier — a command-line tool — has no key to write under, so the SET does nothing and the GET
 * answers the default. Apple's sentence assumes an application bundle exists; this tier's processes need not
 * have one, and inventing an identifier would file a setting under a name no other caller could ask for.
 *
 * The setting's VALUE is the era's `CTFontManagerAutoActivationSetting`, and an absent entry answers
 * `kCTFontManagerAutoActivationDefault` — "when specified, the application will use the global setting", which
 * is the honest answer for a key that was never written. */
extern NSString *const kCTFontManagerBundleIdentifier;
void CTFontManagerSetAutoActivationSetting(NSString *bundleIdentifier, CTFontManagerAutoActivationSetting setting);
CTFontManagerAutoActivationSetting CTFontManagerGetAutoActivationSetting(NSString *bundleIdentifier);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTMANAGER_H */
