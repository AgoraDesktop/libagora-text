/*
 * CTGlyphInfo.h — the glyph substitution: the glyph that should be drawn in place of a character.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), whose ONLY include is `CTFont.h` — and that is the type's whole
 * story: a glyph substitution is A GLYPH, A FACE AND THE CHARACTER IT STANDS FOR, so this header needs the font and
 * nothing else of the framework's.
 *
 * IT IS THE LAST OF THE VOCABULARY, AND IT IS THE VALUE OF A KEY `CTStringAttributes.h` ALREADY NAMES:
 * `kCTGlyphInfoAttributeName` carries one of these, which is how a caller says "draw THIS glyph where THAT character
 * would have gone" — a ligature, a swash, or the glyph a CID stands for.
 *
 * THE THIRD CREATOR IS THE ONE THAT CANNOT BE ANSWERED, AND ITS REASON IS A TYPE THIS LIBRARY DOES NOT HAVE.
 * `CTGlyphInfoCreateWithCharacterIdentifier` names a glyph by a CID plus a CHARACTER COLLECTION, which is a CMap
 * question (a collection is a mapping table, not a font) — so a glyph info made that way KEEPS WHAT IT WAS GIVEN and
 * has no glyph to draw, recorded where it is implemented rather than answered with the CID as if it were an index.
 */
#ifndef CORE_TEXT_CTGLYPHINFO_H
#define CORE_TEXT_CTGLYPHINFO_H

#include <CoreText/CTFont.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE GLYPH INFO. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTGlyphInfoGetTypeID()`. */
typedef const struct __CTGlyphInfo *CTGlyphInfoRef;

CFTypeID CTGlyphInfoGetTypeID(void);

/* WHICH CMap a CID is a key into. Apple's values are printed in its header and there are NO GAPS in them — Identity
 * mapping first, then the five Adobe collections in the order Apple lists them — which is the opposite of the
 * underline vocabulary's shape and worth knowing before reading the case. */
enum {
	kCTIdentityMappingCharacterCollection = 0,
	kCTAdobeCNS1CharacterCollection = 1,
	kCTAdobeGB1CharacterCollection = 2,
	kCTAdobeJapan1CharacterCollection = 3,
	kCTAdobeJapan2CharacterCollection = 4,
	kCTAdobeKorea1CharacterCollection = 5
};
typedef uint16_t CTCharacterCollection;

/* ---------------------------------------------------------------------------------------
 * THE THREE WAYS TO NAME A GLYPH. All three take the CHARACTER the substitution stands for, because a glyph info
 * without one would be a glyph with nowhere to go.
 * ------------------------------------------------------------------------------------- */
/* BY NAME, resolved through the face — `CTFontGetGlyphWithName` is this library's own door for that, and the name is
 * KEPT so that the getter below answers the name it was given rather than a round trip through the face. */
CTGlyphInfoRef CTGlyphInfoCreateWithGlyphName(CFStringRef glyphName, CTFontRef font, CFStringRef baseString);

/* BY INDEX, which is what a shaper or a PDF would have. */
CTGlyphInfoRef CTGlyphInfoCreateWithGlyph(CGGlyph glyph, CTFontRef font, CFStringRef baseString);

/* BY CID IN A COLLECTION: the font is not a parameter and there is nothing to resolve against, which is this tier's
 * one recorded boundary here (see this file's head). */
CTGlyphInfoRef CTGlyphInfoCreateWithCharacterIdentifier(CGFontIndex cid, CTCharacterCollection collection,
							CFStringRef baseString);

/* ---------------------------------------------------------------------------------------
 * WHAT IT SAYS. Three getters and no `GetGlyph` or `GetBaseString` at 10.6 — Apple adds those later in the era, which
 * is a fact about this header rather than a decision here.
 * ------------------------------------------------------------------------------------- */
/* THE GLYPH'S NAME, or NULL when neither the object nor the face can give one. BORROWED, owned by the object — Apple's
 * name for the door is `Get` — and a glyph made by INDEX resolves its name LAZILY through the engine, once. */
CFStringRef CTGlyphInfoGetGlyphName(CTGlyphInfoRef glyphInfo);

/* THE CID AND ITS COLLECTION, for an object made that way; the identity mapping and zero for everything else. */
CGFontIndex CTGlyphInfoGetCharacterIdentifier(CTGlyphInfoRef glyphInfo);
CTCharacterCollection CTGlyphInfoGetCharacterCollection(CTGlyphInfoRef glyphInfo);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTGLYPHINFO_H */
