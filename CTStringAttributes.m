/*
 * CTStringAttributes.m — the value of the one attribute name this tier ships.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WHY A FILE FOR ONE CONSTANT. The value is OURS, for the fourth time and the same reason the name keys
 * and the attribute keys already carry: Apple publishes the NAME of each attribute and the TYPE of its
 * value, never the string — and a name is only ever compared with the constant declared beside it, which
 * is what makes an unpublished value safe. A `.m` rather than a `.c` because the constant IS an
 * `NSString` and a string literal is a Foundation object; `CGBase.h` teaches a C consumer to spell the
 * type, exactly as it does for the font's own names.
 */
#import <Foundation/Foundation.h>

#include <CoreText/CTStringAttributes.h>

/* THE ONE NAME THIS UNIT SHIPS. Apple's doc block for it is one sentence — "the value must be a
 * CTFontRef" — and since CT-M1e-b that value is a real Objective-C object, so it goes into an
 * attributed string as itself. THE OTHER THIRTEEN NAMES OF APPLE'S HEADER ARE THE M6 UNIT and are named
 * there, not forgotten here. */
NSString *const kCTFontAttributeName = @"CTFont";

/* AND THE PARAGRAPH STYLE (M4b): its value is a `CTParagraphStyleRef`, and it is the key that lets a STRING
 * carry a paragraph's alignment — which is what `CTFramesetterCreateFrame` reads. The value is ours for the
 * same reason as its neighbours' (Apple publishes names, not strings). */
NSString *const kCTParagraphStyleAttributeName = @"CTParagraphStyle";

/* AND THE UNDERLINE (M6a): a NUMBER in the string, whose effect is visible — see the header for what is
 * honoured (Single/Thick/Double, solid) and what is refused by name (the patterns). */
NSString *const kCTUnderlineStyleAttributeName = @"CTUnderlineStyle";

/* AND THE THREE COLOUR KEYS (M6c): their values are `CGColor` OBJECTS, which is what
 * `dec-c21da7fdccafc70b` bought. Values are ours, as for every key here. */
NSString *const kCTForegroundColorAttributeName = @"CTForegroundColor";
NSString *const kCTForegroundColorFromContextAttributeName = @"CTForegroundColorFromContext";
NSString *const kCTUnderlineColorAttributeName = @"CTUnderlineColor";

/* AND THE THREE NUMERIC KEYS (M6d). Values are ours, as for every key here. */
NSString *const kCTKernAttributeName = @"CTKern";
NSString *const kCTLigatureAttributeName = @"CTLigature";
NSString *const kCTSuperscriptAttributeName = @"CTSuperscript";

/* AND THE VERTICAL-FORMS FLAG (M6e), whose value is a BOOLEAN. */
NSString *const kCTVerticalFormsAttributeName = @"CTVerticalForms";

/* AND THE UNENCODED-GLYPH KEY (M6f), whose value is a `CTGlyphInfo` OBJECT. */
NSString *const kCTGlyphInfoAttributeName = @"CTGlyphInfo";

/* AND THE STROKE PAIR (M6g): a NUMBER whose sign picks the drawing mode, and a COLOUR. */
NSString *const kCTStrokeWidthAttributeName = @"CTStrokeWidth";
NSString *const kCTStrokeColorAttributeName = @"CTStrokeColor";
