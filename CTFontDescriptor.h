/*
 * CTFontDescriptor.h — the descriptor TYPE and the orientation vocabulary.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THIS FILE IS MOSTLY A TYPE HEADER TODAY, and that is a declared boundary rather than an accident:
 * `CTFontDescriptorRef` is the only thing `CTFontCreateWithGraphicsFont` needs in order to have a
 * parameter, and `CTFontOrientation` is declared HERE IN APPLE'S OWN FILE (`CTFontDescriptor.h`, its
 * line 174) rather than in `CTFont.h` — which is why this header exists before the descriptor class
 * does. The descriptor's own doors (`CTFontDescriptorCreateWithAttributes`,
 * `CTFontDescriptorCreateCopyWithAttributes`, the attribute keys, …) are a unit of their own and every
 * one of their ledger rows stays OPEN.
 *
 * AND THE TYPEDEF COMES WITH A PROMISE THIS TIER CANNOT KEEP YET, NAMED HERE WHERE A CALLER MEETS IT:
 * Apple's `CTFontCreateWithFontDescriptor` says "a best match font will always be returned", and this
 * tier's font registry ANSWERS NULL for a name it cannot resolve (that is `CGFontCreateWithFontName`'s
 * stated rule — it "ANSWERS NULL rather than substituting a font the caller did not ask for"). A
 * `CTFontRef` here is therefore NULLABLE in practice, and every door below tolerates NULL by answering
 * its empty value rather than crashing.
 *
 * THE TYPES APPLE SPELLS WITH COREFOUNDATION, AND WHAT THIS TREE SPELLS INSTEAD. Core Text's
 * signatures use `CFIndex`, `UniChar`, `CFStringRef` and `CFTypeRef`; THIS TREE HAS NO CORE
 * FOUNDATION (measured: no `CFIndex` or `UniChar` anywhere in Foundation or CoreGraphics, and
 * `CGBase.h` names `CFStringRef` only in a comment about how Apple's headers are spelled). The
 * conventions here are the ones CoreGraphics already follows:
 *
 *   * `CFIndex`      -> `long`            (Apple's is a signed pointer-sized integer)
 *   * `UniChar`      -> `unsigned short`  (a UTF-16 code unit; `CGGlyph` is already `unsigned short`)
 *   * `CFStringRef`  -> not here; the two string doors that need one are CT-M1b's and will take
 *                       `NSString *` in a `.m`, exactly as `CGFontCreateWithFontName` does
 *   * `bool`         -> `bool`            (`stdbool`, which `CGFontGetGlyphAdvances` already returns)
 */
#ifndef CORE_TEXT_CTFONTDESCRIPTOR_H
#define CORE_TEXT_CTFONTDESCRIPTOR_H

#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#include <CoreGraphics/CGBase.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AN OPAQUE REFERENCE, and SINCE 2026-10-09 IT IS A REAL OBJECTIVE-C OBJECT RATHER THAN A C STRUCT
 * (`dec-35c114bf75c7c2d7`, the user's decision): Apple's `CTFontDescriptorRef` is a CFType, a CFType is
 * an object that lives in a dictionary, and this tree has neither CoreFoundation nor a bridge — so the
 * class IS how that is spelled here, and an attribute value can be the object instead of a wrapper
 * around it. **THE C API IS UNCHANGED**: the ref is a pointer to the class, every door keeps its
 * signature, and `CTFontDescriptorRetain`/`Release` are thin wrappers over `-retain`/`-release`.
 *
 * THE TWO ARMS ARE `CGBase.h`'s OWN IDIOM, one level up: an Objective-C translation unit meets the
 * class, and a C one meets a `struct objc_object` it can hold and pass but not message. */
#ifdef __OBJC__
@class CTFontDescriptor;
#else
typedef struct objc_object CTFontDescriptor;
#endif
typedef CTFontDescriptor *CTFontDescriptorRef;

/* APPLE'S OWN ENUM, VALUES AND ALL (its `CTFontDescriptor.h` lines 169-174): the intended rendering
 * orientation of a font for obtaining glyph metrics. THE VALUES ARE THE INTERFACE — a caller passes
 * one to `CTFontGetAdvancesForGlyphs`, so a renumbered case would silently ask for the wrong metrics. */
enum {
	kCTFontDefaultOrientation = 0,
	kCTFontHorizontalOrientation = 1,
	kCTFontVerticalOrientation = 2
};
typedef uint32_t CTFontOrientation;

/* ------------------------------------------------------------------------------------------------
 * CT-M1c: THE DESCRIPTOR'S DOORS, AND THE ATTRIBUTE KEYS.
 *
 * A descriptor IS an attributes dictionary — Apple's own door says it "can contain arbitrary attributes
 * that will be preserved" — and the dictionary is `NSDictionary *` here for the reason `CTFont.h` gives
 * for its strings: Apple's is `CFDictionaryRef` and this tree has no CoreFoundation. `CGBase.h` teaches a
 * C consumer the type and the +1 a `Copy` door owes, so these declarations are usable from both
 * languages, exactly as the CoreGraphics font registry's are.
 *
 * **THE TWO "ANY OBJECT" DOORS ARE HERE NOW, AND THE TYPE THAT WAS OWED IS DECLARED**: this note used to
 * say that `CTFontDescriptorCopyAttribute` and `CTFontDescriptorCopyLocalizedAttribute` stayed open "until a
 * unit decides that type", rather than being spelled as one of `CGBase.h`'s six concrete names and being
 * wrong for the other five. The unit decided it, and the answer was already in this tree: **`CFTypeRef`**,
 * declared in `CGBase.h` by the same rule its `CFTypeID` is, so that source written against Apple's headers
 * compiles unmodified. The values are this tier's Objective-C objects.
 *
 * THE KEY VALUES ARE OURS: Apple publishes the names and the type of each VALUE (`CFNumberRef`,
 * `CFDataRef`, a dictionary) and never the strings, and a key is only ever compared with the constants
 * declared beside it.
 * ---------------------------------------------------------------------------------------------- */
extern const CFStringRef kCTFontURLAttribute;
extern const CFStringRef kCTFontNameAttribute;
extern const CFStringRef kCTFontDisplayNameAttribute;
extern const CFStringRef kCTFontFamilyNameAttribute;
extern const CFStringRef kCTFontStyleNameAttribute;
extern const CFStringRef kCTFontTraitsAttribute;
extern const CFStringRef kCTFontVariationAttribute;
extern const CFStringRef kCTFontSizeAttribute;
extern const CFStringRef kCTFontMatrixAttribute;
extern const CFStringRef kCTFontCascadeListAttribute;
extern const CFStringRef kCTFontCharacterSetAttribute;
extern const CFStringRef kCTFontLanguagesAttribute;
extern const CFStringRef kCTFontBaselineAdjustAttribute;
extern const CFStringRef kCTFontMacintoshEncodingsAttribute;
extern const CFStringRef kCTFontFeaturesAttribute;
extern const CFStringRef kCTFontFeatureSettingsAttribute;
extern const CFStringRef kCTFontFixedAdvanceAttribute;
extern const CFStringRef kCTFontOrientationAttribute;
extern const CFStringRef kCTFontFormatAttribute;
extern const CFStringRef kCTFontRegistrationScopeAttribute;
extern const CFStringRef kCTFontPriorityAttribute;
extern const CFStringRef kCTFontEnabledAttribute;

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S (a `CTFontDescriptorRef` is a CFType and
 * there is no CoreFoundation here), exactly as `CTFontRetain`/`CTFontRelease` are. No ledger row. */
CTFontDescriptorRef CTFontDescriptorRetain(CTFontDescriptorRef descriptor);

/* **AND THE TYPE IDENTITY (CT-M7g).** This header never recorded its absence, so there is no sentence to
 * correct here — the argument for all of them is in `CTTypeID_internal.h`. */
CFTypeID CTFontDescriptorGetTypeID(void);
void CTFontDescriptorRelease(CTFontDescriptorRef descriptor);

/* THE THREE CONSTRUCTORS. `CreateWithNameAndSize` OMITS the size attribute when 0.0 is passed (Apple's
 * own sentence), which is why "absent" is a state this class has; `CreateCopyWithAttributes` augments its
 * original and MERGES the two dictionary-valued keys the documentation excepts from replacement. */
CTFontDescriptorRef CTFontDescriptorCreateWithAttributes(CFDictionaryRef attributes);
CTFontDescriptorRef CTFontDescriptorCreateWithNameAndSize(CFStringRef name, CGFloat size);
CTFontDescriptorRef CTFontDescriptorCreateCopyWithAttributes(CTFontDescriptorRef original,
							     CFDictionaryRef attributes);

/* AND THE ACCESSOR THAT NEEDS NO NEW TYPE: the dictionary itself, retained. */
CFDictionaryRef CTFontDescriptorCopyAttributes(CTFontDescriptorRef descriptor) CG_RETURNS_RETAINED;

#ifdef __cplusplus
}
#endif

/* **A HEADING CT-M7e HAD TO CORRECT, BECAUSE IT SPOKE FOR TWO DIFFERENT ENUMS.** It said "THE VALUES ARE
 * OURS": Apple publishes enum case NAMES and no values, the default case is 0, a mask's cases are powers of two,
 * and everything else is the era's order. **THAT IS TRUE OF MANY OF THIS FRAMEWORK'S ENUMS AND FALSE OF BOTH
 * ENUMS UNDER IT.** The era's own header publishes every value below — the priority's six spaced numbers, and
 * the formats' 0..5 (`kCTFontFormatUnrecognized = 0` through `kCTFontFormatBitmap = 5`). **THE FORMAT VALUES
 * WERE MEASURED AGAINST THE ERA AND ALREADY MATCHED, so nothing about them changed**; only the heading that
 * claimed they were ours is gone. What the heading got right, and what stands: the SET OF NAMES is never ours.
 */
/* the priority vocabulary, in CTFontDescriptor.h (Apple's own home).
 *
 * **THE PRIORITY VALUES ARE APPLE'S, AND TWO THINGS WERE WRONG HERE UNTIL CT-M7e MEASURED THEM.** This block
 * sat under "Apple publishes enum case NAMES and no values" and numbered them 1..6. **The era's header
 * publishes ALL SIX**: 10000, 20000, 30000, 40000, 50000, 60000 — and the SPACING is the point of them,
 * because the value IS the priority and a caller may compare or sort by it. A `kCTFontPrioritySystem` of 1
 * would have ordered system fonts LAST for anyone comparing the numbers.
 *
 * **AND `kCTFontPrioritySession` WAS A NAME APPLE DOES NOT HAVE.** It sat at 5 where the era declares
 * `kCTFontPriorityDynamic = 50000`; it appears in no Apple artefact this tree holds — what put it here is
 * Apple's own doc PROSE, whose discussion for that block mentions a "session" scope while the enum body
 * declares `Dynamic`. It is DELETED (nothing in the tree referenced it) and `Dynamic` takes its place, which
 * is the THIRD phantom this framework has turned up: the instrument's universe is Apple's symbol list, so a
 * name Apple does not declare is not a row and is never compared with anything. */
typedef enum {
	kCTFontPrioritySystem = 10000,
	kCTFontPriorityNetwork = 20000,
	kCTFontPriorityComputer = 30000,
	kCTFontPriorityUser = 40000,
	kCTFontPriorityDynamic = 50000,
	kCTFontPriorityProcess = 60000
} CTFontPriority;

/* AND THE FORMATS — `Unrecognized` is the zero, and the rest are the era's order. */
typedef enum {
	kCTFontFormatUnrecognized = 0,
	kCTFontFormatOpenTypePostScript = 1,
	kCTFontFormatOpenTypeTrueType = 2,
	kCTFontFormatTrueType = 3,
	kCTFontFormatPostScript = 4,
	kCTFontFormatBitmap = 5
} CTFontFormat;

/* **M6o: THE DESCRIPTOR'S COPY AND LOOKUP DOORS.** Each is one of the era's sentences:
 *
 *   * `CopyAttribute` — "a retained reference to the requested attribute, or NULL if ... not present";
 *   * `CopyLocalizedAttribute` — "returns a localized attribute based on the global language list. **If
 *     localization is not possible for the attribute the behavior matches `CTFontDescriptorCopyAttribute()`**".
 *     THAT FALLBACK IS WHAT THIS TIER IMPLEMENTS, and it is the era's own answer rather than a stub: the
 *     global language list is a system input this tier does not have, and the era says what to do without it.
 *     `*language` is likewise left NULL, which the contract allows;
 *   * `CreateCopyWithFeature` / `CreateCopyWithVariation` — a copy "modified with the given feature settings"
 *     and one carrying "a new variation instance", whose axis identifier the era describes as "the four
 *     character code of the variation axis";
 *   * `CreateMatchingFontDescriptors` — "an array of font normalized font descriptors matching the attributes
 *     present in descriptor. **If descriptor itself is normalized then the array will contain only one item,
 *     the original descriptor.**" `mandatoryAttributes` is an `NSSet` here where the era takes a `CFSetRef`.
 *
 * `CTFontDescriptorCreateMatchingFontDescriptor` (the SINGLE one) remains owed and is named as such: the era
 * defines its answer as "based on the original descriptor and **system precedence**" — a precedence order this
 * tier does not have, and inventing one would be inventing the surface.
 */
CFTypeRef CTFontDescriptorCopyAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute)
	CG_RETURNS_RETAINED;
CFTypeRef CTFontDescriptorCopyLocalizedAttribute(CTFontDescriptorRef descriptor, CFStringRef attribute,
						     CFStringRef *language) CG_RETURNS_RETAINED;
CTFontDescriptorRef CTFontDescriptorCreateCopyWithFeature(CTFontDescriptorRef original,
							  CFNumberRef featureTypeIdentifier,
							  CFNumberRef featureSelectorIdentifier);
CTFontDescriptorRef CTFontDescriptorCreateCopyWithVariation(CTFontDescriptorRef original,
							    CFNumberRef variationIdentifier,
							    CGFloat variationValue);
CFArrayRef CTFontDescriptorCreateMatchingFontDescriptors(CTFontDescriptorRef descriptor,
						       CFSetRef mandatoryAttributes) CG_RETURNS_RETAINED;

/* **M7l: THE SINGLE PREFERRED MATCH.** Apple's sentence: "Returns an the single preferred matching font
 * descriptor based on the original descriptor and **system precedence**" — which is the PLURAL's answer above,
 * first element, and answering it any other way would be a second matching rule beside the one the matcher
 * already implements. An empty match answers NULL, which is Apple's "the original descriptor may be returned in
 * normalized form" read strictly: there is no normalized form of nothing. */
CTFontDescriptorRef CTFontDescriptorCreateMatchingFontDescriptor(CTFontDescriptorRef descriptor,
								 CFSetRef mandatoryAttributes)
	CG_RETURNS_RETAINED;

#endif /* CORE_TEXT_CTFONTDESCRIPTOR_H */
