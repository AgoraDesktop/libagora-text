/*
 * CTFont.h — the font itself: the type every other Core Text type is built around, the keys its name table
 * is asked for, the vocabularies that classify it, and the doors for its metrics, its glyphs and its outlines.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). It is the header every other Core Text header includes,
 * which is why it is written after the two leaves and not before: it includes CTFontDescriptor.h, and
 * CTFontDescriptor.h cannot include it back. It also includes ATS/ATSTypes.h and two Core Graphics headers.
 *
 * THE KEYS' VALUES, AGAIN, AND THIS TIME WITH MORE EVIDENCE THAN THE LAST TIME. Apple publishes no value for
 * the twenty-two attribute keys in CTFontDescriptor.h, and it publishes none for these either — its
 * documentation index was searched and answers only `let kCTFontPostScriptNameKey: CFString`, and carries no
 * `NSFont…` string for ANY key, including the ones below that are PROVABLE.
 *
 * FIFTEEN KEYS ACROSS THE TWO HEADERS ARE PROVABLE, AND THEY PROVE A RULE. A key is provable when the AppKit
 * declares the SAME NAME with CT spelled NS, because a traits or features dictionary CROSSES between the
 * frameworks and the string it is written with has to be the string it is read with. The eight attribute keys
 * are provable that way. SO ARE SEVEN MORE, AND THEY WERE FOUND BY MEASURING RATHER THAN BY ASSUMING they had
 * no counterpart: kCTFontVariationAxisIdentifierKey, …MinimumValueKey, …MaximumValueKey, …DefaultValueKey,
 * …NameKey, kCTFontFeatureTypeIdentifierKey and kCTFontFeatureSelectorIdentifierKey all appear in Apple's
 * AppKit NSFontDescriptor.h under exactly those names with NS in place of CT.
 *
 * SO THE RULE IS: kCTFontSomethingKey's value is "NSFontSomethingKey". It is measured on fifteen keys and
 * inferred for the eighteen below, because the AppKit declares NO `NSFont…NameKey` constant anywhere — that
 * was measured too, not assumed. The eighteen are therefore recorded as an inference from a measured rule
 * rather than as a fact, which is the same standing the fourteen guessed attribute keys have, and they are the
 * first thing to check the day something can answer for them.
 */
#ifndef CORE_TEXT_CTFONT_H
#define CORE_TEXT_CTFONT_H

#include <CoreText/CTFontDescriptor.h>

#include <CoreGraphics/CGFont.h>
#include <CoreGraphics/CGPath.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* APPLE'S ATS IS NOT IN THIS TREE, so the one type of its that Core Text's surface needs is declared here.
 * Apple's ATS/ATSTypes.h says exactly this and nothing more for it, and the two platform-font doors below are
 * the only users -- which is why the type is here rather than in a framework of its own. */
typedef uint32_t ATSFontRef;

/* THE FONT. Opaque, and unlike a descriptor it is NOT a plain dictionary: it is a face, its size and its
 * matrix, which is why it has doors of its own rather than accessors over attributes. */
typedef const struct __CTFont *CTFontRef;

CFTypeID CTFontGetTypeID(void);

/* ---------------------------------------------------------------------------------------
 * THE NAME KEYS, ASKED THROUGH CTFontCopyName. They name the entries of a face's own `name` table rather
 * than attributes of a description, which is why they are a second family and not more attribute keys.
 * ------------------------------------------------------------------------------------- */
extern const CFStringRef kCTFontCopyrightNameKey;
extern const CFStringRef kCTFontFamilyNameKey;
extern const CFStringRef kCTFontSubFamilyNameKey;
extern const CFStringRef kCTFontStyleNameKey;
extern const CFStringRef kCTFontUniqueNameKey;
extern const CFStringRef kCTFontFullNameKey;
extern const CFStringRef kCTFontVersionNameKey;
extern const CFStringRef kCTFontPostScriptNameKey;
extern const CFStringRef kCTFontTrademarkNameKey;
extern const CFStringRef kCTFontManufacturerNameKey;
extern const CFStringRef kCTFontDesignerNameKey;
extern const CFStringRef kCTFontDescriptionNameKey;
extern const CFStringRef kCTFontVendorURLNameKey;
extern const CFStringRef kCTFontDesignerURLNameKey;
extern const CFStringRef kCTFontLicenseNameKey;
extern const CFStringRef kCTFontLicenseURLNameKey;
extern const CFStringRef kCTFontSampleTextNameKey;
extern const CFStringRef kCTFontPostScriptCIDNameKey;

/* THE VARIATION AXIS KEYS, SEVEN OF WHICH ARE PROVABLE (see this file's head) and two of which are not
 * declared by the AppKit: kCTFontVariationAxisNameKey is, and the other four axis keys are; the two that are
 * NOT in the AppKit's header are kCTFontFeatureTypeNameKey and its selector counterpart. */
extern const CFStringRef kCTFontVariationAxisIdentifierKey;
extern const CFStringRef kCTFontVariationAxisMinimumValueKey;
extern const CFStringRef kCTFontVariationAxisMaximumValueKey;
extern const CFStringRef kCTFontVariationAxisDefaultValueKey;
extern const CFStringRef kCTFontVariationAxisNameKey;

extern const CFStringRef kCTFontFeatureTypeIdentifierKey;
extern const CFStringRef kCTFontFeatureTypeNameKey;
extern const CFStringRef kCTFontFeatureTypeExclusiveKey;
extern const CFStringRef kCTFontFeatureTypeSelectorsKey;
extern const CFStringRef kCTFontFeatureSelectorIdentifierKey;
extern const CFStringRef kCTFontFeatureSelectorNameKey;
extern const CFStringRef kCTFontFeatureSelectorDefaultKey;
extern const CFStringRef kCTFontFeatureSelectorSettingKey;

/* HOW A FONT IS LOOKED FOR. Apple's own note is that these are the SERVER's options rather than the caller's
 * wishes, which is why they exist beside the plain creators and not instead of them. */
enum {
	kCTFontOptionsDefault               = 0,
	kCTFontOptionsPreventAutoActivation = 1 << 0,
	kCTFontOptionsPreferSystemFont      = 1 << 2
};
typedef CFOptionFlags CTFontOptions;

/* THE INTERFACE FONT ROLES, WHICH ARE THE ONLY FONTS IN THIS VOCABULARY THAT ARE NOT ASKED FOR BY NAME. A
 * role is a request to a system that decides which face plays it — and THIS TIER HAS NO SUCH SYSTEM, WHICH IS A
 * DECISION RATHER THAN A SHORTAGE: the door that takes a role is DEFINED, and it answers against the faces the
 * process's registry has, in the matching tier's own order. Which face plays which role is recorded where that
 * door is, in `CTFont.c`, because it is a decision about THIS tier and not a fact about Apple's enum.
 * THE VALUES ARE APPLE'S, AND THERE ARE GAPS IN THE NUMBERING AT 27 AND ABOVE — they stop at 26, and 27 is not
 * a role. */
enum {
	kCTFontNoFontType                     = (uint32_t)-1,
	kCTFontUserFontType                   = 0,
	kCTFontUserFixedPitchFontType         = 1,
	kCTFontSystemFontType                 = 2,
	kCTFontEmphasizedSystemFontType       = 3,
	kCTFontSmallSystemFontType            = 4,
	kCTFontSmallEmphasizedSystemFontType  = 5,
	kCTFontMiniSystemFontType             = 6,
	kCTFontMiniEmphasizedSystemFontType   = 7,
	kCTFontViewsFontType                  = 8,
	kCTFontApplicationFontType            = 9,
	kCTFontLabelFontType                  = 10,
	kCTFontMenuTitleFontType              = 11,
	kCTFontMenuItemFontType               = 12,
	kCTFontMenuItemMarkFontType           = 13,
	kCTFontMenuItemCmdKeyFontType         = 14,
	kCTFontWindowTitleFontType            = 15,
	kCTFontPushButtonFontType             = 16,
	kCTFontUtilityWindowTitleFontType     = 17,
	kCTFontAlertHeaderFontType            = 18,
	kCTFontSystemDetailFontType           = 19,
	kCTFontEmphasizedSystemDetailFontType = 20,
	kCTFontToolbarFontType                = 21,
	kCTFontSmallToolbarFontType           = 22,
	kCTFontMessageFontType                = 23,
	kCTFontPaletteFontType                = 24,
	kCTFontToolTipFontType                = 25,
	kCTFontControlContentFontType         = 26
};
typedef uint32_t CTFontUIFontType;

/* ---------------------------------------------------------------------------------------
 * CREATION. A font is a name and a size and a matrix, or a descriptor and the same two, and a copy varies one
 * of them. Every creator here answers a font for a face this tree can find; the ones that need a system to
 * choose a face are owed and named in CTFont.c.
 * ------------------------------------------------------------------------------------- */
CTFontRef CTFontCreateWithName(CFStringRef name, CGFloat size, const CGAffineTransform *matrix);
CTFontRef CTFontCreateWithFontDescriptor(CTFontDescriptorRef descriptor, CGFloat size,
					 const CGAffineTransform *matrix);
CTFontRef CTFontCreateWithNameAndOptions(CFStringRef name, CGFloat size,
					 const CGAffineTransform *matrix, CTFontOptions options);
CTFontRef CTFontCreateWithFontDescriptorAndOptions(CTFontDescriptorRef descriptor, CGFloat size,
						   const CGAffineTransform *matrix, CTFontOptions options);
CTFontRef CTFontCreateUIFontForLanguage(CTFontUIFontType uiType, CGFloat size, CFStringRef language);
CTFontRef CTFontCreateCopyWithAttributes(CTFontRef font, CGFloat size,
					 const CGAffineTransform *matrix, CTFontDescriptorRef attributes);
CTFontRef CTFontCreateCopyWithSymbolicTraits(CTFontRef font, CGFloat size,
					     const CGAffineTransform *matrix,
					     CTFontSymbolicTraits symTraitValue,
					     CTFontSymbolicTraits symTraitMask);
CTFontRef CTFontCreateCopyWithFamily(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
				     CFStringRef family);
CTFontRef CTFontCreateForString(CTFontRef currentFont, CFStringRef string, CFRange range);

/* THE FONT AND CORE GRAPHICS, WHICH IS HOW THIS TIER HOLDS ITS FACES: a CTFont owns a CGFont. The two doors
 * are inverses of each other, and CopyGraphicsFont's `attributes` out-parameter receives a RETAINED
 * descriptor that the caller must release, Apple's contract for the name `Copy`. */
CGFontRef CTFontCopyGraphicsFont(CTFontRef font, CTFontDescriptorRef *attributes);
CTFontRef CTFontCreateWithGraphicsFont(CGFontRef graphicsFont, CGFloat size,
				       const CGAffineTransform *matrix, CTFontDescriptorRef attributes);

/* AND THE TWO THAT SPEAK IN PLATFORM FONT IDS, which is ATS's vocabulary and not Core Text's; see the
 * ATSFontRef typedef above. */
ATSFontRef CTFontGetPlatformFont(CTFontRef font, CTFontDescriptorRef *attributes);
CTFontRef CTFontCreateWithPlatformFont(ATSFontRef platformFont, CGFloat size,
				       const CGAffineTransform *matrix, CTFontDescriptorRef attributes);

/* ---------------------------------------------------------------------------------------
 * WHAT A FONT SAYS ABOUT ITSELF.
 * ------------------------------------------------------------------------------------- */
CTFontDescriptorRef CTFontCopyFontDescriptor(CTFontRef font);
CFTypeRef CTFontCopyAttribute(CTFontRef font, CFStringRef attribute);
CGFloat CTFontGetSize(CTFontRef font);
CGAffineTransform CTFontGetMatrix(CTFontRef font);
CTFontSymbolicTraits CTFontGetSymbolicTraits(CTFontRef font);
CFDictionaryRef CTFontCopyTraits(CTFontRef font);
CFStringRef CTFontCopyPostScriptName(CTFontRef font);
CFStringRef CTFontCopyFamilyName(CTFontRef font);
CFStringRef CTFontCopyFullName(CTFontRef font);
CFStringRef CTFontCopyDisplayName(CTFontRef font);
CFStringRef CTFontCopyName(CTFontRef font, CFStringRef nameKey);
CFStringRef CTFontCopyLocalizedName(CTFontRef font, CFStringRef nameKey, CFStringRef *language);
CFCharacterSetRef CTFontCopyCharacterSet(CTFontRef font);
CFStringEncoding CTFontGetStringEncoding(CTFontRef font);
CFArrayRef CTFontCopySupportedLanguages(CTFontRef font);

/* ---------------------------------------------------------------------------------------
 * THE GLYPHS, WHICH IS THE ONE THING THIS LIBRARY OWNS THAT CORE GRAPHICS NO LONGER OFFERS. Apple deprecated
 * Core Graphics' public character-to-glyph doors, so this tier's door is the only one, and the mapping is
 * answered by the font engine behind CGFont rather than by this file.
 *
 * CTFontGetGlyphsForCharacters answers true when EVERY character mapped, and leaves 0 -- which is `.notdef`'s
 * index -- in the slot of one that did not, which is Apple's own contract.
 *
 * AND THE OTHER DOOR APPLE PUTS ON THIS OBJECT IS THE GLYPH AS GEOMETRY. `CTFontCreatePathForGlyph` builds a
 * `CGPath` from the face's outline, and Apple's own sentence fixes what the path contains: "the font point
 * size, matrix, and transform parameter, IN THAT ORDER" -- the size scaled in, then THIS FONT'S OWN matrix,
 * then the caller's transform, which is usually a translation to the glyph's origin. A glyph with no outline
 * (a space, a bitmap-only face) is an EMPTY PATH rather than NULL; NULL means the glyph could not be loaded
 * at all, which is Apple's "or NULL if an error occurred".
 * ------------------------------------------------------------------------------------- */
bool CTFontGetGlyphsForCharacters(CTFontRef font, const UniChar characters[], CGGlyph glyphs[],
				  CFIndex count);
CGGlyph CTFontGetGlyphWithName(CTFontRef font, CFStringRef glyphName);
CGRect CTFontGetBoundingRectsForGlyphs(CTFontRef font, CTFontOrientation orientation,
				       const CGGlyph glyphs[], CGRect boundingRects[], CFIndex count);
double CTFontGetAdvancesForGlyphs(CTFontRef font, CTFontOrientation orientation,
				  const CGGlyph glyphs[], CGSize advances[], CFIndex count);
void CTFontGetVerticalTranslationsForGlyphs(CTFontRef font, const CGGlyph glyphs[],
					    CGSize translations[], CFIndex count);
CGPathRef CTFontCreatePathForGlyph(CTFontRef font, CGGlyph glyph, const CGAffineTransform *transform);

/* THE METRICS. Every one of these is a property of the FACE divided by a property of the EM, scaled by the
 * font's size — which is why they are doors on a font rather than on a descriptor. */
CGFloat CTFontGetAscent(CTFontRef font);
CGFloat CTFontGetDescent(CTFontRef font);
CGFloat CTFontGetLeading(CTFontRef font);
unsigned CTFontGetUnitsPerEm(CTFontRef font);
CFIndex CTFontGetGlyphCount(CTFontRef font);
CGRect CTFontGetBoundingBox(CTFontRef font);
CGFloat CTFontGetUnderlinePosition(CTFontRef font);
CGFloat CTFontGetUnderlineThickness(CTFontRef font);
CGFloat CTFontGetSlantAngle(CTFontRef font);
CGFloat CTFontGetCapHeight(CTFontRef font);
CGFloat CTFontGetXHeight(CTFontRef font);

/* ---------------------------------------------------------------------------------------
 * VARIATIONS AND FEATURES, which describe the axes and the settings a face's own tables carry.
 * ------------------------------------------------------------------------------------- */
CFArrayRef CTFontCopyVariationAxes(CTFontRef font);
CFDictionaryRef CTFontCopyVariation(CTFontRef font);
CFArrayRef CTFontCopyFeatures(CTFontRef font);
CFArrayRef CTFontCopyFeatureSettings(CTFontRef font);

/* ---------------------------------------------------------------------------------------
 * THE FONT'S OWN TABLES, BY THEIR FOUR-CHARACTER TAGS. Apple publishes these values in its header, so unlike
 * the keys above there is nothing to guess: a tag is the four bytes of the table's name in the file.
 * ------------------------------------------------------------------------------------- */
enum {
	kCTFontTableBASE = 'BASE',	/* Baseline */
	kCTFontTableCFF  = 'CFF ',	/* PostScript font program */
	kCTFontTableDSIG = 'DSIG',	/* Digital signature */
	kCTFontTableEBDT = 'EBDT',	/* Embedded bitmap */
	kCTFontTableEBLC = 'EBLC',	/* Embedded bitmap location */
	kCTFontTableEBSC = 'EBSC',	/* Embedded bitmap scaling */
	kCTFontTableGDEF = 'GDEF',	/* Glyph definition */
	kCTFontTableGPOS = 'GPOS',	/* Glyph positioning */
	kCTFontTableGSUB = 'GSUB',	/* Glyph substitution */
	kCTFontTableJSTF = 'JSTF',	/* Justification */
	kCTFontTableLTSH = 'LTSH',	/* Linear threshold */
	kCTFontTableOS2  = 'OS/2',	/* OS/2 and Windows specific metrics */
	kCTFontTablePCLT = 'PCLT',	/* PCL 5 data */
	kCTFontTableVDMX = 'VDMX',	/* Vertical device metrics */
	kCTFontTableVORG = 'VORG',	/* Vertical origin */
	kCTFontTableZapf = 'Zapf',	/* Glyph reference */
	kCTFontTableAcnt = 'acnt',	/* Accent attachment */
	kCTFontTableAvar = 'avar',	/* Axis variation */
	kCTFontTableBdat = 'bdat',	/* Bitmap data */
	kCTFontTableBhed = 'bhed',	/* Bitmap header */
	kCTFontTableBloc = 'bloc',	/* Bitmap location */
	kCTFontTableBsln = 'bsln',	/* Baseline */
	kCTFontTableCmap = 'cmap',	/* Character to glyph mapping */
	kCTFontTableCvar = 'cvar',	/* CVT variation */
	kCTFontTableCvt  = 'cvt ',	/* Control value table */
	kCTFontTableFdsc = 'fdsc',	/* Font descriptor */
	kCTFontTableFeat = 'feat',	/* Layout feature */
	kCTFontTableFmtx = 'fmtx',	/* Font metrics */
	kCTFontTableFpgm = 'fpgm',	/* Font program */
	kCTFontTableFvar = 'fvar',	/* Font variation */
	kCTFontTableGasp = 'gasp',	/* Grid-fitting/Scan-conversion */
	kCTFontTableGlyf = 'glyf',	/* Glyph data */
	kCTFontTableGvar = 'gvar',	/* Glyph variation */
	kCTFontTableHdmx = 'hdmx',	/* Horizontal device metrics */
	kCTFontTableHead = 'head',	/* Font header */
	kCTFontTableHhea = 'hhea',	/* Horizontal header */
	kCTFontTableHmtx = 'hmtx',	/* Horizontal metrics */
	kCTFontTableHsty = 'hsty',	/* Horizontal style */
	kCTFontTableJust = 'just',	/* Justification */
	kCTFontTableKern = 'kern',	/* Kerning */
	kCTFontTableLcar = 'lcar',	/* Ligature caret */
	kCTFontTableLoca = 'loca',	/* Index to location */
	kCTFontTableMaxp = 'maxp',	/* Maximum profile */
	kCTFontTableMort = 'mort',	/* Morph */
	kCTFontTableMorx = 'morx',	/* Extended morph */
	kCTFontTableName = 'name',	/* Naming table */
	kCTFontTableOpbd = 'opbd',	/* Optical bounds */
	kCTFontTablePost = 'post',	/* PostScript information */
	kCTFontTablePrep = 'prep',	/* CVT program */
	kCTFontTableProp = 'prop',	/* Properties */
	kCTFontTableTrak = 'trak',	/* Tracking */
	kCTFontTableVhea = 'vhea',	/* Vertical header */
	kCTFontTableVmtx = 'vmtx'	/* Vertical metrics */
};
typedef uint32_t CTFontTableTag;

enum {
	kCTFontTableOptionNoOptions        = 0,
	kCTFontTableOptionExcludeSynthetic = (1 << 0)
};
typedef uint32_t CTFontTableOptions;

CFArrayRef CTFontCopyAvailableTables(CTFontRef font, CTFontTableOptions options);
CFDataRef CTFontCopyTable(CTFontRef font, CTFontTableTag table, CTFontTableOptions options);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONT_H */
