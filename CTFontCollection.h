/*
 * CTFontCollection.h — the fonts that exist: enumerated, and matched.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A COLLECTION IS A SET OF FONT DESCRIPTORS, AND THE INTERESTING PART IS WHERE ITS MEMBERS COME FROM.**
 * `CTFontCollectionCreateFromAvailableFonts` means "every font this system has", and in this tree that is
 * **THE SAME PLACE THE REGISTRY SEARCHES**: the FSH font directories (and `FN_FONT_PATH`'s override), walked
 * through `cg_font_enumerate` — the CoreGraphics seam door whose whole reason for existing is that the
 * directory list must live in one place. So a font a caller can OPEN BY NAME is a font a collection
 * reports, and vice versa, without a second registry to keep in step.
 *
 * **THE PROOF THIS MILESTONE ASKS FOR IS EXACTLY THAT AGREEMENT**: "the shipped DejaVu faces enumerated by
 * family name, through the SAME registry `CGFontCreateWithFontName` uses" — so the probe asks the collection
 * for its descriptors and then asks the *registry* to open each one by name, and asserts they are the same
 * faces rather than merely a plausible list.
 *
 * WHAT IS DELIBERATELY ABSENT, EACH NAMED: the two SORTED matching doors
 * (`…SortedWithCallback`, `…SortedByFamilyNameWithCallback`) take a C callback plus a `CFStringRef` context
 * — a different calling convention from this tier's own (no CoreFoundation), and a unit of its own;
 * `CTFontCollectionCreateWithFontDescriptors` and `…CreateCopyWithFontDescriptors` are the *modelling* half
 * (a collection the caller assembles rather than one the system answers) and arrive with the descriptor
 * matcher that gives them a purpose; and `CTFontCollectionGetTypeID` is a `CFTypeID`.
 */
#ifndef CORE_TEXT_CTFONTCOLLECTION_H
#define CORE_TEXT_CTFONTCOLLECTION_H

#include <CoreGraphics/CGBase.h>
#include <CoreText/CTFontDescriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTFontCollection;
#else
typedef struct objc_object CTFontCollection;
#endif
typedef CTFontCollection *CTFontCollectionRef;

/* **EVERY AVAILABLE FONT.** Apple's `options` dictionary selects among the SORTS of collection (all fonts,
 * only the available ones, and so on) and is ACCEPTED AND IGNORED here, because this tier's answer to "what
 * fonts exist" is the registry's own list and there is no second category to choose from — a named refusal in
 * the shape this codebase uses. NULL is returned when the enumeration found nothing, which is a real state
 * (a system with no fonts installed) rather than an error. */
CTFontCollectionRef CTFontCollectionCreateFromAvailableFonts(NSDictionary *options);

/* THE MEMBERS, AS DESCRIPTORS, RETAINED (+1): one per face the enumeration found, in the order the
 * directories are searched. A face that cannot be opened is SKIPPED rather than reported as a nil member,
 * because a descriptor is what a caller acts on and a list with holes in it is a list that has to be
 * filtered twice. */
NSArray *CTFontCollectionCreateMatchingFontDescriptors(CTFontCollectionRef collection) CG_RETURNS_RETAINED;

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S. No ledger row is claimed for it. */
CTFontCollectionRef CTFontCollectionRetain(CTFontCollectionRef collection);
void CTFontCollectionRelease(CTFontCollectionRef collection);


/* **M6q: THE COLLECTION'S DESCRIPTOR-BASED DOORS**, the model half the note above named as owed. The era's
 * sentences: `CreateWithFontDescriptors` defines its contents by "MATCHING the provided descriptors against
 * all available font descriptors" — which is `CTFontDescriptorCreateMatchingFontDescriptors`, the door §41
 * defined; `CreateCopyWithFontDescriptors` answers "a copy of the original collection AUGMENTED with the new
 * font descriptors"; and `CreateMatchingFontDescriptorsSortedWithCallback` answers "the array of matching font
 * descriptors SORTED with the callback function", whose order the caller defines.
 *
 * `options` is an `NSDictionary` here where the era takes a `CFDictionaryRef`, and the sort callback is a
 * plain C function pointer of the era's own shape.
 *
 * **`CTFontCollectionGetTypeID` IS DECLARED NOW** (CT-M7g): this sentence said it "is not declared, as
 * elsewhere", and the argument that makes that wrong for every one of these doors is in `CTTypeID_internal.h`. */
CFTypeID CTFontCollectionGetTypeID(void);
typedef int (*CTFontCollectionSortDescriptorsCallback)(CTFontDescriptorRef first, CTFontDescriptorRef second,
						      void *refCon);
CTFontCollectionRef CTFontCollectionCreateWithFontDescriptors(NSArray *descriptors, NSDictionary *options);
CTFontCollectionRef CTFontCollectionCreateCopyWithFontDescriptors(CTFontCollectionRef original,
								 NSArray *descriptors,
								 NSDictionary *options);
NSArray *CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(
	CTFontCollectionRef collection, CTFontCollectionSortDescriptorsCallback sortCallback, void *refCon)
	CG_RETURNS_RETAINED;

#ifdef __cplusplus
}
#endif
#endif /* CORE_TEXT_CTFONTCOLLECTION_H */
