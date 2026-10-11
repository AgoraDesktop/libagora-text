/*
 * CTFontTraits.h — a font's classification, and the traits that carry it.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **APPLE'S OWN HOME FOR THE STYLISTIC CLASSES** (`CTFontTraits.h`, where 10.6 declares them), which is why
 * this header exists here rather than the names being re-homed into `CTFont.h`: the standing rule is to match
 * Apple in WHICH HEADER a type lives in.
 *
 * The values are the era's declaration order under §36's rule — Apple publishes case names and no values —
 * and the SFNT vocabulary this file sits beside does NOT fix them (they are a class, not a feature index).
 */
#ifndef CORE_TEXT_CTFONT_TRAITS_H
#define CORE_TEXT_CTFONT_TRAITS_H

#include <CoreText/SFNTLayoutTypes.h>

/* **THE STYLISTIC CLASSES, in the era's order.** Names from the era's headers; a value shown here is one the vocabulary itself fixes
 * (a table tag IS its four characters, and a collection its identifier), otherwise §36's rule —
 * the era's declaration order. */
typedef enum {
	kCTFontClarendonSerifsClass = (4 << kCTFontClassMaskShift),
	kCTFontFreeformSerifsClass = (7 << kCTFontClassMaskShift),
	kCTFontModernSerifsClass = (3 << kCTFontClassMaskShift),
	kCTFontOldStyleSerifsClass = (1 << kCTFontClassMaskShift),
	kCTFontOrnamentalsClass = (9 << kCTFontClassMaskShift),
	kCTFontSansSerifClass = (8 << kCTFontClassMaskShift),
	kCTFontScriptsClass = (10 << kCTFontClassMaskShift),
	kCTFontSlabSerifsClass = (5 << kCTFontClassMaskShift),
	kCTFontSymbolicClass = (12 << kCTFontClassMaskShift),
	kCTFontTransitionalSerifsClass = (2 << kCTFontClassMaskShift),
	kCTFontUnknownClass = (0 << kCTFontClassMaskShift),
} CTFontStylisticClass;

/* **THE TRAIT KEYS `CTFontCopyTraits` ANSWERS WITH — APPLE'S FOUR, AND TWO PHANTOMS DELETED.** Apple declares
 * exactly these in `CTFontTraits.h`: the symbolic traits, and the three NORMALIZED numbers whose sentences the
 * era gives in full ("a float value between -1.0 and 1.0 for normalized weight. The value of 0.0 corresponds to
 * the regular or medium font weight", and likewise for width and slant).
 *
 * **`kCTFontSizeTrait` AND `kCTFontFamilyNameTrait` ARE GONE.** This header used to declare them and
 * `CTFontCopyTraits` put them in its dictionary. §46 found them — a name in no Apple artefact this tree holds,
 * whose VALUE was AppKit's own spelling (`@"NSFontSizeAttribute"`, `@"NSFontFamilyAttribute"`), which is where
 * they came from. They were the same class as the phantom `kCTFontManagerErrorUnrecognizedFileFormat`: the
 * instrument's universe is Apple's symbol list, so a name Apple does not declare is never a row and is never
 * compared with anything. **THE PROBE NOW CHECKS THAT THE DICTIONARY HAS EXACTLY FOUR ENTRIES**, which is the
 * check that would have caught them.
 *
 * The VALUES are ours (Apple publishes the names and the value TYPES, never the strings), which is what makes
 * them safe: a key is only ever compared with the constant declared beside it. */
extern NSString *const kCTFontSymbolicTrait;
extern NSString *const kCTFontWeightTrait;
extern NSString *const kCTFontWidthTrait;
extern NSString *const kCTFontSlantTrait;

#endif /* CORE_TEXT_CTFONT_TRAITS_H */
