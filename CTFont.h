/*
 * CTFont.h — a font, its metrics, and the character-to-glyph step.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THIS IS CT-M1a: THE OBJECT AND EVERY DOOR THAT DOES NOT NEED A STRING. A `CTFont` here is a
 * `CGFontRef` — which is a face, a name and a registry this tree already has — plus THE POINT SIZE and
 * THE MATRIX Apple's own type adds to it, and that is the whole object. WHAT IT IS NOT YET IS THE
 * NAME-BASED HALF: `CTFontCreateWithName` and the four `CTFontCopy*Name` doors take a `CFStringRef`,
 * which this tree has no type for, so they are CT-M1b's — and the plan says so at the milestone rather
 * than leaving a reader to wonder where they went.
 *
 * **AND THE SHAPE OF THE OBJECT IS WHY THIS MILESTONE EXISTS AT ALL.** Apple's `CTFontRef` is
 * toll-free bridged with AppKit's `NSFont`, and this tree's AppKit has 306 open rows waiting on a font
 * object (plan §1's table). A `CGFontRef` is metrics in FONT UNITS; a `CTFont` is the same metrics
 * SCALED BY A POINT SIZE AND A MATRIX — which is the difference between "the engine knows the font"
 * and "a caller can lay anything out".
 *
 * THE CONTRACTS ARE TRANSCRIBED FROM THE 10.6 HEADER'S OWN DOC BLOCKS, which this era publishes in
 * full (unlike the AppKit's, where several doors this plan shipped had no comment at all):
 *
 *   * `CTFontCreateWithGraphicsFont` — "creates a font reference for the given graphics font"; the
 *     `attributes` descriptor "will override any specified in the graphics font's descriptor, unless
 *     they are unspecified". REFUSED BY NAME AT THE DOOR: a non-NULL descriptor is accepted and
 *     IGNORED, because CT-M1a ships no way to BUILD one — the descriptor class is its own unit — and
 *     ignoring a parameter is stated rather than silent.
 *   * the metrics — "returns the font ascent metric scaled based on the point size AND MATRIX of the
 *     font reference". The READING this tier takes, stated because that sentence does not say how the
 *     matrix scales a height: a vertical metric is `CGFontGet*` (font units) x size / unitsPerEm x the
 *     matrix's y factor (`d`), and a horizontal one x its x factor (`a`). That is exact for the
 *     identity and for a pure scale — every matrix this tier can currently receive — and it is a
 *     STATED APPROXIMATION for a rotation or a shear, whose "scale" Apple does not define either.
 *   * `CTFontGetGlyphsForCharacters` — "performs basic character-to-glyph mapping ... only provides
 *     the nominal mapping as specified by the font's Unicode cmap (or equivalent); such mapping does
 *     not constitute proper Unicode layout". THE THREE CLAUSES THAT MAKE IT IMPLEMENTABLE ARE ALL IN
 *     THAT BLOCK: the input is UTF-16 code units, "non-BMP characters must be encoded as surrogate
 *     pairs", a pair maps the FIRST glyph to the full character and sets "the second glyph ... to 0",
 *     and the return is true only when EVERY character mapped, with unmapped ones answered as glyph 0.
 *     THE IMPLEMENTATION IS THE ENGINE'S OWN `FT_Get_Char_Index` ON THE FACE THIS FONT ALREADY HOLDS —
 *     reached through `cg_font_face()`, the CoreGraphics internal seam whose own comment says it is
 *     what "a PDF context and Core Text" need — and NOT BY OPENING THE FILE AGAIN, because a second
 *     concurrent open of one font file is a measured defect in this guest (`font_twice`).
 *   * `CTFontGetAdvancesForGlyphs` — "calculates the advances for an array of glyphs and returns the
 *     summed advance", with `advances` allowed to be NULL ("in which case only the overall advance is
 *     calculated"). The HORIZONTAL and DEFAULT orientations are implemented from
 *     `CGFontGetGlyphAdvances`; THE VERTICAL ONE IS REFUSED BY NAME and answers 0, because a vertical
 *     advance needs the font's vertical metrics table and this tier has no vertical path at all.
 */
#ifndef CORE_TEXT_CTFONT_H
#define CORE_TEXT_CTFONT_H

#include <CoreGraphics/CGBase.h>
/* §41: this header's new declarations NAME `CGPathRef`, so it owes that type's header — the rule applied to
 * a header rather than to an implementation. */
#include <CoreGraphics/CGPath.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGAffineTransform.h>
#include <CoreGraphics/CGFont.h>
#include <CoreText/CTFontDescriptor.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* AN OPAQUE REFERENCE, and SINCE 2026-10-09 IT IS A REAL OBJECTIVE-C OBJECT (`dec-35c114bf75c7c2d7`,
 * whose second half is CT-M1e-b): Apple's `CTFontRef` is a CFType, a CFType lives in a dictionary, and
 * this tree has neither CoreFoundation nor a bridge. **THE C API IS UNCHANGED** — the ref is a pointer to
 * the class, every door keeps its signature, and `CTFontRetain`/`Release` are thin wrappers.
 *
 * THE TWO ARMS ARE `CGBase.h`'s OWN IDIOM, exactly as `CTFontDescriptorRef` spells it since CT-M1e-a. */
#ifdef __OBJC__
@class CTFont;
#else
typedef struct objc_object CTFont;
#endif
typedef CTFont *CTFontRef;

/* APPLE'S OWN VALUES, from its `CTFontTraits.h` (the symbolic-trait enum and `kCTFontClassMaskShift`).
 * THE NUMBERS ARE THE INTERFACE, exactly as the cell constants were: `kCTFontClassMaskTrait` reserves
 * the UPPER FOUR BITS for the OS/2 font class, which is why the shift is declared beside them. */
enum {
	kCTFontClassMaskShift = 28
};

enum {
	kCTFontItalicTrait = (1 << 0),
	kCTFontBoldTrait = (1 << 1),
	kCTFontExpandedTrait = (1 << 5),
	kCTFontCondensedTrait = (1 << 6),
	kCTFontMonoSpaceTrait = (1 << 10),
	kCTFontVerticalTrait = (1 << 11),
	kCTFontUIOptimizedTrait = (1 << 12),
	kCTFontClassMaskTrait = (15 << kCTFontClassMaskShift)
};
typedef uint32_t CTFontSymbolicTraits;

/* THE REFERENCE COUNT, WHICH IS THE ONE PIECE OF THIS HEADER THAT IS OURS RATHER THAN APPLE'S. Apple has
 * no `CTFontRetain`: a `CTFontRef` is a CFType, and a caller owns it with `CFRetain`/`CFRelease`. THIS
 * TREE HAS NO COREFOUNDATION (see CTFontDescriptor.h), so a caller would have no way to own a font it
 * created — and a door that hands out an object nobody can release is a leak with an API shape. The
 * pair below is therefore NAMED AS OURS where it is declared, and no ledger row is claimed for it. */
CTFontRef CTFontRetain(CTFontRef font);
void CTFontRelease(CTFontRef font);

/* ------------------------------------------------------------------------------------------------
 * CT-M1b: THE NAME DOORS — THE HALF OF THIS CLASS THAT IS SPELLED WITH A STRING.
 *
 * **THE TYPES ARE THIS TREE'S AND THE DEVIATION IS APPLE'S OWN DEVICE MISSPELLED, SAID PLAINLY:**
 * Apple's signatures take and return `CFStringRef`, and this tree has no CoreFoundation, so these are
 * `NSString *` and they live in `CTFont.m`. That is not a new invention here — `CGFont.h` has declared
 * `NSString *CGFontCopyPostScriptName(CGFontRef)` beside `CGFontNames.m` since the CoreGraphics font
 * registry, and `CGBase.h` teaches a C compiler to spell `NSString` (`typedef struct objc_object
 * NSString` outside `__OBJC__`) and to spell the +1 a `Copy` door owes (`CG_RETURNS_RETAINED`). The
 * public ledger rows are the same rows either way: a row records the NAME and its availability, and
 * the type behind the name is this tier's business.
 *
 * AND THE NAME SPECIFIER CONSTANTS' VALUES ARE OURS, for the reason this tree has already recorded for
 * enum values: Apple publishes the names and the keys, never the strings. A key is only ever compared
 * with the constants beside it, which is what makes that safe.
 * ---------------------------------------------------------------------------------------------- */
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

/* A FONT BY NAME — and THE ONE PLACE THIS TIER DIVERGES FROM APPLE'S OWN PROMISE, stated here rather
 * than discovered: Apple's block says "a best match font will always be returned", and this tier's
 * registry answers NULL for a name that resolves to no face. A name matches a PostScript name, a full
 * name or `family style`, case-insensitively. */
CTFontRef CTFontCreateWithName(CFStringRef name, CGFloat size, const CGAffineTransform *matrix);

/* **`CTFontUIFontType`, ITS ENUMERATORS, AND A CLAIM THIS HEADER HAD BACKWARDS.** It used to say that 10.6's
 * `CTFont.h` declares "exactly `typedef uint32_t CTFontUIFontType;`" and "publishes **no enumerators at all**",
 * and that the ledger's cases were therefore struck. **BOTH HALVES WERE FALSE, AND ONE GREP SHOWS IT**: the
 * era's header declares all 28 enumerators WITH THEIR VALUES — `kCTFontUserFontType = 0` through
 * `kCTFontControlContentFontType = 26`, and `kCTFontNoFontType = (uint32_t)-1` — and the ledger's rows for
 * them have always been `open`, never struck. The instrument REFUSED to strike them because they ARE in the
 * era, and a story was written to explain the refusal instead of following it to the answer; §31's own title
 * ("the strike the instrument refused") is where that went wrong.
 *
 * **SO THE VALUES BELOW ARE APPLE'S.** The rule is that a value is ours only where nothing outside fixes it,
 * and here the era's own header fixes all 28 — including `kCTFontNoFontType`, which is the whole of the
 * unsigned 32-bit range. The door below therefore takes a NAME, not "a number a caller cannot name". */
enum {
	kCTFontNoFontType = (uint32_t)-1,
	kCTFontUserFontType = 0,
	kCTFontUserFixedPitchFontType = 1,
	kCTFontSystemFontType = 2,
	kCTFontEmphasizedSystemFontType = 3,
	kCTFontSmallSystemFontType = 4,
	kCTFontSmallEmphasizedSystemFontType = 5,
	kCTFontMiniSystemFontType = 6,
	kCTFontMiniEmphasizedSystemFontType = 7,
	kCTFontViewsFontType = 8,
	kCTFontApplicationFontType = 9,
	kCTFontLabelFontType = 10,
	kCTFontMenuTitleFontType = 11,
	kCTFontMenuItemFontType = 12,
	kCTFontMenuItemMarkFontType = 13,
	kCTFontMenuItemCmdKeyFontType = 14,
	kCTFontWindowTitleFontType = 15,
	kCTFontPushButtonFontType = 16,
	kCTFontUtilityWindowTitleFontType = 17,
	kCTFontAlertHeaderFontType = 18,
	kCTFontSystemDetailFontType = 19,
	kCTFontEmphasizedSystemDetailFontType = 20,
	kCTFontToolbarFontType = 21,
	kCTFontSmallToolbarFontType = 22,
	kCTFontMessageFontType = 23,
	kCTFontPaletteFontType = 24,
	kCTFontToolTipFontType = 25,
	kCTFontControlContentFontType = 26
};
typedef uint32_t CTFontUIFontType;

/* "Returns the special UI font for the given language and UI type" — with "If 0.0 is specified, the default
 * size for the requested uiType is used". **THIS TIER HAS ONE UI FONT** (see `CTFont.m`): every uiType answers
 * the tree's single face and the language argument selects nothing further, which is recorded as a
 * divergence rather than invented as a table. */
/* **THE `language` ARGUMENT IS AN `NSString *` HERE, NOT A `CFStringRef`**: this tier has no `CFStringRef`
 * (the Core Foundation carve-out the name-based half of this header already records), and Apple's own sentence
 * for it — "the language specifier for which to return the font", which may be NULL — survives that spelling
 * unchanged. It selects nothing further in this tier anyway: one UI font, one face. */
CTFontRef CTFontCreateUIFontForLanguage(CTFontUIFontType uiType, CGFloat size, CFStringRef language);

/* THE FOUR COPY DOORS RETURN +1 (that is what `Copy` means) AND ANSWER nil FOR A FACE WITH NO SUCH
 * NAME, which is a font fact rather than a failure. `-CopyDisplayName` is the full name in this tier:
 * there is no localization, and Apple's own doc says the display name is "usually the full name". */
CFStringRef CTFontCopyPostScriptName(CTFontRef font) CG_RETURNS_RETAINED;
CFStringRef CTFontCopyFamilyName(CTFontRef font) CG_RETURNS_RETAINED;
CFStringRef CTFontCopyFullName(CTFontRef font) CG_RETURNS_RETAINED;
CFStringRef CTFontCopyDisplayName(CTFontRef font) CG_RETURNS_RETAINED;

/* AND THE KEYED PAIR. `CTFontCopyName` takes one of the constants above and answers the `name` table's
 * entry for it — the table's own IDs (0-25), which is why the key→ID table lives in `CTFont.m` beside
 * the keys. THE THREE KEYS THAT ARE NOT HERE (`kCTFontStyleNameKey` resolves to no ID, and the
 * variation-axis and feature keys belong to those collections) answer nil rather than a guess, and
 * `CTFontCopyLocalizedName` sets its language out-parameter to nil because this tier has none. */
CFStringRef CTFontCopyName(CTFontRef font, CFStringRef key) CG_RETURNS_RETAINED;
CFStringRef CTFontCopyLocalizedName(CTFontRef font, CFStringRef key, CFStringRef *actualLanguage)
	CG_RETURNS_RETAINED;

/* ------------------------------------------------------------------------------------------------
 * CT-M1c: THE THREE DESCRIPTOR DOORS ON THIS CLASS.
 *
 * A font made from a descriptor is a font made from the descriptor's own attributes: THIS TIER RESOLVES
 * THE NAME (the `kCTFontNameAttribute`, which Apple defines as the PostScript name) THROUGH THE SAME
 * REGISTRY every other door uses, because that is the one attribute the engine can match. The size and
 * the matrix come from the ARGUMENTS when they are given and FROM THE DESCRIPTOR WHEN THEY ARE NOT —
 * Apple's own sentence: "The size and matrix parameters will override any specified in the font
 * descriptor, unless they are unspecified."
 *
 * **AND THE REFUSAL IS NAMED, BECAUSE A DESCRIPTOR CARRIES MORE THAN A NAME**: the traits, variation,
 * cascade-list, character-set, language and feature attributes are PRESERVED on the descriptor and
 * IGNORED here (Apple's own wording for an attribute it does not act on is "ignored on font creation"),
 * and a descriptor whose FAMILY name is set but whose name is not answers NULL rather than guessing a
 * match, because family and style matching is the font-collection unit's work.
 * ---------------------------------------------------------------------------------------------- */
CTFontRef CTFontCreateWithFontDescriptor(CTFontDescriptorRef descriptor, CGFloat size,
					 const CGAffineTransform *matrix);
CTFontDescriptorRef CTFontCopyFontDescriptor(CTFontRef font);
CTFontRef CTFontCreateCopyWithAttributes(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
					 CTFontDescriptorRef attributes);

/* CREATION. The graphics-font door is the bridge to everything this tree already has: a `CGFontRef`
 * comes from `CGFontCreateWithFontName`, from a data provider, or from an image's font. */
CTFontRef CTFontCreateWithGraphicsFont(CGFontRef graphicsFont, CGFloat size, const CGAffineTransform *matrix,
				       CTFontDescriptorRef attributes);

/* THE METRICS, SCALED (see the file comment for the reading of "scaled ... and matrix"). */
CGFloat CTFontGetSize(CTFontRef font);
CGAffineTransform CTFontGetMatrix(CTFontRef font);
CGFloat CTFontGetAscent(CTFontRef font);
CGFloat CTFontGetDescent(CTFontRef font);
CGFloat CTFontGetLeading(CTFontRef font);
CGFloat CTFontGetCapHeight(CTFontRef font);
CGFloat CTFontGetXHeight(CTFontRef font);
CGRect CTFontGetBoundingBox(CTFontRef font);
unsigned CTFontGetUnitsPerEm(CTFontRef font);
long CTFontGetGlyphCount(CTFontRef font);
CTFontSymbolicTraits CTFontGetSymbolicTraits(CTFontRef font);

/* ------------------------------------------------------------------------------------------------
 * CT-M1d: THE FONT'S OWN NUMBERS, AND ITS COVERAGE.
 *
 * THE TWO UNDERLINE METRICS FOLLOW THE SAME SCALING RULE AS EVERY OTHER METRIC ("scaled based on the
 * point size and matrix"), and their SOURCE is the face's `post` table — which FreeType parsed when it
 * opened the face, so a face without one answers 0 rather than a guess. **THE SIGN IS PASSED THROUGH: an
 * underline position is normally NEGATIVE (the line sits below the baseline) and this tier does not flip
 * it**, because the substrate's sign is what every other door in this tree already reports.
 *
 * `CTFontGetSlantAngle` IS "the transformed slant angle ... the italic or caret angle WITH ANY SKEW FROM
 * THE TRANSFORMATION MATRIX APPLIED" (Apple's own sentence): the face's italic angle, in degrees, plus the
 * skew the matrix introduces. With the identity matrix that is EXACTLY the face's angle, which is the case
 * the probe asserts.
 *
 * AND `CTFontCopyCharacterSet` answers "the font's character set ... the nominal referenced by the font's
 * Unicode cmap table (or equivalent)" — THE ENGINE'S OWN CMAP, ENUMERATED, which is the same table
 * `CTFontGetGlyphsForCharacters` looks a character up in. It returns +1. **`NSCharacterSet` IS NAMED HERE
 * FOR THE FIRST TIME IN THIS TREE**, and `CGBase.h`'s C-visible type list has been extended by exactly one
 * name to let a C header say so — which is what that list is for.
 * ---------------------------------------------------------------------------------------------- */
CGFloat CTFontGetUnderlinePosition(CTFontRef font);
CGFloat CTFontGetUnderlineThickness(CTFontRef font);
CGFloat CTFontGetSlantAngle(CTFontRef font);
CFCharacterSetRef CTFontCopyCharacterSet(CTFontRef font) CG_RETURNS_RETAINED;

/* THE CHARACTER-TO-GLYPH STEP AND THE ADVANCES, both documented in the file comment. */
bool CTFontGetGlyphsForCharacters(CTFontRef font, const unsigned short characters[], CGGlyph glyphs[],
				  long count);
double CTFontGetAdvancesForGlyphs(CTFontRef font, CTFontOrientation orientation, const CGGlyph glyphs[],
				  CGSize advances[], long count);

/* ------------------------------------------------------------------------------------------------
 * CT-M3: THE GRAPHICS FONT THE DRAWING DOORS NEED, AND WHY IT IS HERE RATHER THAN IN A DRAWING FILE.
 *
 * Apple's door has an OUT-PARAMETER — "additional attributes from the font will be passed back as a font
 * descriptor via the attributes parameter" — so the second argument receives the descriptor this tier can
 * always build (`CTFontCopyFontDescriptor`) when the caller asks for it, or NULL. **THE RESULT IS
 * RETAINED** ("the result must be released by the caller"), and it is the SAME `CGFontRef` the font was
 * made from rather than a copy: a font object holds one graphics font for its whole life, which is the
 * "one face per font" rule the other way round.
 *
 * AND IT EXISTS FOR THE DRAWING DOORS, WHICH ARE THE PLAN'S RULE 1 IN ACTION: a line is drawn by handing
 * CoreGraphics the GLYPHS and the FONT — `CGContextSetFont` takes a `CGFontRef` — so this door is the
 * bridge from the object a caller holds to the thing the context needs.
 * ---------------------------------------------------------------------------------------------- */
/* **NO `CG_RETURNS_RETAINED` HERE, AND THAT IS A FIX RATHER THAN A STYLE.** The macro is
 * `ns_returns_retained`, which applies to functions returning an OBJECTIVE-C OBJECT; a `CGFontRef` is a plain
 * pointer in this tree, so the attribute is inapplicable and clang said so on every build that included this
 * header. The function's own name carries the convention — "Copy"/"Create" return `+1` — which is how every other
 * C-typed declaration in this framework is already read. */
CGFontRef CTFontCopyGraphicsFont(CTFontRef font, CTFontDescriptorRef *attributes);


/* **THE VALUES ARE OURS**: Apple publishes enum case NAMES and no values, and the tree's Core Text
 * vocabulary follows the same rule as the rest of it — the default or "none" case is 0, a MASK's cases are
 * powers of two, and everything else is the order the era's header declares them in. What is NOT ours is the
 * set of names: every one below is a name 10.6's own headers carry. */
/* the font options are a MASK: `Default` is zero and the flags are bits (Apple's own home). */
typedef enum {
	kCTFontOptionsDefault = 0,
	kCTFontOptionsPreventAutoActivation = (1 << 0),
	kCTFontOptionsPreferSystemFont = (1 << 1)
} CTFontOptions;

/* **THE TABLE TAGS — AND A TAG *IS* ITS FOUR CHARACTERS**, which is why these values are the vocabulary's
 * own rather than this tree's: the rule §36 states (the values are ours) holds where nothing outside
 * fixes them, and here the SFNT vocabulary does. */
typedef enum {
	kCTFontTableAcnt = 'acnt',
	kCTFontTableAvar = 'avar',
	kCTFontTableBASE = 'BASE',
	kCTFontTableBdat = 'bdat',
	kCTFontTableBhed = 'bhed',
	kCTFontTableBloc = 'bloc',
	kCTFontTableBsln = 'bsln',
	kCTFontTableCFF = 'CFF ',
	kCTFontTableCmap = 'cmap',
	kCTFontTableCvar = 'cvar',
	kCTFontTableCvt = 'cvt ',
	kCTFontTableDSIG = 'DSIG',
	kCTFontTableEBDT = 'EBDT',
	kCTFontTableEBLC = 'EBLC',
	kCTFontTableEBSC = 'EBSC',
	kCTFontTableFdsc = 'fdsc',
	kCTFontTableFeat = 'feat',
	kCTFontTableFmtx = 'fmtx',
	kCTFontTableFpgm = 'fpgm',
	kCTFontTableFvar = 'fvar',
	kCTFontTableGDEF = 'GDEF',
	kCTFontTableGPOS = 'GPOS',
	kCTFontTableGSUB = 'GSUB',
	kCTFontTableGasp = 'gasp',
	kCTFontTableGlyf = 'glyf',
	kCTFontTableGvar = 'gvar',
	kCTFontTableHdmx = 'hdmx',
	kCTFontTableHead = 'head',
	kCTFontTableHhea = 'hhea',
	kCTFontTableHmtx = 'hmtx',
	kCTFontTableHsty = 'hsty',
	kCTFontTableJSTF = 'JSTF',
	kCTFontTableJust = 'just',
	kCTFontTableKern = 'kern',
	kCTFontTableLTSH = 'LTSH',
	kCTFontTableLcar = 'lcar',
	kCTFontTableLoca = 'loca',
	kCTFontTableMaxp = 'maxp',
	kCTFontTableMort = 'mort',
	kCTFontTableMorx = 'morx',
	kCTFontTableName = 'name',
	kCTFontTableOS2 = 'OS/2',
	kCTFontTableOpbd = 'opbd',
	kCTFontTablePCLT = 'PCLT',
	kCTFontTablePost = 'post',
	kCTFontTablePrep = 'prep',
	kCTFontTableProp = 'prop',
	kCTFontTableTrak = 'trak',
	kCTFontTableVDMX = 'VDMX',
	kCTFontTableVORG = 'VORG',
	kCTFontTableVhea = 'vhea',
	kCTFontTableVmtx = 'vmtx',
	kCTFontTableZapf = 'Zapf',
} CTFontTableTag;

/* **AND THE OPTION THE TWO TABLE DOORS TAKE, IN APPLE'S OWN HOME** (`CTFont.h`, where 10.6 declares it, right
 * after this vocabulary). The values are Apple's, and unlike most of this framework's enums THEY ARE
 * PUBLISHED: 10.6 writes `= 0` and `= (1 << 0)` beside them. `ExcludeSynthetic` is the deprecated one — kept
 * and honoured because a ported application still passes it (the user's policy), and its meaning is the §45
 * shape: it asks that tables the ENGINE WOULD SYNTHESIZE be left out, and this tier synthesizes none — a face's
 * tables are the file's own, read through the font engine — so the option describes behaviour this tier already
 * has. The probe asserts that the two option values CHANGE NOTHING rather than that the flag is ignored. */
typedef enum {
	kCTFontTableOptionNoOptions = 0,
	kCTFontTableOptionExcludeSynthetic = (1 << 0)
} CTFontTableOptions;

/* **M6r: THE FONT'S ATTRIBUTES, TRAITS, NAMES, OUTLINES AND INK** — five doors of which THREE COMPOSE on
 * doors that already exist: the descriptor's attribute door (§41) applied to the descriptor made from this
 * font; the face's own glyph-name table, the same lookup `CTGlyphInfo` uses; and the one ink box
 * (`CTInk_internal.h`) the run's and the line's bounds already share.
 *
 * The ORIENTATION is refused by name for the vertical case: this tier lays text out left-to-right and says so
 * elsewhere. `CTFontGetTypeID` IS declared, at the end of the block below. */
CFTypeRef CTFontCopyAttribute(CTFontRef font, CFStringRef attribute) CG_RETURNS_RETAINED;
CFDictionaryRef CTFontCopyTraits(CTFontRef font) CG_RETURNS_RETAINED;
CGGlyph CTFontGetGlyphWithName(CTFontRef font, CFStringRef glyphName);
/* AND THE SAME ABSENCE FOR THE SAME REASON: a `CGPathRef` is a plain pointer, and the name says `+1`. */
CGPathRef CTFontCreatePathForGlyph(CTFontRef font, CGGlyph glyph, const CGAffineTransform *matrix);
CGRect CTFontGetBoundingRectsForGlyphs(CTFontRef font, CTFontOrientation orientation, const CGGlyph *glyphs,
				       CGRect *boundingRects, long count);

/* **M6s: THE OPTIONS AND THE FAMILY** — three doors whose whole content is a door that already exists:
 *
 *   * `CreateWithNameAndOptions` / `CreateWithFontDescriptorAndOptions` are their 10.5 counterparts "with
 *     options", where the options are the `CTFontOptions` mask (§36). **THE TWO FLAGS DESCRIBE BEHAVIOUR THIS
 *     TIER ALREADY HAS**: `kCTFontOptionsPreventAutoActivation` — this tier never auto-activates a face, it
 *     reads the font directories and nothing else — and `kCTFontOptionsPreferSystemFont` — this tier has one
 *     font set, so there is nothing to prefer between. An option whose effect is absent by construction is
 *     satisfied rather than ignored, and saying so is the difference between a no-op and a stub;
 *   * `CreateCopyWithFamily` — "a new font based on the original font with the specified family" — which is
 *     the descriptor's own copy door (§41) with the family attribute, and then the descriptor's font door.
 *
 * `CTFontCreateCopyWithSymbolicTraits` remains owed and is named here rather than left to be found: the era
 * defines it as a copy "with the specified symbolic traits", which means RESOLVING A STYLE within a family —
 * a face-selection rule this tier's registry does not carry, and inventing one would be inventing the surface. */
CTFontRef CTFontCreateWithNameAndOptions(CFStringRef name, CGFloat size, const CGAffineTransform *matrix,
					 CTFontOptions options);
CTFontRef CTFontCreateWithFontDescriptorAndOptions(CTFontDescriptorRef descriptor, CGFloat size,
						   const CGAffineTransform *matrix, CTFontOptions options);
CTFontRef CTFontCreateCopyWithFamily(CTFontRef font, CGFloat size, const CGAffineTransform *matrix,
				     CFStringRef family);

/* **M7b: THE FONT'S SFNT TABLES — WHAT THE FILE ITSELF CARRIES.** Two doors, and they are ONE WALK SEEN TWICE:
 *
 *   * `CTFontCopyAvailableTables` asks the FACE which tables it has (the font engine's own table-index door),
 *     so the array is the file's real table directory rather than a list this tree kept;
 *   * `CTFontCopyTable` FETCHES ONE, by tag, through the same face. The two agree by construction — the probe
 *     checks that every tag the array reports yields nonzero data and that a tag the face lacks answers NULL —
 *     which is the property a second, private list of table names could not have.
 *
 * **AND TWO MAPPINGS FROM APPLE'S SPELLING ARE STATED RATHER THAN HIDDEN.** Apple's array holds the tags
 * UNBOXED, because a `CFArray` holds pointer-sized values — its own doc block extracts one with
 * `(CTFontTableTag)(uintptr_t)CFArrayGetValueAtIndex(tags, index)`. An `NSArray` holds OBJECTS, so the value
 * here is an `NSNumber` carrying the tag (`-unsignedIntValue`), which is the whole of the difference. And
 * Apple's "the table data is not actually copied" is an optimisation note about wrapping the file's own bytes:
 * this tier's bytes come from the face through the font engine, and the returned data object owns them, so
 * there IS one copy per call. Both are stated because a caller reading Apple's doc block and finding an object
 * in the array — or a copy — should be reading the reason here rather than a discrepancy.
 *
 * **AND `CTFontGetTypeID` IS DECLARED HERE** (CT-M7g), because the sentence that used to close this block
 * said it was not, "as for every type here" — nine headers said that, and `CTTypeID_internal.h` carries the
 * measurement that corrects all nine. */
/* **M7l: THE PLATFORM FONT IDENTITY, AND A HOME DEVIATION RECORDED RATHER THAN HIDDEN.** Apple declares
 * `ATSFontRef` in `ATSTypes.h` (`typedef UInt32 ATSFontRef;` — a 32-BIT INTEGER, not a pointer, which is what
 * makes "the font object IS the platform font" impossible and an ID table necessary), and this tree has no ATS
 * framework: the platform/Quickdraw pair has been "out of scope since the port began" since §10. **THE TYPE IS
 * DECLARED HERE, BESIDE THE TWO DOORS THAT TAKE AND ANSWER IT**, which is the same reading `CTGlyphInfo.h`
 * records for `CTCharacterCollection`: when the era's home is a framework this tree does not carry, the type
 * goes where its only users are and the deviation is written down.
 *
 * **AND THE IDS ARE THIS TIER'S, ON THE `CGTypeID` PRECEDENT**: Apple's are minted by ATS at load and published
 * nowhere, so a caller can round-trip one and cannot port one — which is the whole of what the type promises.
 * Zero is not an id (an invalid or failed lookup answers it), exactly as the type-id table's numbering
 * requires. */
typedef uint32_t ATSFontRef;

/* **"CREATES A NEW FONT REFERENCE FROM AN ATSFontRef"**, with Apple's own size default ("If 0.0 is specified,
 * the default font size of 12.0 will be used") and its matrix reading. **THE `attributes` DESCRIPTOR IS
 * APPLIED RATHER THAN IGNORED**: Apple's sentence is "a CTFontDescriptorRef containing additional attributes
 * that should be matched", and this tier's way of applying a descriptor to a font is the copy door (§41) — so
 * the font is created from the platform identity and then copied with those attributes, which is the same
 * composition §M7c used for the family copy. An unknown ref answers NULL, and an unknown ref is the case that
 * makes this a question rather than a lookup. */
CTFontRef CTFontCreateWithPlatformFont(ATSFontRef platformFont, CGFloat size, const CGAffineTransform *matrix,
				       CTFontDescriptorRef attributes);

/* **"RETURNS THE ATSFontRef AND ATTRIBUTES"** — the round trip, with Apple's own out-parameter contract: a
 * pointer "to receive a font descriptor containing additional attributes. Can be NULL. Must be released by
 * caller." The ref is the SAME one the same face was created with (or minted with, the first time), because
 * the identity is the face's name and not an allocation. */
ATSFontRef CTFontGetPlatformFont(CTFontRef font, CTFontDescriptorRef *attributes);

/* **M7m: THE VARIATION AND FEATURE DOORS — THE FOUR §47 LEFT OWED because the two faces this tree shipped
 * carried neither table. Both tables are here now (`userland/fonts/variable/`, §61), so the doors land with the
 * input their positive branches need.
 *
 * **THE VARIATION HALF IS `FT_Get_MM_Var` AND NOTHING ELSE.** `CTFontCopyVariationAxes` answers "an array of
 * variation axes" whose dictionaries carry the five keys above, and the face's own multiple-master data IS that
 * array — the axis tag, its three values and its name. A face with no `fvar` answers an EMPTY ARRAY rather than
 * NULL, because "this font has no axes" is an answer; `CTFontCopyVariation` answers "the current variation
 * instance as a dictionary" keyed by the axis identifier, and **THIS TIER CARRIES NO INSTANCE**: a font is made
 * from a name, a size and a matrix, and the variation attribute §41's descriptor does not implement, so the
 * current instance IS the default one. That is stated rather than implied, and the probe checks the two doors
 * AGREE, which is the property that makes the pair meaningful.
 *
 * **THE FEATURE HALF PARSES `feat` ITSELF**, Apple's AAT table, to Apple's published layout — the one table in
 * this framework the tier reads rather than the ENGINE reads, because FreeType has no AAT feature API. The
 * eight keys below are §53's vocabulary, and the NAME strings come from the `name` table through the CoreGraphics
 * seam (`cg_font_name_entry`), so a feature's name is looked up exactly as a font's own name is. Their sentences
 * are the spec, and two matter in particular: `…SelectorDefaultKey` "if present and true indicates that this
 * selector is the default setting for the current feature type", and `…SelectorSettingKey` "to indicate whether
 * this selector is on or off. If this key is not present, the default setting is used".
 *
 * A face with no `feat` answers NULL from both, which is Apple's own "or NULL if the default settings should be
 * used". */
CFArrayRef CTFontCopyVariationAxes(CTFontRef font) CG_RETURNS_RETAINED;
CFDictionaryRef CTFontCopyVariation(CTFontRef font) CG_RETURNS_RETAINED;
CFArrayRef CTFontCopyFeatures(CTFontRef font) CG_RETURNS_RETAINED;
CFArrayRef CTFontCopyFeatureSettings(CTFontRef font) CG_RETURNS_RETAINED;

CFTypeID CTFontGetTypeID(void);
CFArrayRef CTFontCopyAvailableTables(CTFontRef font, CTFontTableOptions options) CG_RETURNS_RETAINED;
CFDataRef CTFontCopyTable(CTFontRef font, CTFontTableTag table, CTFontTableOptions options) CG_RETURNS_RETAINED;

/* **M7f: THE VARIATION AND FEATURE VOCABULARY — ELEVEN KEYS, IN APPLE'S OWN HOME.** The era declares these in
 * `CTFont.h`, not in the headers of the doors that will read them, and the standing rule is to match Apple in
 * which header a name lives in.
 *
 * THE VARIATION AXIS KEYS are the five entries of an axis dictionary: the identifier, the three values and the
 * localized name — and the era's own sentence for `CTFontCopyVariationAxes` says exactly that ("Each variation
 * axis dictionary contains the five variation axis keys above"). THE FEATURE KEYS are the two nested shapes an AAT
 * feature has: a TYPE with a name, an exclusivity flag and its selectors, and a SELECTOR with a name, a default
 * flag and its setting.
 *
 * **THE DOORS THAT READ THEM ARE OWED AND THIS UNIT SAYS SO RATHER THAN SHIPPING THEM**: `CTFontCopyVariationAxes`,
 * `CTFontCopyVariation`, `CTFontCopyFeatures` and `CTFontCopyFeatureSettings` need an `fvar`/`feat` face, and the
 * two faces this tree ships carry neither — so their positive branch would have no input and their checks could
 * not fail. The VOCABULARY is a different question: a key is a name Apple publishes with a documented value TYPE,
 * and a name is not a measurement. This is the same split §46 made for the font manager's error domain, shipped
 * before anything produced an error.
 *
 * The VALUES are ours, as every unpublished key's are. */
extern const CFStringRef kCTFontVariationAxisIdentifierKey;
extern const CFStringRef kCTFontVariationAxisMinimumValueKey;
extern const CFStringRef kCTFontVariationAxisMaximumValueKey;
extern const CFStringRef kCTFontVariationAxisDefaultValueKey;
extern const CFStringRef kCTFontVariationAxisNameKey;

extern const CFStringRef kCTFontFeatureTypeNameKey;
extern const CFStringRef kCTFontFeatureTypeExclusiveKey;
extern const CFStringRef kCTFontFeatureTypeSelectorsKey;
extern const CFStringRef kCTFontFeatureSelectorNameKey;
extern const CFStringRef kCTFontFeatureSelectorDefaultKey;
extern const CFStringRef kCTFontFeatureSelectorSettingKey;

/* **AND THE TWO IDENTIFIER KEYS ARE DECLARED ELSEWHERE, WHICH IS A PLACEMENT DEVIATION FOUND HERE RATHER THAN
 * CREATED**: `kCTFontFeatureTypeIdentifierKey` and `kCTFontFeatureSelectorIdentifierKey` are Apple's too (10.6
 * declares them in `CTFont.h` with the rest of this vocabulary) and they were already shipping from
 * `SFNTLayoutTypes.h`, defined in `CTFontDescriptor.m`. They are NOT repeated here, because a second identical
 * declaration is a second place for the two to drift — but Apple's home for them is this header, and moving them
 * is a change with callers rather than a line in a vocabulary unit. Recorded, not done.
 */

#ifdef __cplusplus
}
#endif
#endif /* CORE_TEXT_CTFONT_H */
