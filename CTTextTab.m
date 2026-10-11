/*
 * CTTextTab.m — the class, with its ivars HERE rather than in a shared header.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE STORAGE BELONGS TO THE OWNER OF THE LAYOUT** (§32's finding, applied before it could bite again):
 * clang emits the scaled ivar-offset globals in the translation unit that declares the class's ivars, and a
 * unit that merely READS another class's ivars asks for a symbol nothing defines. Nothing outside this file
 * needs these fields — the doors below are the whole interface — so the ivars live with the implementation.
 *
 * The object is immutable once created, which is what lets `CTTextTabGetOptions` answer the dictionary the
 * caller passed: the tab holds it, and `dealloc` is the only place it goes back.
 */
#import <Foundation/Foundation.h>

#include "CTTextTab.h"

#include <CoreText/CTParagraphStyle.h>

@interface CTTextTab : NSObject {
	@public
	CTTextAlignment alignment;	/* the era's own type, declared in CTParagraphStyle.h */
	CGFloat location;		/* points, "relative to the BACK margin" */
	NSDictionary *options;		/* RETAINED; may be nil, and the getter answers NULL for that */
}
@end

@implementation CTTextTab

- (void)dealloc
{
	[options release];
	[super dealloc];
}

@end

/* **THE ONE CONSTRUCTOR, WITH APPLE'S OWN PARAMETERS** — "an alignment type and location", the alignment
 * being this tier's `CTTextAlignment` because that is the type 10.6's own declaration takes. */
CTTextTabRef CTTextTabCreate(CTTextAlignment alignment, CGFloat location, NSDictionary *options)
{
	CTTextTab *tab = [[CTTextTab alloc] init];

	tab->alignment = alignment;
	tab->location = location;
	tab->options = [options retain];
	return tab;
}

CTTextTabRef CTTextTabRetain(CTTextTabRef tab)
{
	return (CTTextTabRef)[(id)tab retain];
}

void CTTextTabRelease(CTTextTabRef tab)
{
	[(id)tab release];
}

CTTextAlignment CTTextTabGetAlignment(CTTextTabRef tab)
{
	/* "The tab's text alignment value" — and a NULL tab has none, so the era's own neutral value is the
	 * honest answer rather than a zero that a reader would take for `kCTLeftTextAlignment`. */
	return (tab == NULL) ? kCTNaturalTextAlignment : tab->alignment;
}

CGFloat CTTextTabGetLocation(CTTextTabRef tab)
{
	return (tab == NULL) ? 0.0 : tab->location;
}

/* "The dictionary of attributes associated with the tab **or NULL if**" there are none — the era's own
 * sentence, and the reason this door answers NULL rather than an empty dictionary. */
NSDictionary *CTTextTabGetOptions(CTTextTabRef tab)
{
	return (tab == NULL) ? NULL : tab->options;
}

/* THE OPTION KEY THIS CLASS EXISTS TO CARRY, valued as every key in this tier is. */
NSString *const kCTTabColumnTerminatorsAttributeName = @"CTTabColumnTerminators";
