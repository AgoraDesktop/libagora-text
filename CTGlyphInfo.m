/*
 * CTGlyphInfo.m — the class, its six doors, and the ONE predicate the drawing side asks.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE THREE CREATE FUNCTIONS DIFFER ONLY IN HOW THE GLYPH IS NAMED**, which is why one internal
 * constructor serves all three and the `kind` field is what the getters read:
 *
 *   * by NAME — "a glyph name such as `"copyright"`", resolved through the font's own glyph-name table;
 *   * by INDEX — a `CGGlyph` the caller already has;
 *   * by CHARACTER IDENTIFIER — a `cid` and a collection, which is a pre-OpenType Adobe arrangement.
 *
 * **THE THIRD ONE CANNOT BE RESOLVED IN THIS TIER, AND SAYS SO BY NAME**: a CID is an index into one of
 * Adobe's character collections, and this tree carries no such tables — so the object is created, stores what
 * it was given, answers both getters with it, and `ctg_is_resolvable` REFUSES it. That is the shape this tree
 * prefers for an unimplementable door (work list = the in-body refusal sites) rather than a value invented to
 * look complete.
 */
#import "CTGlyphInfo_internal.h"

#import <Foundation/Foundation.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <CoreGraphics/CGFont_internal.h>
#include <CoreText/CTStringAttributes.h>
/* **THE FONT'S OWN IVARS**, which is where `graphics` lives: this file reads the face out of a `CTFontRef`, so
 * it needs the internal header rather than the public one — the same include `CTLine.m` uses to shape. */
#include <CoreText/CTFont_internal.h>

#include <string.h>

/* THE ONE CONSTRUCTOR, with everything the three doors collect. `base` may be nil, which overrides nothing:
 * the attribute key's first condition is that the range's contents MATCH the base string, and a nil one is
 * stored as the empty string so that the comparison is a comparison and not a special case. */
/* **THE CLASS AND ITS IVARS, HERE RATHER THAN IN THE SHARED HEADER** (see that header's comment): the layout
 * belongs to the unit that owns it, and the scaled ivar-offset symbols clang emits come with it. */
@interface CTGlyphInfo : NSObject {
	@public
	int kind;			/* 0 = by name, 1 = by index, 2 = by character identifier */
	NSString *glyph_name;		/* RETAINED; the name door's lookup key */
	CGGlyph glyph_index;		/* the index door's glyph, or a cid under `kind` 2 */
	uint16_t collection;		/* the CID door's collection */
	CTFontRef font;			/* RETAINED: the font the glyph is read from */
	NSString *base_string;		/* RETAINED: the text this object overrides */
}
@end

@implementation CTGlyphInfo
@end

static CTGlyphInfo *ctg_new(int kind, NSString *name, CGGlyph index, uint16_t collection, CTFontRef font,
			    NSString *base)
{
	CTGlyphInfo *info = [[CTGlyphInfo alloc] init];

	info->kind = kind;
	info->glyph_name = [name copy];
	info->glyph_index = index;
	info->collection = collection;
	info->font = (font != NULL) ? (CTFontRef)CTFontRetain(font) : NULL;
	info->base_string = [(base != nil ? base : @"") copy];
	return info;
}

CTGlyphInfoRef CTGlyphInfoCreateWithGlyphName(NSString *glyphName, CTFontRef font, NSString *baseString)
{
	if (glyphName == nil) {
		return NULL;
	}
	return ctg_new(0, glyphName, 0, 0, font, baseString);
}

CTGlyphInfoRef CTGlyphInfoCreateWithGlyph(CGGlyph glyph, CTFontRef font, NSString *baseString)
{
	return ctg_new(1, nil, glyph, 0, font, baseString);
}

CTGlyphInfoRef CTGlyphInfoCreateWithCharacterIdentifier(CGFontIndex cid, CTCharacterCollection collection,
						       NSString *baseString)
{
	return ctg_new(2, nil, (CGGlyph)cid, collection, NULL, baseString);
}

NSString *CTGlyphInfoGetGlyphName(CTGlyphInfoRef glyphInfo)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *answer = nil;

	if (glyphInfo != NULL && glyphInfo->kind == 0) {
		answer = glyphInfo->glyph_name;
	}
	/* THE NAME IS ANSWERED WITHOUT OWNERSHIP HANDED OVER, which is the convention this tier's name-based
	 * doors already follow (see `CTFontCopyFamilyName`). */
	[pool release];
	return answer;
}

CGFontIndex CTGlyphInfoGetCharacterIdentifier(CTGlyphInfoRef glyphInfo)
{
	if (glyphInfo == NULL || glyphInfo->kind != 2) {
		return 0;
	}
	return (CGFontIndex)glyphInfo->glyph_index;
}

CTCharacterCollection CTGlyphInfoGetCharacterCollection(CTGlyphInfoRef glyphInfo)
{
	if (glyphInfo == NULL || glyphInfo->kind != 2) {
		return 0;
	}
	return glyphInfo->collection;
}

NSString *ctg_base_string(CTGlyphInfo *info)
{
	return (info == nil) ? nil : info->base_string;
}

/* **BOTH OF THE KEY'S CONDITIONS, IN ONE PLACE**, because they are one question — "may this object stand in
 * for this range?" The first is the base string, compared by the caller (it owns the range's text); these are
 * the second and third: the glyph must RESOLVE (by name or index) and it must be AVAILABLE IN THE FONT the
 * run is being drawn with, which is the era's own last clause. */
int ctg_is_resolvable(CTGlyphInfo *info, CTFontRef run_font, CGGlyph *out_glyph)
{
	FT_Face face;
	FT_Face run_face;
	CGGlyph glyph = 0;

	if (info == nil || info->font == NULL) {
		return 0;
	}
	face = (FT_Face)cg_font_face(info->font->graphics);
	run_face = (run_font != NULL) ? (FT_Face)cg_font_face(run_font->graphics) : face;
	if (face == NULL || run_face == NULL) {
		return 0;
	}
	if (info->kind == 1) {
		glyph = info->glyph_index;
	} else if (info->kind == 0) {
		const char *name = [info->glyph_name UTF8String];

		if (name == NULL) {
			return 0;
		}
		glyph = (CGGlyph)FT_Get_Name_Index(face, (FT_String *)name);
		if (glyph == 0) {
			return 0;	/* the font has no glyph by that name: the override simply does not happen */
		}
	} else {
		return 0;		/* A CID: the Adobe character collections are not in this tree */
	}
	if (run_face->num_glyphs <= 0 || glyph >= (CGGlyph)run_face->num_glyphs) {
		return 0;		/* "the specified glyph is available in the font specified by kCTFontAttributeName" */
	}
	*out_glyph = glyph;
	return 1;
}
