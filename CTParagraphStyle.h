/*
 * CTParagraphStyle.h — the paragraph style: the settings a paragraph is laid out with, and the type that carries
 * them from a caller to the framesetter.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). Apple's includes for it are `CFArray.h` and `CGBase.h` — a
 * style is Core Foundation containers and Core Graphics numbers and nothing else — which is why this header sits
 * where it does in the include order: above `CTFont` and `CTFontManager`, below the layout types that READ it.
 *
 * A PARAGRAPH STYLE IS A TABLE, AND THE TABLE IS THE INTERFACE. There are fourteen settings, each with its OWN
 * TYPE and its OWN DEFAULT VALUE, both printed in Apple's header beside the constant — and the type is not
 * decoration: a caller hands a `void *` and a size, so the SIZE IS HOW THE VALUE IS READ. That is why the
 * table below is written out in full in this header rather than reduced to an enum: it is the one place where
 * "what a paragraph style is" can be stated once, and the implementation's table is required to agree with it.
 *
 * THE ONE DEFAULT THIS TIER CANNOT YET ANSWER IS NAMED WHERE IT IS, in `CTParagraphStyle.c`: the default for
 * `kCTParagraphStyleSpecifierTabStops` is "12 left-aligned tabs, spaced by 28.0 points", and a tab is a
 * `CTTextTabRef` — whose header was LATER in Apple's include order and has SINCE LANDED, so the setting is answered
 * as Apple's twelve tabs and the deviation that row used to carry is paid (`CTParagraphStyle.c`).
 */
#ifndef CORE_TEXT_CTPARAGRAPHSTYLE_H
#define CORE_TEXT_CTPARAGRAPHSTYLE_H

#include <CoreFoundation/CFArray.h>
#include <CoreGraphics/CGBase.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE STYLE. Opaque and a Core Foundation object, so its identity comes from the runtime: `CFGetTypeID` of one
 * IS `CTParagraphStyleGetTypeID()`. IMMUTABLE, which Apple states as a reason rather than a description —
 * "Paragraph styles should be kept immutable for totally lock-free operation". */
typedef const struct __CTParagraphStyle *CTParagraphStyleRef;

CFTypeID CTParagraphStyleGetTypeID(void);

/* ---------------------------------------------------------------------------------------
 * THE VALUES THAT ARE NOT NUMBERS. Each is a small vocabulary, and the WIDTHS ARE APPLE'S: a text alignment is
 * ONE BYTE and a writing direction is ONE SIGNED BYTE, which is a fact a caller who copies these in and out of
 * a style has to know.
 * ------------------------------------------------------------------------------------- */
enum {
	kCTLeftTextAlignment = 0,
	kCTRightTextAlignment = 1,
	kCTCenterTextAlignment = 2,
	kCTJustifiedTextAlignment = 3,	/* fully justified; the last line aligns naturally */
	kCTNaturalTextAlignment = 4	/* the natural alignment of the text's script */
};
typedef uint8_t CTTextAlignment;

enum {
	kCTLineBreakByWordWrapping = 0,		/* at word boundaries, unless one word cannot fit */
	kCTLineBreakByCharWrapping = 1,		/* before the first character that does not fit */
	kCTLineBreakByClipping = 2,		/* simply not drawn past the frame's edge */
	kCTLineBreakByTruncatingHead = 3,	/* the end fits, the missing head becomes an ellipsis */
	kCTLineBreakByTruncatingTail = 4,	/* the beginning fits, the missing tail becomes an ellipsis */
	kCTLineBreakByTruncatingMiddle = 5	/* both ends fit, the middle becomes an ellipsis */
};
typedef uint8_t CTLineBreakMode;

/* A WRITING DIRECTION IS SIGNED, and the reason is in the value: `kCTWritingDirectionNatural` is `-1`, because it
 * is not a DIRECTION at all — it is the absence of one, to be determined from the text by the Unicode
 * Bidirectional Algorithm's rules P2 and P3. */
enum {
	kCTWritingDirectionNatural = -1,
	kCTWritingDirectionLeftToRight = 0,
	kCTWritingDirectionRightToLeft = 1
};
typedef int8_t CTWritingDirection;

/* ---------------------------------------------------------------------------------------
 * THE FOURTEEN SETTINGS. Name, type and default together, because those three are what a setting IS; the
 * implementation's table is required to agree with this list, and the suite holds the numbers.
 *
 * THE LAST LINE OF EACH ENTRY SAYS WHO READS IT, which is not this library yet: the framesetter and the
 * typesetter do, and both are LATER in Apple's include order. A setting is therefore RECORDED AND ANSWERED here
 * and nothing acts on it — which is the honest state of a type that is defined before its readers are.
 * ------------------------------------------------------------------------------------- */
enum {
	/* CTTextAlignment; default kCTNaturalTextAlignment; read by the framesetter */
	kCTParagraphStyleSpecifierAlignment = 0,
	/* CGFloat, nonnegative; default 0.0; the gap from the leading margin to the FIRST line; framesetter */
	kCTParagraphStyleSpecifierFirstLineHeadIndent = 1,
	/* CGFloat, nonnegative; default 0.0; the gap from the leading margin to every line BUT the first */
	kCTParagraphStyleSpecifierHeadIndent = 2,
	/* CGFloat; default 0.0; positive counts from the LEADING margin, 0 or negative from the TRAILING one */
	kCTParagraphStyleSpecifierTailIndent = 3,
	/* CFArray of CTTextTabRef, sorted by location; the default is 12 left-aligned tabs 28.0 points apart, and that is
	 * what this tier answers (see CTParagraphStyle.c); framesetter and typesetter */
	kCTParagraphStyleSpecifierTabStops = 4,
	/* CGFloat; default 0.0; tabs after the last one in TabStops go at multiples of this; framesetter, typesetter */
	kCTParagraphStyleSpecifierDefaultTabInterval = 5,
	/* CTLineBreakMode; default kCTLineBreakByWordWrapping; framesetter */
	kCTParagraphStyleSpecifierLineBreakMode = 6,
	/* CGFloat; default 0.0; multiplied into the natural line height before it is clamped; framesetter */
	kCTParagraphStyleSpecifierLineHeightMultiple = 7,
	/* CGFloat, nonnegative; default 0.0, which means NO LIMIT; framesetter */
	kCTParagraphStyleSpecifierMaximumLineHeight = 8,
	/* CGFloat, nonnegative; default 0.0; framesetter */
	kCTParagraphStyleSpecifierMinimumLineHeight = 9,
	/* CGFloat, nonnegative; default 0.0; the space added between lines -- the leading; framesetter */
	kCTParagraphStyleSpecifierLineSpacing = 10,
	/* CGFloat, nonnegative; default 0.0; added after the paragraph; framesetter */
	kCTParagraphStyleSpecifierParagraphSpacing = 11,
	/* CGFloat; default 0.0; the gap between the paragraph's top and its first line; framesetter */
	kCTParagraphStyleSpecifierParagraphSpacingBefore = 12,
	/* CTWritingDirection; default kCTWritingDirectionNatural; framesetter and typesetter */
	kCTParagraphStyleSpecifierBaseWritingDirection = 13,

	kCTParagraphStyleSpecifierCount = 14
};
typedef uint32_t CTParagraphStyleSpecifier;

/* ONE SETTING AS IT ARRIVES: a specifier, the SIZE of the value it names, and a pointer to that value. THE
 * CALLER'S MEMORY IS BORROWED FOR THE DURATION OF THE `Create` CALL -- the style COPIES what it is given, which
 * is what lets the caller free its own array afterwards. `valueSize` "must match the size of the value required
 * by the specifier", and a setting whose size does not match is SKIPPED rather than misread (see
 * CTParagraphStyleCreate). */
typedef struct CTParagraphStyleSetting {
	CTParagraphStyleSpecifier spec;
	size_t valueSize;
	const void *value;
} CTParagraphStyleSetting;

/* ---------------------------------------------------------------------------------------
 * CREATION AND ACCESS. Three doors, and Apple's 10.6 header has no more: no mutable copy, no specifier list.
 * ------------------------------------------------------------------------------------- */
/* NULL `settings` IS APPLE'S OWN WAY OF SAYING "the default set of settings", and so is a count of 0.
 *
 * AND AN UNKNOWN SPECIFIER IS NOT AN ERROR, WHICH IS APPLE'S DELIBERATE DESIGN FOR FORWARD COMPATIBILITY: "If an
 * invalid paragraph style setting specifier is passed … nothing bad will happen but just don't expect to be able
 * to query for this value." Such a setting is SKIPPED here, so a style made by a newer caller with one setting
 * this tier does not know is still a usable style with the other thirteen. */
CTParagraphStyleRef CTParagraphStyleCreate(const CTParagraphStyleSetting *settings, CFIndex settingCount);

/* AN IMMUTABLE COPY, which is why this door is short: nothing about the original can have moved since it was
 * made. NULL in, NULL out — Apple's own words for the parameter are that it "may not be set to NULL". */
CTParagraphStyleRef CTParagraphStyleCreateCopy(CTParagraphStyleRef paragraphStyle);

/* THE VALUE FOR ONE SPECIFIER, WHETHER OR NOT THE CALLER SET IT: "If the user has not set it, this function will
 * return the default value." That sentence is the door's whole reason for existing, and it is why the default
 * table in CTParagraphStyle.c is as much a part of the implementation as the storage is.
 *
 * TRUE WHEN THE BUFFER WAS FILLED, false when a parameter was invalid — and Apple gives the two invalid cases
 * different consequences, which are held here: AN UNKNOWN SPECIFIER ZEROES the buffer ("the buffer value will
 * simply be zeroed out"), while a buffer TOO SMALL FOR THE SPECIFIER'S TYPE is not written to at all. A CFArray
 * value is written as a BORROWED reference: the array belongs to the style. */
bool CTParagraphStyleGetValueForSpecifier(CTParagraphStyleRef paragraphStyle,
					  CTParagraphStyleSpecifier spec, size_t valueBufferSize,
					  void *valueBuffer);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTPARAGRAPHSTYLE_H */
