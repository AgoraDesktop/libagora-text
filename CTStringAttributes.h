/*
 * CTStringAttributes.h — the attribute vocabulary: the keys a run of text carries its font, its colour and its
 * paragraph settings under.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), and it is the LAST of the vocabulary headers — the one Apple's
 * own include order puts beside `CTGlyphInfo` and `CTTextTab` at the end. It has no doors at all: a string attribute
 * is a KEY and the value is somebody else's object, so everything here is a constant or an enum.
 *
 * AND IT RETIRES TWO RECORDED BOUNDARIES, WHICH IS THE REASON IT WAS WRITTEN BEFORE THE OTHER TWO HEADERS BESIDE IT.
 * `CTLine.c` and `CTTypesetter.c` each carried the VALUE of an attribute key as a local literal, because the keys live
 * here and this header did not exist: `"NSFontAttributeName"` and `"NSParagraphStyleAttributeName"`. Both are now
 * `kCTFontAttributeName` and `kCTParagraphStyleAttributeName` READ FROM THIS HEADER, and the local copies are gone —
 * one value in one place, which is what those comments said they wanted the day this arrived.
 *
 * THE VALUES, AND TWELVE OF THE FOURTEEN ARE NOW PROVEN RATHER THAN INFERRED. The rule this tree established for the
 * font descriptor's keys applies to an ATTRIBUTE for the same reason it applies to a descriptor: **a dictionary
 * crosses between Core Text and the AppKit**, so a key's string has to be the one the other framework reads it by —
 * and the AppKit spells the same name with `NS` where Core Text spells it with `CT`. THE MEASUREMENT: Apple's
 * `NSAttributedString.h` at 10.6 declares TWENTY-SIX `NS…AttributeName` constants, and TWELVE of the fourteen keys
 * below have an exact counterpart there (the count is written beside each one below). TWO HAVE NONE —
 * `kCTForegroundColorFromContextAttributeName` and `kCTVerticalFormsAttributeName` — so those two keep the standing
 * the library's other unprovable values have: an inference from the measured rule, named as the first thing to check.
 *
 * AND ONE OF THESE KEYS IS WORTH KNOWING ABOUT BEFORE READING ITS VALUE: `kCTForegroundColorFromContextAttributeName`
 * and `kCTForegroundColorAttributeName` both CARRY A `CGColorRef` IN CORE TEXT, where the AppKit's counterpart is an
 * `NSColor` — a dictionary that crosses has to carry the same KIND of value, which is why the AppKit's
 * `NSForegroundColorAttributeName` is not evidence for the first of those two keys and is for the second.
 */
#ifndef CORE_TEXT_CTSTRINGATTRIBUTES_H
#define CORE_TEXT_CTSTRINGATTRIBUTES_H

#include <CoreFoundation/CFString.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------------------
 * THE FOURTEEN KEYS. Each carries the name of an object whose TYPE is the attribute's real contract: a `CTFontRef`
 * for the font, a `CGColorRef` for the colours, a `CTParagraphStyleRef` for the settings, a `CFNumberRef` for the
 * numbers and a `CTGlyphInfoRef` for the glyph substitution.
 * ------------------------------------------------------------------------------------- */
/* A `CTFontRef`. THE FACE THE RUN IS SET IN, and the one key every layer of this library reads: `CTLine` finds its
 * font by it, and `CTTypesetter` measures with the font it names. PROVABLE (1 of 12) — `NSFontAttributeName` is in the
 * AppKit's header. */
extern const CFStringRef kCTFontAttributeName;

/* A `CFBooleanRef`: when true the foreground colour comes from the CONTEXT's fill colour rather than from the
 * attribute beside it. INFERRED — the AppKit declares no counterpart for this key. */
extern const CFStringRef kCTForegroundColorFromContextAttributeName;

/* A `CFNumberRef`, in POINTS. PROVABLE (2 of 12) — `NSKernAttributeName`. */
extern const CFStringRef kCTKernAttributeName;

/* A `CFNumberRef`, 0 or 1: whether the font's own ligatures are used. PROVABLE (3 of 12) — `NSLigatureAttributeName`. */
extern const CFStringRef kCTLigatureAttributeName;

/* A `CGColorRef`. PROVABLE (4 of 12) — `NSForegroundColorAttributeName`. */
extern const CFStringRef kCTForegroundColorAttributeName;

/* A `CTParagraphStyleRef`. PROVABLE (5 of 12) — `NSParagraphStyleAttributeName`, and THIS IS THE SECOND OF THE TWO
 * BOUNDARIES THIS HEADER RETIRES: `CTTypesetter.c` read it as a literal because the line-break mode it carries is
 * what a breaker obeys. */
extern const CFStringRef kCTParagraphStyleAttributeName;

/* A `CFNumberRef`, a PERCENTAGE OF THE FONT SIZE — negative strokes, positive strokes and fills. PROVABLE (6 of 12). */
extern const CFStringRef kCTStrokeWidthAttributeName;

/* A `CGColorRef`, for the stroke above. PROVABLE (7 of 12) — `NSStrokeColorAttributeName`. */
extern const CFStringRef kCTStrokeColorAttributeName;

/* A `CFNumberRef` whose value is a `CTUnderlineStyle` with its modifiers (see the two enums below). PROVABLE
 * (8 of 12) — `NSUnderlineStyleAttributeName`. */
extern const CFStringRef kCTUnderlineStyleAttributeName;

/* A `CFNumberRef`: 1 raises the text, -1 lowers it, and 0 is normal. PROVABLE (9 of 12) — `NSSuperscriptAttributeName`. */
extern const CFStringRef kCTSuperscriptAttributeName;

/* A `CGColorRef`, for the underline above. PROVABLE (10 of 12) — `NSUnderlineColorAttributeName`. */
extern const CFStringRef kCTUnderlineColorAttributeName;

/* A `CFBooleanRef`: whether the glyphs are drawn in their VERTICAL forms. INFERRED — the AppKit declares no
 * counterpart for this key either, and Apple added this one for the vertical-text support Core Text has and this tier
 * does not (see `CTFramesetter.c` on progression). */
extern const CFStringRef kCTVerticalFormsAttributeName;

/* A `CTGlyphInfoRef`: a glyph substituted for a CHARACTER, which is the object `CTGlyphInfo.h` declares and this
 * header only names. PROVABLE (11 of 12) — `NSGlyphInfoAttributeName`. */
extern const CFStringRef kCTGlyphInfoAttributeName;

/* **A `CTRunDelegateRef`: the BOX a character occupies when it is not a glyph.** Attachment characters, inline
 * pictures and tab stops all set this over their placeholder; the line reserves the delegate's width instead of the
 * glyph's advance and draws no ink for it. INFERRED — the AppKit declares no counterpart for this key, because the
 * value is a Core Text object the AppKit has no class for — so the literal below is this tier's, spelled the way the
 * neighboring keys are. */
extern const CFStringRef kCTRunDelegateAttributeName;

/* A `CFNumberRef` naming a `kCharacterShapeType` shape, from the AAT `mort` feature vocabulary. PROVABLE (12 of 12) —
 * `NSCharacterShapeAttributeName`. */
extern const CFStringRef kCTCharacterShapeAttributeName;

/* ---------------------------------------------------------------------------------------
 * THE UNDERLINE VOCABULARY, WHICH IS TWO ENUMS THAT COMBINE: a style says what kind of line it is and a pattern
 * modifier says how it is drawn, and they are OR-ed together into the one number
 * `kCTUnderlineStyleAttributeName` carries. THE VALUES ARE APPLE'S AND THERE IS A GAP IN THE FIRST ONE — the shapes
 * are 0x00, 0x01, 0x02 and 0x09, with 0x03 through 0x08 unassigned — which is why they are named here rather than
 * counted, and why the suite asserts the gap as well as the numbers.
 * ------------------------------------------------------------------------------------- */
enum {
	kCTUnderlineStyleNone = 0x00,
	kCTUnderlineStyleSingle = 0x01,
	kCTUnderlineStyleThick = 0x02,
	kCTUnderlineStyleDouble = 0x09	/* the gap: 0x03 through 0x08 are not underline styles */
};
typedef int32_t CTUnderlineStyle;

/* AND THE PATTERNS ARE IN THE HIGH BYTE, which is what makes the OR possible: a style is a low byte and a pattern
 * selects bits above it. */
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
