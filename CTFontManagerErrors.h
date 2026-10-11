/*
 * CTFontManagerErrors.h — the font manager's error vocabulary.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THIS IS APPLE'S OWN HEADER FOR THESE NAMES** (`CTFontManagerErrors.h`, where 10.6 declares them), created
 * here rather than re-homed into `CTFontManager.h`: the standing rule is to match Apple in which header a type
 * lives in, and `CTFontManager.h` includes this one — Apple's include graph as well as Apple's placement.
 *
 * **AND THE VALUES ARE NOT OURS HERE, WHICH IS A CORRECTION THIS UNIT HAD TO MAKE.** The previous revision of
 * this header carried the framework's usual sentence — "Apple publishes enum case NAMES and no values" — and
 * numbered them 1..8. THAT SENTENCE IS FALSE FOR THIS HEADER, and it was only ever a claim: 10.6's own
 * `CTFontManagerErrors.h` writes `= 101`, `= 102`, … beside every case. §37's refined rule is that **a value is
 * ours only where nothing outside fixes it**, and here Apple's own header fixes all eight, so the numbers below
 * are Apple's. A caller that stored an error code and compared it across a rebuild was reading a value this
 * tree had invented.
 *
 * **AND ONE NAME WAS A PHANTOM.** The same revision declared `kCTFontManagerErrorUnrecognizedFileFormat` — a
 * name that appears in NO Apple surface and in NONE of the three SDKs this tree holds (10.2, 10.5, 10.6). The
 * surface rule is "match Apple except where specifically struck", so a name Apple does not have may not be
 * declared: it is deleted, and `kCTFontManagerErrorUnrecognizedFormat` (103) is the real one. Nothing in the
 * tree referenced the phantom, so nothing had to be rewritten.
 *
 * AND `CTFontManagerError` IS DECLARED AS THIS FRAMEWORK'S ENUMS ARE (a tagged `typedef enum`), NOT as Apple's
 * `typedef CFIndex` over two anonymous enums. That is this tree's landed shape for the whole CT vocabulary, and
 * every case name and every value below is Apple's.
 */
#ifndef CTFONTMANAGERERRORS_H
#define CTFONTMANAGERERRORS_H

#include <CoreGraphics/CGBase.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE ERROR DOMAIN and the user-info KEY are `CFStringRef` constants in the era. `CGBase.h`'s rule is what lets
 * a C header here name the type: under `__OBJC__` it is the real `NSString`, and elsewhere the C spelling of the
 * object pointer — the same two-armed typedef the attribute keys use. Values are OURS (Apple publishes no
 * string for any error domain or user-info key), which is what makes them safe: a domain is only ever compared
 * with the constant declared beside it. */
extern NSString *const kCTFontManagerErrorDomain;
extern NSString *const kCTFontManagerErrorFontURLsKey;

#ifdef __cplusplus
}
#endif

/* AND THE ERRORS THEMSELVES, in the era's declaration order and with the era's own numbers. */
typedef enum {
	kCTFontManagerErrorFileNotFound = 101,
	kCTFontManagerErrorInsufficientPermissions = 102,
	kCTFontManagerErrorUnrecognizedFormat = 103,
	kCTFontManagerErrorInvalidFontData = 104,
	kCTFontManagerErrorAlreadyRegistered = 105,
	kCTFontManagerErrorNotRegistered = 201,
	kCTFontManagerErrorInUse = 202,
	/* **APPLE GIVES THIS ONE `InUse`'s VALUE, AND THAT IS REPRODUCED RATHER THAN TIDIED.** 10.6's header writes
	 * `kCTFontManagerErrorInUse = 202` and `kCTFontManagerErrorSystemRequired = 202` — a duplicate in Apple's
	 * own file. The surface rule is "match Apple", so the duplicate ships: silently numbering it 203 would make
	 * a caller's comparison against Apple's own constant wrong, which is a worse defect than a repeated number.
	 * It is the ninth and last case of the era's vocabulary. */
	kCTFontManagerErrorSystemRequired = 202
} CTFontManagerError;

#endif /* CTFONTMANAGERERRORS_H */
