/*
 * CTFontCollection.m — every face the registry can find, as descriptors.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE ENUMERATION IS A CALLBACK INTO `cg_font_enumerate`, AND THAT IS THE WHOLE DESIGN.** A collection
 * could have opened the font directories itself, and then the directories a font may live in would exist in
 * two places — the bug this tree keeps paying for in other forms (one face per font, one list per
 * directory). Instead the CoreGraphics seam walks the FSH list (and `FN_FONT_PATH`'s override) and hands this
 * file one PATH at a time; this file turns each path into a face with CoreGraphics' own doors
 * (`CGDataProviderCreateWithFilename` → `CGFontCreateWithDataProvider`) and then into a descriptor with
 * CoreText's own.
 *
 * **A FACE THAT DOES NOT OPEN IS SKIPPED, NOT REPORTED AS A HOLE** (`CTFontCollection.h` says so): a
 * directory can hold something with a font suffix that is not a font, and a descriptor is what a caller acts
 * on.
 */
#import <Foundation/Foundation.h>

#include "CTFontCollection.h"
#include <CoreText/CTFont.h>
/* **FOR `CTFontDescriptorCreateMatchingFontDescriptors` AND THE TYPE ITS SIGNATURE NAMES** — the door the
 * era's sentence for `CreateWithFontDescriptors` points at, and §41's: trace every call to its header. */
#include <CoreText/CTFontDescriptor.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGFont.h>
#include <CoreGraphics/CGFont_internal.h>

@interface CTFontCollection : NSObject {
	@public
	NSArray *descriptors;	/* the members, RETAINED by the array */
}
@end

@implementation CTFontCollection

- (void)dealloc
{
	[descriptors release];
	[super dealloc];
}

@end

/* THE VISITOR. It is a plain C function because `cg_font_enumerate` is a C door — the seam's own shape. */
static void ct_collect_face(void *info, const char *path)
{
	NSMutableArray *list = (NSMutableArray *)info;
	CGDataProviderRef provider;
	CGFontRef graphics;
	CTFontRef font;
	CTFontDescriptorRef descriptor;

	if (list == nil) {
		return;
	}
	provider = CGDataProviderCreateWithFilename(path);
	if (provider == NULL) {
		return;
	}
	graphics = CGFontCreateWithDataProvider(provider);
	CGDataProviderRelease(provider);
	if (graphics == NULL) {
		return;
	}
	/* THE SIZE IS THE TIER'S DEFAULT AND IS NOT A CLAIM ABOUT THIS FACE: a descriptor carries the NAME, and
	 * `CTFontCopyFontDescriptor` builds one from it (plus a size and a matrix, which are this object's own). */
	font = CTFontCreateWithGraphicsFont(graphics, 12.0, NULL, NULL);
	CGFontRelease(graphics);
	if (font == NULL) {
		return;
	}
	descriptor = CTFontCopyFontDescriptor(font);
	CTFontRelease(font);
	if (descriptor != NULL) {
		[list addObject:(id)descriptor];
		CTFontDescriptorRelease(descriptor);	/* the array holds its own reference */
	}
}

CTFontCollectionRef CTFontCollectionCreateFromAvailableFonts(NSDictionary *options)
{
	NSMutableArray *list = [NSMutableArray array];
	CTFontCollection *collection;

	/* APPKIT-REFUSE: Apple's `options` selects among the SORTS of collection, and this tier has one — the
	 * registry's own list (CTFontCollection.h says so). */
	(void)options;
	cg_font_enumerate(ct_collect_face, list);
	if ([list count] == 0) {
		return NULL;	/* nothing installed is a real state, not an error */
	}
	collection = [[CTFontCollection alloc] init];
	collection->descriptors = [list retain];
	return (CTFontCollectionRef)collection;
}

NSArray *CTFontCollectionCreateMatchingFontDescriptors(CTFontCollectionRef collection)
{
	if (collection == NULL) {
		return nil;
	}
	return [(NSArray *)collection->descriptors retain];
}

CTFontCollectionRef CTFontCollectionRetain(CTFontCollectionRef collection)
{
	return (CTFontCollectionRef)[(id)(uintptr_t)collection retain];
}

void CTFontCollectionRelease(CTFontCollectionRef collection)
{
	[(id)(uintptr_t)collection release];
}

/* ====================================================================================================
 * M6q: THE COLLECTION'S DESCRIPTOR-BASED DOORS.
 * ================================================================================================== */

/* "The contents of this collection is defined by MATCHING the provided descriptors against all available font
 * descriptors" — so each descriptor the caller passes is EXPANDED by the descriptor-matching door §41
 * defined, and the results are the collection. Constructed exactly as `CreateFromAvailableFonts` does. */
CTFontCollectionRef CTFontCollectionCreateWithFontDescriptors(NSArray *descriptors, NSDictionary *options)
{
	NSMutableArray *members = [NSMutableArray array];
	CTFontCollection *collection;

	(void)options;		/* Apple's options dictionary, which this tier has nothing to say about */
	for (NSUInteger i = 0; descriptors != nil && i < [descriptors count]; i++) {
		NSArray *matched = CTFontDescriptorCreateMatchingFontDescriptors(
			(CTFontDescriptorRef)[descriptors objectAtIndex:i], nil);

		if (matched != nil) {
			[members addObjectsFromArray:matched];
		}
	}
	collection = [[CTFontCollection alloc] init];
	collection->descriptors = [members retain];
	return (CTFontCollectionRef)collection;
}

/* "A copy of the original collection AUGMENTED with the new font descriptors" — the union, the original's
 * members first so a copy's order begins with what it copied. */
CTFontCollectionRef CTFontCollectionCreateCopyWithFontDescriptors(CTFontCollectionRef original,
								 NSArray *descriptors, NSDictionary *options)
{
	NSMutableArray *members = [NSMutableArray array];
	CTFontCollectionRef expanded = CTFontCollectionCreateWithFontDescriptors(descriptors, options);
	CTFontCollection *collection;

	if (original != NULL && original->descriptors != nil) {
		[members addObjectsFromArray:(NSArray *)original->descriptors];
	}
	if (expanded != NULL && expanded->descriptors != nil) {
		[members addObjectsFromArray:(NSArray *)expanded->descriptors];
	}
	CTFontCollectionRelease(expanded);
	collection = [[CTFontCollection alloc] init];
	collection->descriptors = [members retain];
	return (CTFontCollectionRef)collection;
}

/* "The array of matching font descriptors SORTED with the callback function" — the callback defines the order,
 * and a caller with no callback to give gets the array unsorted rather than a refusal, because the sorting is
 * the caller's business and an unsorted match is a true answer. */
NSArray *CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(
	CTFontCollectionRef collection, CTFontCollectionSortDescriptorsCallback sortCallback, void *refCon)
{
	NSArray *matched = CTFontCollectionCreateMatchingFontDescriptors(collection);

	if (matched == nil || sortCallback == NULL) {
		return matched;
	}
	{
		NSMutableArray *sorted = [NSMutableArray arrayWithArray:matched];

		/* AN INSERTION SORT, because the era's callback is a C function pointer with no context beyond
		 * `refCon` and this tier has no comparator to hand it to. */
		for (NSUInteger i = 1; i < [sorted count]; i++) {
			id item = [[sorted objectAtIndex:i] retain];
			NSUInteger j = i;

			while (j > 0 && sortCallback((CTFontDescriptorRef)item,
						     (CTFontDescriptorRef)[sorted objectAtIndex:j - 1],
						     refCon) < 0) {
				[sorted replaceObjectAtIndex:j withObject:[sorted objectAtIndex:j - 1]];
				j--;
			}
			[sorted replaceObjectAtIndex:j withObject:item];
			[item release];
		}
		return sorted;
	}
}

