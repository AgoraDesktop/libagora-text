/*
 * CTFontCollection.h — the collection: a QUERY over the faces a system has, and the type a caller reaches for
 * to ask what is installed.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). It includes CTFontDescriptor.h and nothing else: a
 * collection is made OF descriptors, and Apple's header for it declares one opaque type, one type id, one
 * callback, one option key and five doors.
 *
 * A COLLECTION IS A QUERY AND NOT A LIST, WHICH IS THE WHOLE SHAPE OF THIS HEADER. The caller does not build
 * a collection out of fonts it already has; it states WHAT IT IS LOOKING FOR — an array of descriptors, or
 * nothing at all for "whatever this system has" — and the collection ANSWERS by matching that against every
 * face the system can produce. Apple says so twice, once per creation door: the descriptors-based one "is
 * defined by matching the provided descriptors against all available font descriptors", and the copy door
 * "merges the new font descriptors with the existing descriptors to create a single set". So the descriptors
 * a collection holds are what it is ASKED, and its answer is a separate door.
 *
 * THE ANSWER IS THE PART THIS TIER OWES. Both matching doors are declared here and NOT DEFINED, because
 * Apple's answer is not the set of faces that match — it is "an array of NORMALIZED font descriptors matching
 * the collection definition", and BOTH halves of that sentence are Apple's and not a signature's: which
 * attributes a returned descriptor carries, and in what order the array arrives. A registry of installed
 * faces exists under this library (`cg_font_enumerate` walks the configured directories and the process's
 * registrations), so the SET is reachable — it is the normalization and the ordering that would have to be
 * invented, and an invented order is precisely the kind of answer this tree refuses to give. The two doors
 * therefore land with the tier that owns matching, exactly as `CTFontDescriptorCreateMatchingFontDescriptors`
 * waits for it.
 *
 * THE ONE OPTION KEY IS AN INFERENCE, AND THE MEASUREMENT IS WORTH RECORDING BECAUSE IT IS NEGATIVE. Apple's
 * header declares `kCTFontCollectionRemoveDuplicatesOption` and publishes no value for it. The rule that
 * proves the other keys — the AppKit declaring the same name with CT spelled NS — CANNOT BE APPLIED HERE, and
 * that was measured rather than assumed: AT 10.6 THE APPKIT SHIPS NO `NSFontCollection.h` AT ALL (its font
 * headers are `NSFont.h`, `NSFontDescriptor.h`, `NSFontManager.h` and `NSFontPanel.h`), so `NSFontCollection`
 * is post-10.6 and this key has no counterpart in the era to pair with. Its value below is therefore recorded
 * as the same inference the other unprovable keys carry, and it is the first thing to check the day something
 * can answer for it.
 */
#ifndef CORE_TEXT_CTFONTCOLLECTION_H
#define CORE_TEXT_CTFONTCOLLECTION_H

#include <CoreText/CTFontDescriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE COLLECTION. Opaque, and a Core Foundation object like the descriptor: its identity comes from the
 * runtime, so `CFGetTypeID` of one IS `CTFontCollectionGetTypeID()`. */
typedef const struct __CTFontCollection *CTFontCollectionRef;

CFTypeID CTFontCollectionGetTypeID(void);

/* HOW A SORTED ANSWER IS ORDERED, and it is the CALLER's comparison rather than ours — which is the workaround
 * this header offers for the one thing a matching door cannot know: which of two faces a caller wants first.
 * Apple's own words for the parameter are that it "defines the sort order". */
typedef CFComparisonResult (*CTFontCollectionSortDescriptorsCallback)(CTFontDescriptorRef first,
								     CTFontDescriptorRef second, void *refCon);

/* ---------------------------------------------------------------------------------------
 * THE OPTIONS, WHICH ARE THE CALLER'S AND ARE RECORDED RATHER THAN OBEYED. Apple gives this one key exactly
 * one meaning: "specify this option key in the options dictionary with a non-zero value to enable automatic
 * filtering of duplicate font descriptors". Filtering duplicates is a property of MATCHING, which is a door
 * this tier owes (see this file's head), so a collection keeps the options it was handed and the door that
 * would apply them is the one that is not defined. THE VALUE IS THE INFERENCE THIS FILE'S HEAD DESCRIBES.
 * ------------------------------------------------------------------------------------- */
extern const CFStringRef kCTFontCollectionRemoveDuplicatesOption;

/* ---------------------------------------------------------------------------------------
 * CREATION. Three doors make a collection and one copies it — and no door hands its descriptors back, because
 * a collection's descriptors ARE its question and its answer comes from matching.
 * ------------------------------------------------------------------------------------- */
/* EVERY FACE THE SYSTEM HAS, with `options` as above. This is the door a caller uses to enumerate what is
 * installed: its question is "all of them", and its answer is the matching door's. NULL options are Apple's
 * "no options", not an error. */
CTFontCollectionRef CTFontCollectionCreateFromAvailableFonts(CFDictionaryRef options);

/* A COLLECTION THAT ASKS A QUESTION: the array is the descriptors to match against every available face, so a
 * collection made from one descriptor asks about one face and not about one font. An EMPTY array is a
 * question with no conditions, which is a different thing from "all available" only in Apple's answer to it,
 * and both are legal. */
CTFontCollectionRef CTFontCollectionCreateWithFontDescriptors(CFArrayRef descriptors,
							      CFDictionaryRef options);

/* THE COPY MERGES RATHER THAN REPLACES: Apple's sentence is that the new descriptors "are merged with the
 * existing descriptors to create a single set", so the copy asks the original's question AND the new one. */
CTFontCollectionRef CTFontCollectionCreateCopyWithFontDescriptors(CTFontCollectionRef original,
								  CFArrayRef descriptors,
								  CFDictionaryRef options);

/* ---------------------------------------------------------------------------------------
 * THE ANSWER, WHICH ITS REASON NO LONGER HOLDS FOR (see this file's head and `CTFontCollection.c`): the tier that
 * owns matching exists, so the SET is answered and the ORDER is a recorded decision — by family name, stable and
 * reproducible. The sorted door is Apple's own way out of it: the callback "defines the sort order".
 * ------------------------------------------------------------------------------------- */
CFArrayRef CTFontCollectionCreateMatchingFontDescriptors(CTFontCollectionRef collection);
CFArrayRef CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(
	CTFontCollectionRef collection, CTFontCollectionSortDescriptorsCallback sortCallback, void *refCon);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTCOLLECTION_H */
