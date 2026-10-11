/*
 * CTParagraphStyle.h — the paragraph's own attributes: alignment, indents, spacing, line breaks.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A PARAGRAPH STYLE IS A SET OF VALUES AND A COUNT, WHICH IS WHY APPLE'S CONSTRUCTOR IS AN ARRAY OF
 * `{specifier, valueSize, value}` TRIPLES RATHER THAN A DOZEN INITIALISERS**: the specifiers are an enum
 * (14 of them, and the VALUES ARE THE INTERFACE because a setting's specifier is what indexes the array),
 * each with its own value type, and a door per attribute would be a door per future attribute. The style
 * object stores a COPY of each value it was given, so a caller's struct is free the moment the constructor
 * returns — which is what makes the array-of-triples shape usable from C.
 *
 * **WHAT THIS TIER HONOURS, AND WHAT IT MERELY STORES.** The FRAME honours `Alignment` (left, right, centre
 * and natural-by-script-direction, which for one font is left) — that is the half of M4's plan proof that
 * says *"alignment honoured"*. Everything else is STORED AND ANSWERED (`GetValueForSpecifier` returns what
 * was set, which is what a caller reading a style needs) but not yet APPLIED: indents, line spacing,
 * paragraph spacing, tab stops and the line-break mode are the *paragraph layout* half, and they are named
 * in the plan as their own unit rather than half-applied here. **`kCTJustifiedTextAlignment` IS REFUSED BY
 * NAME**: justification moves glyphs *within* a line (it is per-glyph spacing, not per-line placement), and
 * this tier's frame places whole lines. **AND THE SPACING HALF NOW EXISTS**: `CTLineCreateJustifiedLine`
 * (§M7c) reshapes a line to a measure, so honouring this alignment is that frame unit's own work — it asks
 * each line for its justified form at the frame's width — rather than a mechanism this header has to hold.
 */
#ifndef CORE_TEXT_CTPARAGRAPHSTYLE_H
#define CORE_TEXT_CTPARAGRAPHSTYLE_H

#include <CoreGraphics/CGBase.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTParagraphStyle;
#else
typedef struct objc_object CTParagraphStyle;
#endif
typedef CTParagraphStyle *CTParagraphStyleRef;

/* **THE SPECIFIERS, NAMES AND VALUES, TRANSCRIBED FROM THE 10.6 HEADER** (its `CTParagraphStyle.h` lines
 * 297-316). The values are the interface twice over: they index a `CTParagraphStyleSetting` array, and
 * `Count` is how a caller can size one. */
enum {
	kCTParagraphStyleSpecifierAlignment = 0,
	kCTParagraphStyleSpecifierFirstLineHeadIndent = 1,
	kCTParagraphStyleSpecifierHeadIndent = 2,
	kCTParagraphStyleSpecifierTailIndent = 3,
	kCTParagraphStyleSpecifierTabStops = 4,
	kCTParagraphStyleSpecifierDefaultTabInterval = 5,
	kCTParagraphStyleSpecifierLineBreakMode = 6,
	kCTParagraphStyleSpecifierLineHeightMultiple = 7,
	kCTParagraphStyleSpecifierMaximumLineHeight = 8,
	kCTParagraphStyleSpecifierMinimumLineHeight = 9,
	kCTParagraphStyleSpecifierLineSpacing = 10,
	kCTParagraphStyleSpecifierParagraphSpacing = 11,
	kCTParagraphStyleSpecifierParagraphSpacingBefore = 12,
	kCTParagraphStyleSpecifierBaseWritingDirection = 13,

	kCTParagraphStyleSpecifierCount = 14
};
typedef uint32_t CTParagraphStyleSpecifier;

/* AND THE TWO ENUMS ITS VALUES ARE SPELLED WITH. `CTTextAlignment`'s values are the interface (a caller
 * writes `kCTRightTextAlignment` into a setting and the frame reads it back); `CTLineBreakMode`'s are the
 * vocabulary of the setting this tier stores and does not yet apply (see the file comment). */
enum {
	kCTLeftTextAlignment = 0,
	kCTRightTextAlignment = 1,
	kCTCenterTextAlignment = 2,
	kCTJustifiedTextAlignment = 3,
	kCTNaturalTextAlignment = 4
};
typedef uint8_t CTTextAlignment;

enum {
	kCTLineBreakByWordWrapping = 0,
	kCTLineBreakByCharWrapping = 1,
	kCTLineBreakByClipping = 2,
	kCTLineBreakByTruncatingHead = 3,
	kCTLineBreakByTruncatingTail = 4,
	kCTLineBreakByTruncatingMiddle = 5
};
typedef uint8_t CTLineBreakMode;

/* THE TRIPLE, APPLE'S OWN SHAPE: a `void *` because the value's TYPE belongs to its specifier, and a size
 * because the object copies it. */
typedef struct CTParagraphStyleSetting {
	CTParagraphStyleSpecifier spec;
	size_t valueSize;
	const void *value;
} CTParagraphStyleSetting;

/* CREATION. A style with no settings is the DEFAULTS (natural alignment, plain line breaks), which is
 * exactly what a string with no paragraph attribute at all behaves like — so `Create` with a count of 0 is a
 * meaningful call rather than an empty one. A setting whose specifier is out of range, or whose value is
 * NULL, is IGNORED rather than refused: the constructor cannot report a failure, and the alternative is
 * losing the settings that were fine. */
CTParagraphStyleRef CTParagraphStyleCreate(const CTParagraphStyleSetting *settings, size_t settingCount);
CTParagraphStyleRef CTParagraphStyleCreateCopy(CTParagraphStyleRef paragraphStyle) CG_RETURNS_RETAINED;

/* AND THE READ DOOR. Apple's contract for it is the one thing a caller must know: **`bufferSize` must equal
 * the size the specifier's value has**, and a mismatch answers `false` without writing — which is why the
 * size cannot simply be computed here from the specifier's type (the caller says how much room it has, and
 * the object says how much it needs). */
bool CTParagraphStyleGetValueForSpecifier(CTParagraphStyleRef paragraphStyle, CTParagraphStyleSpecifier spec,
					  size_t bufferSize, void *buffer);

/* THE REFERENCE COUNT, THIS TIER'S DOOR RATHER THAN APPLE'S. No ledger row is claimed for it. */
CTParagraphStyleRef CTParagraphStyleRetain(CTParagraphStyleRef paragraphStyle);
void CTParagraphStyleRelease(CTParagraphStyleRef paragraphStyle);

/* **`CTParagraphStyleGetTypeID` (CT-M7g)** — declared here rather than "not here, named". The sentence this
 * replaces gave the reason as "this tree has no CoreFoundation to mint one — the boundary `CTFontCopyAttribute`
 * records", and `CTTypeID_internal.h` is where that reason was measured and found wrong. */
CFTypeID CTParagraphStyleGetTypeID(void);

#ifdef __cplusplus
}
#endif

/* **THE VALUES ARE OURS**: Apple publishes enum case NAMES and no values, and the tree's Core Text
 * vocabulary follows the same rule as the rest of it — the default or "none" case is 0, a MASK's cases are
 * powers of two, and everything else is the order the era's header declares them in. What is NOT ours is the
 * set of names: every one below is a name 10.6's own headers carry. */
/* `Natural` is -1 in the era's own declaration order, and the two directions follow (Apple's own home). */
typedef enum {
	kCTWritingDirectionNatural = -1,
	kCTWritingDirectionLeftToRight = 0,
	kCTWritingDirectionRightToLeft = 1
} CTWritingDirection;

#endif /* CORE_TEXT_CTPARAGRAPHSTYLE_H */
