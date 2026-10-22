/*
 * CTFontTraits.h — a font's classification: the symbolic traits, the stylistic classes, and the four
 * dictionary keys a traits dictionary is read with.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER, which is the guide this library is being rebuilt against
 * (PROVENANCE.md). Apple declares these in CTFontTraits.h and not in CTFont.h, and this library keeps a type
 * in the header Apple put it in.
 *
 * THE VALUES ARE APPLE'S, INCLUDING THE ONES THIS HEADER WOULD NOT HAVE TO GIVE. Apple publishes the case
 * names and, in this header, the VALUES TOO: kCTFontItalicTrait is `1 << 0`, kCTFontClassMaskTrait is
 * `15 << kCTFontClassMaskShift`, and the eleven stylistic classes are the OpenType 'OS/2' font-class numbers
 * shifted into the top four bits.
 *
 * AN EARLIER ATTEMPT AT THIS LIBRARY DECLARED ITS VALUES AS "the era's declaration ORDER ... Apple publishes
 * case names and no values". THAT RULE IS WRONG HERE — the values are right there in Apple's header — and it
 * happened to land on the correct numbers anyway. Nothing is guessed this time.
 *
 * AND THE FOUR KEYS ARE THE APPKIT'S STRINGS — `NSFontSymbolicTrait` and its three siblings — WHICH IS THE
 * POINT OF THEM RATHER THAN A COINCIDENCE: a traits dictionary is exchanged between the frameworks, so Apple's
 * Core Text and its `NSFontDescriptor` necessarily name the same trait the same way.
 *
 * AND NOTHING HERE CAN BE CHECKED AGAINST THAT COUNTERPART YET, WHICH IS SAID RATHER THAN IMPLIED: the AppKit's
 * declarations belong to `NSFontDescriptor`, THAT lives in the AppKit and not in Foundation, and the AppKit
 * repository (`libagora-app`) is an empty one with a `.gitignore`. So the strings are asserted in the suite for
 * what they ARE — literals matching Apple's vocabulary — and not against a local peer that does not exist.
 */
#ifndef CORE_TEXT_CTFONT_TRAITS_H
#define CORE_TEXT_CTFONT_TRAITS_H

#include <CoreFoundation/CFString.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE FOUR KEYS a traits dictionary is read with. THE THREE NUMBERS ARE NORMALIZED, and the ends of their
 * range are what Apple's own sentences say: 0.0 is the regular face, and weight, width (condensed to
 * expanded) and slant each run from -1.0 to 1.0. */
extern const CFStringRef kCTFontSymbolicTrait;
extern const CFStringRef kCTFontWeightTrait;
extern const CFStringRef kCTFontWidthTrait;
extern const CFStringRef kCTFontSlantTrait;

/* THE FONT CLASS LIVES IN THE TOP FOUR BITS of the symbolic traits, which is what this shift reserves. */
enum {
	kCTFontClassMaskShift = 28
};

/* THE SYMBOLIC TRAITS. THE UPPER SIXTEEN BITS DESCRIBE APPEARANCE AND THE LOWER SIXTEEN THE TYPEFACE, which
 * is Apple's own division and the reason the class mask sits where it does.
 *
 * THE TWO DEPRECATED BITS ARE DECLARED ANYWAY. Apple marked kCTFontItalicTrait and kCTFontBoldTrait
 * deprecated in favour of the slant and weight NUMBERS, and the surface ledger this work is measured against
 * records that in its `why` column. They are in the era's header, and a font's symbolic traits still answer
 * with them. */
enum {
	kCTFontItalicTrait      = (1 << 0),	/* deprecated in favour of kCTFontSlantTrait */
	kCTFontBoldTrait        = (1 << 1),	/* deprecated in favour of kCTFontWeightTrait */
	kCTFontExpandedTrait    = (1 << 5),	/* expanded and condensed are mutually exclusive */
	kCTFontCondensedTrait   = (1 << 6),
	kCTFontMonoSpaceTrait   = (1 << 10),	/* fixed-pitch glyphs where the face has them */
	kCTFontVerticalTrait    = (1 << 11),	/* vertical glyph variants and metrics */
	kCTFontUIOptimizedTrait = (1 << 12),

	kCTFontClassMaskTrait   = (15 << kCTFontClassMaskShift)
};
typedef uint32_t CTFontSymbolicTraits;

/* THE STYLISTIC CLASSES, which are the OpenType 'OS/2' font-class numbers in the top four bits. THE GAPS AT 6
 * AND 11 ARE APPLE'S — the 'OS/2' assignments skip them — and they are kept rather than closed up, because a
 * class is a number rather than a position. */
enum {
	kCTFontUnknownClass            = (0 << kCTFontClassMaskShift),
	kCTFontOldStyleSerifsClass     = (1 << kCTFontClassMaskShift),
	kCTFontTransitionalSerifsClass = (2 << kCTFontClassMaskShift),
	kCTFontModernSerifsClass       = (3 << kCTFontClassMaskShift),
	kCTFontClarendonSerifsClass    = (4 << kCTFontClassMaskShift),
	kCTFontSlabSerifsClass         = (5 << kCTFontClassMaskShift),
	kCTFontFreeformSerifsClass     = (7 << kCTFontClassMaskShift),
	kCTFontSansSerifClass          = (8 << kCTFontClassMaskShift),
	kCTFontOrnamentalsClass        = (9 << kCTFontClassMaskShift),
	kCTFontScriptsClass            = (10 << kCTFontClassMaskShift),
	kCTFontSymbolicClass           = (12 << kCTFontClassMaskShift)
};
typedef uint32_t CTFontStylisticClass;

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONT_TRAITS_H */
