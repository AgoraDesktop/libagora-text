/*
 * CTStringAttributes.h — the attribute names a Core Text string carries.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * APPLE'S HEADER OF THE SAME NAME declares FOURTEEN of these, and this file ships TWO — each one because a
 * DOOR WOULD NOT WORK WITHOUT IT (first the font a line is shaped from, then the paragraph style a frame's
 * alignment comes from). **(CORRECTED BY M4b: this paragraph said "ONE" and named `CTParagraphStyleRef`
 * among the types that "this tier cannot carry yet"; the style now exists, so the style's KEY is the second
 * name here.) THE OTHER TWELVE ARE THE M6 VOCABULARY UNIT** ("the `kCT*AttributeName` vocabulary and its
 * Foundation twins"), and they wait for the same reason as before: most are packaged as types this tier
 * cannot carry yet — a `CGColorRef` (a Core Graphics object this tree has not made an object of), a
 * `CTRunDelegateRef` — and shipping a NAME whose value cannot be built is the drift the plan's rule 4 warns
 * about.
 *
 * **`kCTFontAttributeName` IS THE ONE WHOSE VALUE IS READY, AND IT IS READY BECAUSE OF THE USER'S
 * DECISION** (`dec-35c114bf75c7c2d7`): Apple says "the value must be a `CTFontRef`", and since CT-M1e-b
 * a `CTFontRef` IS an Objective-C object — so the value goes into an attributed string AS ITSELF, with
 * no wrapper and no bridge, which is what the decision was for.
 */
#ifndef CORE_TEXT_CTSTRINGATTRIBUTES_H
#define CORE_TEXT_CTSTRINGATTRIBUTES_H

#include <CoreGraphics/CGBase.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE VALUE IS A `CTFontRef` (which is to say: the font object itself). Apple's own doc block for this
 * key says so in one sentence — "the value must be a CTFontRef" — and it is the whole of what a line
 * needs in order to be shaped. */
extern NSString *const kCTFontAttributeName;

/* **AND THE PARAGRAPH STYLE, WHOSE VALUE IS A `CTParagraphStyleRef`** — the second key this tier ships, and
 * the one a FRAME reads: `CTFramesetterCreateFrame` asks the string's attributes at the paragraph's start
 * for this, and honours the alignment it finds (M4b). Apple's doc for it says the value must be a
 * `CTParagraphStyleRef`, which since M4b is an object like every other Core Text object. */
extern NSString *const kCTParagraphStyleAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6a: THE UNDERLINE — THE THIRD KEY, AND THE FIRST ONE THAT CHANGES WHAT A DRAWING LOOKS LIKE.
 *
 * Apple's doc block for it is short and complete: "Value must be a CFNumberRef. Default is
 * kCTUnderlineStyleNone. Set a value of something other than kCTUnderlineStyleNone to draw an underline."
 * So the value is a NUMBER (the style, optionally OR'd with a pattern modifier below) and the default is
 * nothing — which makes this the one attribute in this tier whose effect is VISIBLE, and therefore one a
 * PIXEL probe can hold to account.
 *
 * **AND THE NUMBERS IT DRAWS WITH ARE ALREADY HERE**: `CTFontGetUnderlinePosition` and
 * `CTFontGetUnderlineThickness` (M1d) are the face's own `post`-table metrics scaled to the point size, so
 * the underline's y and thickness come from the FACE rather than a constant — which is the kind of agreement
 * a probe can check by measuring the ink against those two doors.
 *
 * **THE PATTERNS ARE REFUSED BY NAME** (`kCTUnderlinePatternDot`, `…Dash`, `…DashDot`, `…DashDotDot`): a
 * dashed underline is a STROKED path with a dash pattern, while this tier's drawing fills through
 * `CGContextShowGlyphsAtPositions` — shipping the names and painting a solid line would be a wrong picture,
 * which is worse than a missing one. `Single`, `Thick` and `Double` ARE honoured, because each is one or two
 * filled rectangles.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTUnderlineStyleAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6c: THE COLOUR KEYS — THE FIRST ATTRIBUTES WHOSE VALUE IS A CORE GRAPHICS OBJECT.
 *
 * Which is possible NOW and was not before: a `CGColorRef` is a real Objective-C object since
 * `dec-c21da7fdccafc70b` (the user's second conversion decision), so **the value IS the colour** — no
 * wrapper, and no second convention in the tree. Three of Apple's five colour keys ship here:
 *
 *   * `kCTForegroundColorAttributeName` — "value must be a CGColorRef … used to fill the glyphs";
 *   * `kCTForegroundColorFromContextAttributeName` — a BOOLEAN: when true, the fill colour comes from the
 *     context and the colour key is ignored, which is this tier's behaviour when no colour is present at all;
 *   * `kCTUnderlineColorAttributeName` — the underline's own colour, which pairs with M6a's rule.
 *
 * **`kCTStrokeColorAttributeName` AND `kCTStrokeWidthAttributeName` ARE NAMED AND NOT SHIPPED**: stroked
 * text is a DIFFERENT DRAWING (the glyph OUTLINES, stroked with `CGContextStrokePath`) rather than a
 * different colour on the fill path — and this tier has the outline door (`cg_font_glyph_outline`) but not
 * the unit that drives it. Shipping the two names and filling instead would be a wrong picture, the same
 * refusal the underline's patterns got.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTForegroundColorAttributeName;
extern NSString *const kCTForegroundColorFromContextAttributeName;
extern NSString *const kCTUnderlineColorAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6d: THE THREE NUMERIC ATTRIBUTES — the era's words for each, because every one of them is a
 * CONTRACT and not a name:
 *
 *   * `kCTKernAttributeName` — "how many points the FOLLOWING character should be shifted from its
 *     default offset ... a positive kern indicates a shift farther along and a negative kern
 *     indicates a shift closer ... **If this attribute is set to 0.0, no kerning will be done at
 *     all**" (which is why an ABSENT attribute and a ZERO one mean different things here);
 *   * `kCTLigatureAttributeName` — "**0** ... only ligatures essential for proper rendering ..., **1**
 *     ... standard ligatures ..., **2** ... all available ligatures";
 *   * `kCTSuperscriptAttributeName` — "**if supported by the specified font**, a value of **1**
 *     enables superscripting and a value of **-1** enables subscripting" — the condition is the era's
 *     own, and the rise is a fraction of the font size here, recorded as a divergence.
 *
 * All three are `10_5_AND_LATER` in the era's header, so all three are in this tier's surface.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTKernAttributeName;
extern NSString *const kCTLigatureAttributeName;
extern NSString *const kCTSuperscriptAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6e: `kCTVerticalFormsAttributeName` — and the era's sentence draws a boundary that the NAME does
 * not: "Value must be a `CFBooleanRef`. Default is false. A value of false indicates that
 * **horizontal glyph forms** are to be used, true indicates that **vertical glyph forms** are to be
 * used." So this key selects the ALTERNATES a font carries for upright text — HarfBuzz's `vert`
 * feature — and it does **not** make text vertical: there is no stacking, no rotation and no
 * change of direction here, which is what the probe pins by requiring the layout to be undisturbed.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTVerticalFormsAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6f: `kCTGlyphInfoAttributeName` — "Allows the use of unencoded glyphs", and its sentence carries
 * TWO CONDITIONS that the implementation must honour both of:
 *
 *   "The glyph specified by this CTGlyphInfo object is assigned to the entire attribute range,
 *    PROVIDED THAT its contents match the specified base string AND that the specified glyph is
 *    available in the font specified by kCTFontAttributeName."
 *
 * So a glyph info whose base string does not match the range changes nothing, and neither does one
 * whose glyph the run's font does not carry. It is honoured while the line is BUILT, where the
 * range's own text is in hand — which is the first of the two conditions.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTGlyphInfoAttributeName;

/* ------------------------------------------------------------------------------------------------
 * M6g: THE STROKE PAIR. The width's sentence carries THREE BEHAVIOURS IN ONE NUMBER:
 *
 *   "Value must be a CFNumberRef. Default value is 0.0, or no stroke. This attribute, interpreted as
 *    a PERCENTAGE OF FONT POINT SIZE, controls the text drawing mode: POSITIVE VALUES EFFECT DRAWING
 *    WITH STROKE ONLY; NEGATIVE VALUES ARE FOR STROKE AND FILL. A typical value for outlined text is
 *    3.0."
 *
 * and the colour's one sentence is a default: "Default is the foreground color."
 *
 * Which is why a stroked run is a DIFFERENT DRAWING rather than a different colour: it is the glyph
 * OUTLINES as a path, stroked (and filled, when the width is negative), where the ordinary run is
 * the font's own glyph drawing.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTStrokeWidthAttributeName;
extern NSString *const kCTStrokeColorAttributeName;

/* THE STYLE: four values, and the era's own numbers (its `CTStringAttributes.h` lines 229-236). */
enum {
	kCTUnderlineStyleNone = 0x00,
	kCTUnderlineStyleSingle = 0x01,
	kCTUnderlineStyleThick = 0x02,
	kCTUnderlineStyleDouble = 0x09
};
typedef int32_t CTUnderlineStyle;

/* AND THE PATTERN MODIFIERS, which OR into the same number (its lines 248-256, values and all). */
enum {
	kCTUnderlinePatternSolid = 0x0000,
	kCTUnderlinePatternDot = 0x0100,
	kCTUnderlinePatternDash = 0x0200,
	kCTUnderlinePatternDashDot = 0x0300,
	kCTUnderlinePatternDashDotDot = 0x0400
};
typedef int32_t CTUnderlineStyleModifiers;

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTSTRINGATTRIBUTES_H */
