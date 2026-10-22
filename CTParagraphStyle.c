/*
 * CTParagraphStyle.c — the paragraph style: a table of fourteen settings, each with its own type and default.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * THE TABLE BELOW IS THE IMPLEMENTATION, AND THE REST OF THIS FILE IS BOOKKEEPING AROUND IT. A setting is a
 * (type, default) pair — Apple prints both beside each constant in its header — and those two facts are what
 * make the three doors answerable: `Create` copies what it is given by SIZE, `GetValueForSpecifier` writes the
 * stored copy or the DEFAULT when nothing was stored, and nothing has to know what a setting MEANS to carry it
 * across. That last part is worth saying plainly, because it is why this file is short: NOTHING HERE LAYS ANYTHING
 * OUT. The framesetter and the typesetter do, and neither exists yet.
 *
 * IMMUTABLE, SO THERE IS NO LOCK AND NO MUTABLE COPY. Apple's own reason for asking for immutability is "totally
 * lock-free operation", and the way to have that is to have nothing that can change: a style is written once, in
 * `Create`, and read thereafter. There is no door that modifies one, which is why the storage needs no guard.
 */
#include <CoreText/CTParagraphStyle.h>

#include <CoreText/CTTextTab.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 * THE TABLE. One row per specifier, in Apple's own order and with Apple's own type and default for each.
 * =============================================================================================== */
/* THE SCALAR DEFAULTS, AS OBJECTS rather than as literals: the table points at them, and a pointer is how a
 * `void *` value is handed out for a value nobody set. They are `static const` and never written. */
static const CGFloat ct_para_zero = 0.0;
static const CTTextAlignment ct_para_alignment_default = kCTNaturalTextAlignment;
static const CTLineBreakMode ct_para_break_default = kCTLineBreakByWordWrapping;
static const CTWritingDirection ct_para_direction_default = kCTWritingDirectionNatural;

struct ct_para_spec {
	size_t size;		/* the type's size, which is how the value is read AND written */
	Boolean cftype;		/* true when the value is a CFTypeRef to RETAIN rather than bytes to copy */
	const void *fallback;	/* the default's address, or NULL when the default is built rather than stored */
};

static const struct ct_para_spec ct_para_specs[kCTParagraphStyleSpecifierCount] = {
	[kCTParagraphStyleSpecifierAlignment] = { sizeof(CTTextAlignment), false, &ct_para_alignment_default },
	[kCTParagraphStyleSpecifierFirstLineHeadIndent] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierHeadIndent] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierTailIndent] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierTabStops] = { sizeof(CFArrayRef), true, NULL },
	[kCTParagraphStyleSpecifierDefaultTabInterval] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierLineBreakMode] = { sizeof(CTLineBreakMode), false, &ct_para_break_default },
	[kCTParagraphStyleSpecifierLineHeightMultiple] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierMaximumLineHeight] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierMinimumLineHeight] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierLineSpacing] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierParagraphSpacing] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierParagraphSpacingBefore] = { sizeof(CGFloat), false, &ct_para_zero },
	[kCTParagraphStyleSpecifierBaseWritingDirection] = { sizeof(CTWritingDirection), false,
							    &ct_para_direction_default }
};

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTParagraphStyle {
	CFRuntimeBase base;
	Boolean set[kCTParagraphStyleSpecifierCount];	/* what a caller actually set */
	void *value[kCTParagraphStyleSpecifierCount];	/* the copy; a RETAINED CFType for the cftype row */
	CFArrayRef tab_fallback;			/* the default for TabStops, built once and borrowed out */
};

static void ct_para_finalize(CFTypeRef cf)
{
	struct __CTParagraphStyle *style = (struct __CTParagraphStyle *)cf;
	int i;

	for (i = 0; i < (int)kCTParagraphStyleSpecifierCount; i++) {
		if (!style->set[i] || style->value[i] == NULL) {
			continue;
		}
		if (ct_para_specs[i].cftype) {
			CFRelease((CFTypeRef)style->value[i]);
		} else {
			free(style->value[i]);
		}
	}
	if (style->tab_fallback != NULL) {
		CFRelease(style->tab_fallback);
	}
}

/* TWO STYLES ARE EQUAL WHEN THEY HOLD THE SAME SETTINGS. A value nobody set is not compared against one that was
 * set to the same number, because "unset" and "set to the default" are the same ANSWER through the accessor and
 * different FACTS about the style — and the caller can see the difference in the accessor's return value. */
static Boolean ct_para_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTParagraphStyle *a = (const struct __CTParagraphStyle *)cf1;
	const struct __CTParagraphStyle *b = (const struct __CTParagraphStyle *)cf2;
	int i;

	for (i = 0; i < (int)kCTParagraphStyleSpecifierCount; i++) {
		if (a->set[i] != b->set[i]) {
			return false;
		}
		if (!a->set[i]) {
			continue;
		}
		if (ct_para_specs[i].cftype) {
			if (!CFEqual((CFTypeRef)a->value[i], (CFTypeRef)b->value[i])) {
				return false;
			}
		} else if (memcmp(a->value[i], b->value[i], ct_para_specs[i].size) != 0) {
			return false;
		}
	}
	return true;
}

static CFHashCode ct_para_hash(CFTypeRef cf)
{
	const struct __CTParagraphStyle *style = (const struct __CTParagraphStyle *)cf;
	CFHashCode h = 17u;
	int i;

	for (i = 0; i < (int)kCTParagraphStyleSpecifierCount; i++) {
		if (!style->set[i]) {
			continue;
		}
		h = h * 31u + (CFHashCode)i;
		if (ct_para_specs[i].cftype) {
			h = h * 31u + CFHash((CFTypeRef)style->value[i]);
		} else {
			/* THE BYTES OF A SCALAR ARE HASHED DIRECTLY, which is what a value type owes: two styles holding
			 * the same number must hash the same, and memcmp/hash agree by construction. */
			const unsigned char *p = style->value[i];
			size_t k;

			for (k = 0; k < ct_para_specs[i].size; k++) {
				h = h * 31u + p[k];
			}
		}
	}
	return h;
}

static const CFRuntimeClass ct_para_class = {
	0,			/* version */
	"CTParagraphStyle",	/* className */
	NULL,			/* init */
	NULL,			/* copy */
	ct_para_finalize,	/* finalize */
	ct_para_equal,		/* equal */
	ct_para_hash,		/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTParagraphStyleGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_para_class);
	}
	return type_id;
}

static struct __CTParagraphStyle *ct_para_alloc(void)
{
	return (struct __CTParagraphStyle *)_CFRuntimeCreateInstance(kCFAllocatorDefault,
		CTParagraphStyleGetTypeID(),
		(CFIndex)(sizeof(struct __CTParagraphStyle) - sizeof(CFRuntimeBase)), NULL);
}

/* ONE SETTING, COPIED. The size must match the specifier's type — Apple's own "must match" — and a setting whose
 * size does not is SKIPPED rather than read as the wrong width, which is the same forward-compatible behaviour
 * an unknown specifier gets: a style with one setting this library cannot read is still a style. */
static void ct_para_store(struct __CTParagraphStyle *style, CTParagraphStyleSpecifier spec, size_t size,
			  const void *value)
{
	const struct ct_para_spec *row;

	if (spec >= (CTParagraphStyleSpecifier)kCTParagraphStyleSpecifierCount || value == NULL) {
		return;
	}
	row = &ct_para_specs[spec];
	if (size != row->size) {
		return;
	}
	if (row->cftype) {
		if (CFGetTypeID(*(CFTypeRef const *)value) == CFArrayGetTypeID()) {
			style->value[spec] = (void *)CFRetain(*(CFTypeRef const *)value);
		} else {
			return;	/* a TabStops value that is not an array is not a TabStops value */
		}
	} else {
		style->value[spec] = malloc(row->size);
		if (style->value[spec] == NULL) {
			return;
		}
		memcpy(style->value[spec], value, row->size);
	}
	style->set[spec] = true;
}

CTParagraphStyleRef CTParagraphStyleCreate(const CTParagraphStyleSetting *settings, CFIndex settingCount)
{
	struct __CTParagraphStyle *style;
	CFIndex i;

	style = ct_para_alloc();
	if (style == NULL) {
		return NULL;
	}
	/* NULL `settings` IS THE DEFAULT SET, and so is a count of zero — Apple's own sentence for the parameter.
	 * Either way the object exists with nothing set, and every accessor answers its default. */
	for (i = 0; settings != NULL && i < settingCount; i++) {
		ct_para_store(style, settings[i].spec, settings[i].valueSize, settings[i].value);
	}
	return (CTParagraphStyleRef)style;
}

CTParagraphStyleRef CTParagraphStyleCreateCopy(CTParagraphStyleRef paragraphStyle)
{
	const struct __CTParagraphStyle *original = (const struct __CTParagraphStyle *)paragraphStyle;
	struct __CTParagraphStyle *copy;
	int i;

	if (original == NULL) {
		return NULL;
	}
	copy = ct_para_alloc();
	if (copy == NULL) {
		return NULL;
	}
	/* A REAL COPY AND NOT A RETAINED SELF, even though an immutable object would make the second legitimate: a
	 * caller that leaks one of two pointers should not be punished for the library's shortcut. */
	for (i = 0; i < (int)kCTParagraphStyleSpecifierCount; i++) {
		if (!original->set[i]) {
			continue;
		}
		if (ct_para_specs[i].cftype) {
			copy->value[i] = (void *)CFRetain((CFTypeRef)original->value[i]);
		} else {
			copy->value[i] = malloc(ct_para_specs[i].size);
			if (copy->value[i] == NULL) {
				continue;
			}
			memcpy(copy->value[i], original->value[i], ct_para_specs[i].size);
		}
		copy->set[i] = true;
	}
	return (CTParagraphStyleRef)copy;
}

/* =================================================================================================
 * ACCESS.
 * =============================================================================================== */
/* THE DEFAULT FOR TabStops, WHICH IS APPLE'S TWELVE TABS, AND THE DEVIATION THIS ROW USED TO CARRY IS PAID. While
 * `CTTextTab.h` was unwritten this row answered an EMPTY ARRAY — a recorded deviation that named its own landing
 * point in this very comment ("the tabs arrive with CTTextTab, and this function is where they will be built").
 * THEY HAVE ARRIVED: Apple's header prints the default as "12 left-aligned tabs, spaced by 28.0 points", so twelve
 * tabs are built here, LEFT-ALIGNED at 28.0, 56.0 … 336.0 — the multiples of 28.0 that Apple's second number names.
 *
 * BUILT ONCE PER STYLE AND BORROWED OUT, because the accessor's contract for a CFArray value is a borrowed
 * reference: a fresh array per call would leak in the caller's hands. */
static CFArrayRef ct_para_tab_fallback(struct __CTParagraphStyle *style)
{
	if (style->tab_fallback != NULL) {
		return style->tab_fallback;
	}
	style->tab_fallback = CFArrayCreateMutable(kCFAllocatorDefault, 12, &kCFTypeArrayCallBacks);
	if (style->tab_fallback == NULL) {
		return NULL;
	}
	{
		int i;

		for (i = 1; i <= 12; i++) {
			CTTextTabRef tab = CTTextTabCreate(kCTLeftTextAlignment, 28.0 * (double)i, NULL);

			if (tab == NULL) {
				/* A TAB THAT COULD NOT BE MADE LEAVES THE ROW SHORTER RATHER THAN LYING ABOUT ITS LENGTH: the
				 * array is what was actually built, and twelve is the answer only when twelve exist. */
				break;
			}
			CFArrayAppendValue((CFMutableArrayRef)style->tab_fallback, tab);
			CFRelease(tab);
		}
	}
	return style->tab_fallback;
}

bool CTParagraphStyleGetValueForSpecifier(CTParagraphStyleRef paragraphStyle, CTParagraphStyleSpecifier spec,
					  size_t valueBufferSize, void *valueBuffer)
{
	struct __CTParagraphStyle *style = (struct __CTParagraphStyle *)paragraphStyle;

	if (style == NULL || valueBuffer == NULL) {
		return false;
	}
	/* AN UNKNOWN SPECIFIER ZEROES THE BUFFER AND ANSWERS FALSE, which is Apple's own sentence: "nothing bad will
	 * happen and the buffer value will simply be zeroed out". */
	if (spec >= (CTParagraphStyleSpecifier)kCTParagraphStyleSpecifierCount) {
		memset(valueBuffer, 0, valueBufferSize);
		return false;
	}
	/* A BUFFER TOO SMALL IS NOT WRITTEN TO AT ALL. Apple requires the size to match the specifier's type, and
	 * writing part of a number would leave the caller with half of one. */
	if (valueBufferSize < ct_para_specs[spec].size) {
		return false;
	}
	if (style->set[spec]) {
		/* A CFType VALUE IS THE POINTER ITSELF AND NOT THE BYTES IT POINTS AT, which is the one place the two
		 * kinds of row differ and the place this door was WRONG until the suite caught it. A scalar row holds the
		 * ADDRESS of the value's bytes, so the value is copied FROM there; a CFType row holds THE OBJECT, so what
		 * the caller receives is that object. Treating the second like the first copies the array's own first
		 * bytes -- which are zero -- and hands the caller a NULL where its tab stops should be. It is invisible
		 * for every scalar, which is why the case that holds a style's TabStops through a release is the one that
		 * finds it. */
		if (ct_para_specs[spec].cftype) {
			*(const void **)valueBuffer = style->value[spec];
		} else {
			memcpy(valueBuffer, style->value[spec], ct_para_specs[spec].size);
		}
		return true;
	}
	/* THE DEFAULT, WHICH IS THE DOOR'S OWN REASON FOR EXISTING: "If the user has not set it, this function will
	 * return the default value." */
	if (ct_para_specs[spec].cftype) {
		*(CFArrayRef *)valueBuffer = ct_para_tab_fallback(style);
		return true;
	}
	memcpy(valueBuffer, ct_para_specs[spec].fallback, ct_para_specs[spec].size);
	return true;
}
