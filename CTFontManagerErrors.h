/*
 * CTFontManagerErrors.h — the font manager's error vocabulary: the domain its errors carry, the user-info key
 * that names the URLs which failed, and the codes registration fails with.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), and it is a header of its own because Apple makes it one:
 * `CTFontManager.h` INCLUDES this file and does not carry these declarations itself, so this is written first —
 * Apple's own include order, not a preference.
 *
 * IT HOLDS TWO KINDS OF FACT AND THEY ARE OF DIFFERENT STANDING, which is why this comment separates them.
 * THE CODES ARE APPLE'S PUBLISHED VALUES, printed in its header beside each name, and they are copied here
 * exactly — INCLUDING THE ONE APPLE GOT WRONG. `kCTFontManagerErrorInUse` and `kCTFontManagerErrorSystemRequired`
 * BOTH CARRY THE VALUE `202` in Apple's own header, so an in-use refusal and a system-required refusal are
 * indistinguishable to a caller, and THIS FILE REPRODUCES THAT RATHER THAN REPAIRING IT: a library that
 * renumbered one of them would answer a question Apple's framework cannot, and a caller comparing against the
 * era's constants would be wrong in a way that is invisible until it matters. The suite asserts the collision.
 *
 * THE TWO STRINGS ARE INFERENCES AND NOT VALUES, and the reason is measured rather than assumed. Apple declares
 * `kCTFontManagerErrorDomain` and `kCTFontManagerErrorFontURLsKey` and publishes NO value for either — and the
 * rule that carries the other Core Text keys across to the AppKit's spelling CANNOT BE APPLIED HERE, because the
 * AppKit declares no counterpart: AT 10.6 `NSFontManager.h` DECLARES NO `extern const …StringRef` AT ALL (there
 * is no `NSFontManagerErrorDomain` and no `NSFontManagerErrorFontURLsKey`), and `FoundationErrors.h` declares no
 * domain constant either — it names its domain only in a comment. So what is written below is the constants'
 * OWN NAMES WITH THE LEADING `k` DROPPED, which is the form Apple's newer Core domains take, and both are
 * recorded as the first thing to check the day something can answer for them.
 */
#ifndef CORE_TEXT_CTFONTMANAGERERRORS_H
#define CORE_TEXT_CTFONTMANAGERERRORS_H

#include <CoreFoundation/CoreFoundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE DOMAIN EVERY ERROR FROM THIS FRAMEWORK CARRIES. An error is identified by its domain AND its code, which
 * is what makes the collision below survivable: nothing here is a `CTFontManagerError` on its own. */
extern const CFStringRef kCTFontManagerErrorDomain;

/* AND WHERE THE URLS THAT FAILED ARE NAMED: Apple's own sentence for it is that the value "is a CFArray of font
 * URLs that failed with given error", which is how ONE error in the plural doors below accounts for several
 * files. */
extern const CFStringRef kCTFontManagerErrorFontURLsKey;

/* ---------------------------------------------------------------------------------------
 * THE CODES, AS APPLE PRINTED THEM. Two ranges, and the split is Apple's: 101–105 are refusals of a
 * REGISTRATION and 201–202 are refusals of an UNREGISTRATION.
 * ------------------------------------------------------------------------------------- */
enum {
	kCTFontManagerErrorFileNotFound = 101,			/* the file does not exist at the specified URL */
	kCTFontManagerErrorInsufficientPermissions = 102,	/* the file cannot be accessed */
	kCTFontManagerErrorUnrecognizedFormat = 103,		/* not a recognised or supported font file */
	kCTFontManagerErrorInvalidFontData = 104,		/* invalid font data that could harm the system */
	kCTFontManagerErrorAlreadyRegistered = 105		/* already registered in the specified scope */
};

enum {
	kCTFontManagerErrorNotRegistered = 201,			/* not registered in the specified scope */
	kCTFontManagerErrorInUse = 202,				/* in use and cannot be unregistered */
	kCTFontManagerErrorSystemRequired = 202			/* required by the system -- APPLE'S OWN DUPLICATE */
};

typedef CFIndex CTFontManagerError;

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTMANAGERERRORS_H */
