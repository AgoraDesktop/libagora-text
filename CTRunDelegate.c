/*
 * CTRunDelegate.c — the box's three answers, and the Core Foundation object that carries them.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * THE OBJECT IS A CALLBACK STRUCT AND A POINTER, which is the whole of it: `CTRunDelegateCreate` copies the struct,
 * keeps the reference constant, and every door afterward is a call through one of the three function pointers. There
 * is no state to finalize and no memory of its own, so the finalizer does nothing and the two internal accessors
 * (`CTRunDelegateWidth` and its two siblings) are the same call the engine makes while laying a line out.
 */
#include <CoreText/CTRunDelegate.h>
#include <CoreText/CTRunDelegate_internal.h>

#include <CoreFoundation/CFRuntime.h>

#include <stdint.h>

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTRunDelegate {
	CFRuntimeBase base;
	CTRunDelegateCallbacks callbacks;
	void *refCon;
};

static void ct_rd_finalize(CFTypeRef cf)
{
	(void)cf;	/* nothing this object owns is heap-allocated; the refCon belongs to the caller */
}

/* TWO DELEGATES ARE EQUAL WHEN THEY WERE MADE WITH THE SAME CALLBACKS AND THE SAME REFERENCE — the same rule a run's
 * equality uses, and for the same reason: the answers are a function of those. */
static Boolean ct_rd_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTRunDelegate *a = (const struct __CTRunDelegate *)cf1;
	const struct __CTRunDelegate *b = (const struct __CTRunDelegate *)cf2;

	return a->refCon == b->refCon
		&& a->callbacks.getAscent == b->callbacks.getAscent
		&& a->callbacks.getDescent == b->callbacks.getDescent
		&& a->callbacks.getWidth == b->callbacks.getWidth;
}

static CFHashCode ct_rd_hash(CFTypeRef cf)
{
	const struct __CTRunDelegate *d = (const struct __CTRunDelegate *)cf;
	CFHashCode h = 17u;

	h = h * 31u + (CFHashCode)(uintptr_t)d->refCon;
	h = h * 31u + (CFHashCode)(uintptr_t)d->callbacks.getWidth;
	return h;
}

static const CFRuntimeClass ct_rd_class = {
	0,		/* version */
	"CTRunDelegate",/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_rd_finalize,	/* finalize */
	ct_rd_equal,	/* equal */
	ct_rd_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTRunDelegateGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_rd_class);
	}
	return type_id;
}

/* =================================================================================================
 * MAKING ONE.
 * =============================================================================================== */
CTRunDelegateRef CTRunDelegateCreate(const CTRunDelegateCallbacks *callbacks, void *refCon)
{
	struct __CTRunDelegate *delegate;

	/* A NULL CALLBACKS POINTER IS REFUSED, because a delegate with no answers is not a delegate; a missing
	 * individual callback is kept as NULL and answered as zero by the accessors, which is a box of no width. */
	if (callbacks == NULL) {
		return NULL;
	}
	delegate = (struct __CTRunDelegate *)_CFRuntimeCreateInstance(kCFAllocatorDefault,
			CTRunDelegateGetTypeID(),
			(CFIndex)(sizeof(struct __CTRunDelegate) - sizeof(CFRuntimeBase)), NULL);
	if (delegate == NULL) {
		return NULL;
	}
	delegate->callbacks = *callbacks;	/* THE STRUCT IS COPIED; what its pointers point at is the caller's */
	delegate->refCon = refCon;
	return (CTRunDelegateRef)delegate;
}

void *CTRunDelegateGetRefCon(CTRunDelegateRef runDelegate)
{
	const struct __CTRunDelegate *d = (const struct __CTRunDelegate *)runDelegate;

	return (d == NULL) ? NULL : d->refCon;
}

/* =================================================================================================
 * THE THREE ANSWERS, FOR THE ENGINE.
 * =============================================================================================== */
CGFloat CTRunDelegateWidth(CTRunDelegateRef runDelegate)
{
	const struct __CTRunDelegate *d = (const struct __CTRunDelegate *)runDelegate;

	if (d == NULL || d->callbacks.getWidth == NULL) {
		return 0.0;
	}
	return d->callbacks.getWidth(d->refCon);
}

CGFloat CTRunDelegateAscent(CTRunDelegateRef runDelegate)
{
	const struct __CTRunDelegate *d = (const struct __CTRunDelegate *)runDelegate;

	if (d == NULL || d->callbacks.getAscent == NULL) {
		return 0.0;
	}
	return d->callbacks.getAscent(d->refCon);
}

CGFloat CTRunDelegateDescent(CTRunDelegateRef runDelegate)
{
	const struct __CTRunDelegate *d = (const struct __CTRunDelegate *)runDelegate;

	if (d == NULL || d->callbacks.getDescent == NULL) {
		return 0.0;
	}
	return d->callbacks.getDescent(d->refCon);
}
