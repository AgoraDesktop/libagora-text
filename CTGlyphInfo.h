/*
 * CTGlyphInfo.h — an unencoded glyph, and the range it overrides.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE ERA DECLARES THIS CLASS IN `CTGlyphInfo.h`, `10_5_AND_LATER`, WITH THREE CREATE FUNCTIONS AND THREE
 * GETTERS.** It exists for a case the rest of the framework cannot express: a glyph a font carries but the
 * character set does not encode — "a glyph name such as `"copyright"`", as the era's own example puts it.
 *
 * **`CTGlyphInfoGetTypeID` IS DECLARED NOW** (CT-M7g). This paragraph said it was "NOT DECLARED HERE … this
 * tier has no Core Foundation … so the name stays owed — an open row — rather than shipping as something that
 * returns a number with no meaning". **THE NUMBER HAS A MEANING, AND THE NEIGHBOUR LIBRARY HAS BEEN MINTING
 * THEM ALL ALONG**: `CTTypeID_internal.h` carries the measurement. `CTGlyphInfoGetGlyph` is a **post-10.6**
 * door and is not in this era's header at all.
 */
#ifndef CTGLYPHINFO_H
#define CTGLYPHINFO_H

/* AND THE PUBLIC TYPE ITSELF, IN THE TWO-ARMED SPELLING EVERY CORE TEXT TYPE USES: an Objective-C class
 * name under `__OBJC__`, an opaque struct otherwise (so the C surface keeps Apple's shape). */
#ifdef __OBJC__
@class CTGlyphInfo;
#else
typedef struct objc_object CTGlyphInfo;
#endif
typedef CTGlyphInfo *CTGlyphInfoRef;

#include <CoreText/CTFont.h>
#include <CoreGraphics/CGBase.h>

/* `CFStringRef` IS AN `NSString *` HERE (the same divergence `CTFontCreateWithName` records), and
 * `CTCharacterCollection` is spelled as the era spells it.
 *
 * **AND ITS HOME IS THIS HEADER, WHICH IS WHERE THIS TREE DECLARED IT ANYWAY** — corrected in CT-M7e, because
 * this comment used to say the type "belongs to the SFNT layout vocabulary" in the era. It does not: 10.6
 * declares `CTCharacterCollection` and its six enumerators in `CTGlyphInfo.h`, beside the CID door that takes
 * it, which is exactly where they are here. **THE VALUES ARE APPLE'S TOO**, which makes this the second
 * vocabulary in this framework whose numbers the era publishes rather than leaving to us: the identity mapping
 * is 0 and the five Adobe collections follow in the era's own order. */
enum {
	kCTIdentityMappingCharacterCollection = 0,
	kCTAdobeCNS1CharacterCollection = 1,
	kCTAdobeGB1CharacterCollection = 2,
	kCTAdobeJapan1CharacterCollection = 3,
	kCTAdobeJapan2CharacterCollection = 4,
	kCTAdobeKorea1CharacterCollection = 5
};
typedef uint16_t CTCharacterCollection;

CTGlyphInfoRef CTGlyphInfoCreateWithGlyphName(NSString *glyphName, CTFontRef font, NSString *baseString);
CTGlyphInfoRef CTGlyphInfoCreateWithGlyph(CGGlyph glyph, CTFontRef font, NSString *baseString);
CTGlyphInfoRef CTGlyphInfoCreateWithCharacterIdentifier(CGFontIndex cid, CTCharacterCollection collection,
						       NSString *baseString);
NSString *CTGlyphInfoGetGlyphName(CTGlyphInfoRef glyphInfo);
CGFontIndex CTGlyphInfoGetCharacterIdentifier(CTGlyphInfoRef glyphInfo);
CTCharacterCollection CTGlyphInfoGetCharacterCollection(CTGlyphInfoRef glyphInfo);
CFTypeID CTGlyphInfoGetTypeID(void);

#endif /* CTGLYPHINFO_H */
