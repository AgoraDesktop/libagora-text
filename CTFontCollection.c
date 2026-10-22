/*
 * CTFontCollection.c — the collection: its identity, its four creators, and the two doors it does not answer.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * A COLLECTION HOLDS A QUESTION AND NOT AN ANSWER, which is why this file is short and why the two doors that
 * would be long are owed instead. What a collection owns is the descriptors it was asked about, the options
 * it was handed, and which of Apple's two questions it is. A matching door would take those and look at every
 * face the system can produce; that door is not defined here (see the owed block at this file's end).
 *
 * THE OBJECT RECORDS WHAT IT WAS HANDED RATHER THAN INTERPRETING IT. Apple's copy door says the new
 * descriptors "are merged with the existing descriptors to create a single set", which is unambiguous when the
 * original holds a set — and is written for THAT case, because an available-fonts collection holds no
 * descriptors to merge with. Rather than guess what merging means against "everything", the copy keeps the
 * original's question, appends the new descriptors, and leaves the meaning of the combination to the matching
 * door that owns it. Nothing the caller passed is dropped, and nothing is invented.
 */
#include <CoreText/CTFontCollection.h>

#include <CoreText/CTFontMatching_internal.h>

#include <CoreFoundation/CFRuntime.h>

/* THE ONE OPTION KEY, WHOSE VALUE IS THE INFERENCE CTFontCollection.h's head describes in full: Apple
 * publishes no value for it, and the rule that proves the other keys CANNOT be applied — MEASURED, the AppKit
 * at 10.6 ships no `NSFontCollection.h` at all, so there is no `NSFont…` counterpart in the era to pair this
 * with. */
const CFStringRef kCTFontCollectionRemoveDuplicatesOption = CFSTR("NSFontCollectionRemoveDuplicatesOption");

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTFontCollection {
	CFRuntimeBase base;
	CFArrayRef descriptors;		/* RETAINED: what this collection ASKS; NULL when it asks for every face */
	CFDictionaryRef options;	/* RETAINED: the caller's options, recorded rather than obeyed */
	Boolean all_available;		/* which of Apple's two questions this collection is */
};

static void ct_collection_finalize(CFTypeRef cf)
{
	struct __CTFontCollection *collection = (struct __CTFontCollection *)cf;

	if (collection->descriptors != NULL) {
		CFRelease(collection->descriptors);
	}
	if (collection->options != NULL) {
		CFRelease(collection->options);
	}
}

/* A COLLECTION'S VALUE IS ITS QUESTION AND ITS OPTIONS. There is no accumulated state to compare: a collection
 * never learns anything, because the answer is not stored on it. */
static Boolean ct_collection_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTFontCollection *a = (const struct __CTFontCollection *)cf1;
	const struct __CTFontCollection *b = (const struct __CTFontCollection *)cf2;

	if (a->all_available != b->all_available) {
		return false;
	}
	if ((a->descriptors == NULL) != (b->descriptors == NULL)) {
		return false;
	}
	if (a->descriptors != NULL && !CFEqual(a->descriptors, b->descriptors)) {
		return false;
	}
	if ((a->options == NULL) != (b->options == NULL)) {
		return false;
	}
	if (a->options != NULL && !CFEqual(a->options, b->options)) {
		return false;
	}
	return true;
}

static CFHashCode ct_collection_hash(CFTypeRef cf)
{
	const struct __CTFontCollection *collection = (const struct __CTFontCollection *)cf;
	CFHashCode h = collection->all_available ? 1u : 2u;

	if (collection->descriptors != NULL) {
		h = h * 31u + CFHash(collection->descriptors);
	}
	if (collection->options != NULL) {
		h = h * 31u + CFHash(collection->options);
	}
	return h;
}

static const CFRuntimeClass ct_collection_class = {
	0,			/* version */
	"CTFontCollection",	/* className */
	NULL,			/* init */
	NULL,			/* copy */
	ct_collection_finalize,	/* finalize */
	ct_collection_equal,	/* equal */
	ct_collection_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTFontCollectionGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_collection_class);
	}
	return type_id;
}

/* THE ONE PLACE THAT MAKES A COLLECTION, so nothing else has to remember what one owns. It takes what it will
 * hold and RETAINS it; the caller keeps whatever it had. */
static struct __CTFontCollection *ct_collection_alloc(Boolean all_available, CFArrayRef descriptors,
						      CFDictionaryRef options)
{
	struct __CTFontCollection *collection;

	collection = (struct __CTFontCollection *)_CFRuntimeCreateInstance(kCFAllocatorDefault,
		CTFontCollectionGetTypeID(),
		(CFIndex)(sizeof(struct __CTFontCollection) - sizeof(CFRuntimeBase)), NULL);
	if (collection == NULL) {
		return NULL;
	}
	collection->descriptors = (descriptors == NULL) ? NULL : (CFArrayRef)CFRetain(descriptors);
	collection->options = (options == NULL) ? NULL : (CFDictionaryRef)CFRetain(options);
	collection->all_available = all_available;
	return collection;
}

/* =================================================================================================
 * CREATION.
 * =============================================================================================== */
CTFontCollectionRef CTFontCollectionCreateFromAvailableFonts(CFDictionaryRef options)
{
	/* NO DESCRIPTORS, WHICH IS THE POINT: "every face this system has" is a question with no conditions, so
	 * the descriptor list is NULL rather than a list of everything. Building that list here would be ANSWERING
	 * the question, and answering is the matching door's job -- which is owed, so this door stays a question. */
	return (CTFontCollectionRef)ct_collection_alloc(true, NULL, options);
}

CTFontCollectionRef CTFontCollectionCreateWithFontDescriptors(CFArrayRef descriptors, CFDictionaryRef options)
{
	/* NULL AND AN EMPTY ARRAY ARE DIFFERENT QUESTIONS AND ARE KEPT APART: NULL is the from-available shape,
	 * an empty array is a question with no conditions. They compare unequal below, which is what keeps the two
	 * apart rather than this comment alone. */
	return (CTFontCollectionRef)ct_collection_alloc(false, descriptors, options);
}

/* THE MERGE, WHICH IS APPLE'S WORD AND IS EXPLAINED AT THIS FILE'S HEAD: the result carries the original's
 * question, the original's descriptors and then the new ones, and the options the caller passed this time. */
CTFontCollectionRef CTFontCollectionCreateCopyWithFontDescriptors(CTFontCollectionRef original,
								  CFArrayRef descriptors,
								  CFDictionaryRef options)
{
	const struct __CTFontCollection *o = (const struct __CTFontCollection *)original;
	CFMutableArrayRef merged;
	CFIndex i, count;
	struct __CTFontCollection *copy;

	if (o == NULL) {
		return NULL;
	}
	if (descriptors == NULL) {
		/* NOTHING TO MERGE: a copy that names no descriptors is the original, with the caller's options. */
		return (CTFontCollectionRef)ct_collection_alloc(o->all_available, o->descriptors,
							       (options != NULL) ? options : o->options);
	}
	merged = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (merged == NULL) {
		return NULL;
	}
	/* THE ORIGINAL'S DESCRIPTORS FIRST, THEN THE NEW ONES -- the order Apple's sentence names them in, and
	 * the order a caller who reads that sentence back expects. */
	if (o->descriptors != NULL) {
		count = CFArrayGetCount(o->descriptors);
		for (i = 0; i < count; i++) {
			CFArrayAppendValue(merged, CFArrayGetValueAtIndex(o->descriptors, i));
		}
	}
	count = CFArrayGetCount(descriptors);
	for (i = 0; i < count; i++) {
		CFArrayAppendValue(merged, CFArrayGetValueAtIndex(descriptors, i));
	}
	copy = ct_collection_alloc(o->all_available, merged, (options != NULL) ? options : o->options);
	CFRelease(merged);
	return (CTFontCollectionRef)copy;
}

/* =================================================================================================
 * THE ANSWER, WHICH USED TO BE OWED AND NOW HAS A TIER BEHIND IT.
 *
 * THE OWED REASON SAID, exactly: "THE SET IS REACHABLE AND THE ORDER IS NOT ... THIS TIER DOES NOT INVENT AN ORDER,
 * so these two wait for the tier that owns matching, which is the tier CTFontDescriptorCreateMatchingFontDescriptors
 * waits for." THAT TIER EXISTS NOW (`CTFontMatching.c`), and the user decided the order's question rather than this
 * file inventing one: the array is sorted by FAMILY NAME with the comparator `CTFontManager` offers, WHICH IS STABLE
 * AND REPRODUCIBLE and is itself a recorded deviation from Apple's UI order. `CTFontMatching_internal.h` carries the
 * decision; this file carries only the translation from a COLLECTION to the matcher's question.
 *
 * AND THE SECOND DOOR IS APPLE'S OWN WAY OUT OF THE ORDERING PROBLEM, WHICH IS WHY IT IS THE ONE THAT HANDS THE
 * ORDER TO THE CALLER: `CTFontCollectionSortDescriptorsCallback` "defines the sort order", so a caller who does not
 * like this tier's has one of their own.
 * =============================================================================================== */
CFArrayRef CTFontCollectionCreateMatchingFontDescriptors(CTFontCollectionRef collection)
{
	const struct __CTFontCollection *c = (const struct __CTFontCollection *)collection;

	if (c == NULL) {
		return NULL;
	}
	/* THE COLLECTION'S QUESTION AND THE MATCHER'S ARE THE SAME QUESTION: `descriptors` is the array of query
	 * descriptors, and NULL means "every face" — which is exactly what an empty array means to the matcher. */
	return CTFontMatchingCopyMatches(c->descriptors);
}

CFArrayRef CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(
	CTFontCollectionRef collection, CTFontCollectionSortDescriptorsCallback sortCallback, void *refCon)
{
	CFMutableArrayRef sorted;
	CFArrayRef matches;

	matches = CTFontCollectionCreateMatchingFontDescriptors(collection);
	if (matches == NULL) {
		return NULL;
	}
	/* NO CALLBACK IS NOT AN ERROR HERE, AND IT IS RECORDED: Apple's parameter list says the callback "defines the
	 * sort order", and a caller who passes none gets this tier's own order rather than a refusal — the array is
	 * already ordered, and refusing to hand it back would be a failure to answer a question that has an answer. */
	if (sortCallback == NULL) {
		return matches;
	}
	sorted = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, matches);
	CFRelease(matches);
	if (sorted == NULL) {
		return NULL;
	}
	CFArraySortValues(sorted, CFRangeMake(0, CFArrayGetCount(sorted)), (CFComparatorFunction)sortCallback, refCon);
	return sorted;
}
