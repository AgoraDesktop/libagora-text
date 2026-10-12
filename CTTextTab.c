/*
 * CTTextTab.c — the tab: three values and the object that holds them.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE SHORTEST OBJECT IN THIS LIBRARY, AND THE ONE THAT RETIRES A DEVIATION: its three fields are what Apple's creator
 * takes, and the reason it exists at all is that `kCTParagraphStyleSpecifierTabStops`'s DEFAULT is twelve of them.
 * Compare one to another is comparing those three fields, which is what equality does below — and THE ALIGNMENT IS
 * COMPARED AS A `CTTextAlignment` rather than as bytes, because that is how the vocabulary is defined.
 */
#include <CoreText/CTTextTab.h>

#include <CoreFoundation/CFRuntime.h>

#include <string.h>

struct __CTTextTab {
	CFRuntimeBase base;
	CTTextAlignment alignment;
	double location;
	CFDictionaryRef options;	/* RETAINED: given, kept and never read (see CTTextTab.h) */
};

static void ct_tab_finalize(CFTypeRef cf)
{
	struct __CTTextTab *tab = (struct __CTTextTab *)cf;

	if (tab->options != NULL) {
		CFRelease(tab->options);
	}
}

/* TWO TABS ARE EQUAL WHEN THEIR ALIGNMENT AND LOCATION MATCH: the options are a dictionary Apple names no key for, so
 * they are part of what a tab HOLDS but not of what a tab IS here — recorded rather than silently ignored, because a
 * caller comparing two tabs with different options gets the same answer either way. */
static Boolean ct_tab_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTTextTab *a = (const struct __CTTextTab *)cf1;
	const struct __CTTextTab *b = (const struct __CTTextTab *)cf2;

	return a->alignment == b->alignment && a->location == b->location;
}

static CFHashCode ct_tab_hash(CFTypeRef cf)
{
	const struct __CTTextTab *tab = (const struct __CTTextTab *)cf;

	return (CFHashCode)tab->alignment * 31u + (CFHashCode)tab->location;
}

static const CFRuntimeClass ct_tab_class = {
	0,		/* version */
	"CTTextTab",	/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_tab_finalize,/* finalize */
	ct_tab_equal,	/* equal */
	ct_tab_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTTextTabGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_tab_class);
	}
	return type_id;
}

CTTextTabRef CTTextTabCreate(CTTextAlignment alignment, double location, CFDictionaryRef options)
{
	struct __CTTextTab *tab;

	tab = (struct __CTTextTab *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTTextTabGetTypeID(),
		(CFIndex)(sizeof(struct __CTTextTab) - sizeof(CFRuntimeBase)), NULL);
	if (tab == NULL) {
		return NULL;
	}
	tab->alignment = alignment;
	tab->location = location;
	tab->options = (options == NULL) ? NULL : (CFDictionaryRef)CFRetain(options);
	return (CTTextTabRef)tab;
}

CTTextAlignment CTTextTabGetAlignment(CTTextTabRef tab)
{
	const struct __CTTextTab *t = (const struct __CTTextTab *)tab;

	/* AN INVALID TAB ANSWERS `kCTNaturalTextAlignment`, which is the value a paragraph style uses for "not said" —
	 * not a zero that would read as LEFT. */
	return (t == NULL) ? kCTNaturalTextAlignment : t->alignment;
}

double CTTextTabGetLocation(CTTextTabRef tab)
{
	const struct __CTTextTab *t = (const struct __CTTextTab *)tab;

	return (t == NULL) ? 0.0 : t->location;
}

CFDictionaryRef CTTextTabGetOptions(CTTextTabRef tab)
{
	const struct __CTTextTab *t = (const struct __CTTextTab *)tab;

	return (t == NULL) ? NULL : t->options;
}
