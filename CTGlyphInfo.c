/*
 * CTGlyphInfo.c — the glyph substitution: three creators, three getters, and one boundary.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * THE OBJECT KEEPS WHAT IT WAS GIVEN AND RESOLVES WHAT IT CAN. A glyph info made from a NAME keeps the name and the
 * glyph the face answered for it; one made from an INDEX keeps the index and RESOLVES THE NAME LAZILY through the
 * engine's own glyph-name door, because that door exists at the seam and answering NULL while it does would be a
 * shortage rather than a boundary. AND ONE MADE FROM A CID KEEPS THE CID AND THE COLLECTION AND HAS NO GLYPH AT ALL,
 * which is the boundary this file records: a CID is a key into a COLLECTION's CMap, and a collection is a mapping
 * table this tier does not have — the same standing the character set, the platform font and the vertical metrics have.
 */
#include <CoreText/CTGlyphInfo.h>

#include <CoreGraphics/CGFont_internal.h>

#include <CoreFoundation/CFRuntime.h>

#include <string.h>

struct __CTGlyphInfo {
	CFRuntimeBase base;
	CTFontRef font;			/* RETAINED: what a name was resolved in, or NULL for a CID */
	CFStringRef baseString;		/* RETAINED: the character the substitution stands for */
	CFStringRef glyphName;		/* RETAINED when known, or filled in LAZILY by the getter */
	CGGlyph glyph;
	CGFontIndex cid;		/* the CID, when the object was made from one */
	CTCharacterCollection collection;
	Boolean has_cid;
};

static void ct_glyph_info_finalize(CFTypeRef cf)
{
	struct __CTGlyphInfo *gi = (struct __CTGlyphInfo *)cf;

	if (gi->font != NULL) {
		CFRelease(gi->font);
	}
	if (gi->baseString != NULL) {
		CFRelease(gi->baseString);
	}
	if (gi->glyphName != NULL) {
		CFRelease(gi->glyphName);
	}
}

static Boolean ct_glyph_info_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTGlyphInfo *a = (const struct __CTGlyphInfo *)cf1;
	const struct __CTGlyphInfo *b = (const struct __CTGlyphInfo *)cf2;

	if (a->has_cid != b->has_cid) {
		return false;
	}
	if (a->has_cid) {
		return a->cid == b->cid && a->collection == b->collection;
	}
	return a->glyph == b->glyph && a->font == b->font;
}

static CFHashCode ct_glyph_info_hash(CFTypeRef cf)
{
	const struct __CTGlyphInfo *gi = (const struct __CTGlyphInfo *)cf;

	if (gi->has_cid) {
		return (CFHashCode)gi->cid * 31u + (CFHashCode)gi->collection;
	}
	return (CFHashCode)gi->glyph * 31u + (CFHashCode)(uintptr_t)gi->font;
}

static const CFRuntimeClass ct_glyph_info_class = {
	0,		/* version */
	"CTGlyphInfo",	/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_glyph_info_finalize,	/* finalize */
	ct_glyph_info_equal,	/* equal */
	ct_glyph_info_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTGlyphInfoGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_glyph_info_class);
	}
	return type_id;
}

/* THE ONE PLACE ONE IS MADE, so that the three creators below differ only in what they can resolve. */
static struct __CTGlyphInfo *ct_glyph_info_alloc(CTFontRef font, CFStringRef baseString, CFStringRef glyphName)
{
	struct __CTGlyphInfo *gi;

	gi = (struct __CTGlyphInfo *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTGlyphInfoGetTypeID(),
		(CFIndex)(sizeof(struct __CTGlyphInfo) - sizeof(CFRuntimeBase)), NULL);
	if (gi == NULL) {
		return NULL;
	}
	gi->font = (font == NULL) ? NULL : (CTFontRef)CFRetain(font);
	gi->baseString = (baseString == NULL) ? NULL : (CFStringRef)CFRetain(baseString);
	gi->glyphName = (glyphName == NULL) ? NULL : (CFStringRef)CFRetain(glyphName);
	gi->collection = kCTIdentityMappingCharacterCollection;
	return gi;
}

CTGlyphInfoRef CTGlyphInfoCreateWithGlyphName(CFStringRef glyphName, CTFontRef font, CFStringRef baseString)
{
	struct __CTGlyphInfo *gi;

	if (glyphName == NULL || font == NULL) {
		return NULL;
	}
	gi = ct_glyph_info_alloc(font, baseString, glyphName);
	if (gi == NULL) {
		return NULL;
	}
	/* THE NAME IS RESOLVED THROUGH THE FACE NOW rather than at the getter, because a name the face does not know is
	 * the caller's mistake and this creator is where it is caught: `CTFontGetGlyphWithName` answers 0 for a name the
	 * face has no glyph for, and 0 is ALSO `.notdef` — so a name that resolved to nothing would be indistinguishable
	 * from one that resolved to the notdef glyph. THE NAME IS STILL KEPT, and the getter answers it. */
	gi->glyph = CTFontGetGlyphWithName(font, glyphName);
	return (CTGlyphInfoRef)gi;
}

CTGlyphInfoRef CTGlyphInfoCreateWithGlyph(CGGlyph glyph, CTFontRef font, CFStringRef baseString)
{
	struct __CTGlyphInfo *gi;

	if (font == NULL) {
		return NULL;
	}
	gi = ct_glyph_info_alloc(font, baseString, NULL);
	if (gi == NULL) {
		return NULL;
	}
	gi->glyph = glyph;
	return (CTGlyphInfoRef)gi;
}

CTGlyphInfoRef CTGlyphInfoCreateWithCharacterIdentifier(CGFontIndex cid, CTCharacterCollection collection,
							CFStringRef baseString)
{
	struct __CTGlyphInfo *gi;

	/* NO FONT, AND NO GLYPH: a CID plus a collection is a key into a CMap, and resolving one is a question this tier
	 * cannot answer (see this file's head). THE OBJECT EXISTS ANYWAY, because a caller that has a CID and wants to
	 * carry it has nowhere else to put it — and the getters below answer what it was made from. */
	gi = ct_glyph_info_alloc(NULL, baseString, NULL);
	if (gi == NULL) {
		return NULL;
	}
	gi->cid = cid;
	gi->collection = collection;
	gi->has_cid = true;
	return (CTGlyphInfoRef)gi;
}

CFStringRef CTGlyphInfoGetGlyphName(CTGlyphInfoRef glyphInfo)
{
	struct __CTGlyphInfo *gi = (struct __CTGlyphInfo *)glyphInfo;
	char buffer[256];
	CGFontRef graphics;
	int written;

	if (gi == NULL) {
		return NULL;
	}
	if (gi->glyphName != NULL || gi->font == NULL || gi->has_cid) {
		return gi->glyphName;	/* already known, or unresolvable (no face, or a CID) */
	}
	/* RESOLVED ONCE, THROUGH THE ENGINE'S OWN NAME DOOR, and CACHED because this getter's reference belongs to the
	 * object: a fresh string per call would leak in the caller's hands. */
	graphics = CTFontCopyGraphicsFont(gi->font, NULL);
	if (graphics == NULL) {
		return NULL;
	}
	written = cg_font_glyph_name(graphics, gi->glyph, buffer, sizeof buffer);
	CGFontRelease(graphics);
	if (written > 0) {
		gi->glyphName = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)buffer, (CFIndex)written,
							kCFStringEncodingUTF8, false);
	}
	return gi->glyphName;
}

CGFontIndex CTGlyphInfoGetCharacterIdentifier(CTGlyphInfoRef glyphInfo)
{
	const struct __CTGlyphInfo *gi = (const struct __CTGlyphInfo *)glyphInfo;

	return (gi == NULL || !gi->has_cid) ? 0 : gi->cid;
}

CTCharacterCollection CTGlyphInfoGetCharacterCollection(CTGlyphInfoRef glyphInfo)
{
	const struct __CTGlyphInfo *gi = (const struct __CTGlyphInfo *)glyphInfo;

	/* THE IDENTITY MAPPING IS THE ANSWER FOR EVERYTHING THAT IS NOT A CID, which is the value the field starts at —
	 * Apple's member 0, and the one that means "no collection". */
	return (gi == NULL) ? kCTIdentityMappingCharacterCollection : gi->collection;
}
