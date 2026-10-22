/*
 * CTStringAttributes.c — the fourteen keys, and nothing else: the two enums are in the header.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * TWELVE OF THESE VALUES ARE PROVEN AND TWO ARE INFERRED, AND WHICH IS WHICH IS THE POINT OF THIS FILE. The proof is
 * the crossing rule the font descriptor's keys were proven by, applied here to a measurement of its own: Apple's
 * `NSAttributedString.h` at 10.6 declares twenty-six `NS…AttributeName` constants, and the twelve keys marked
 * "PROVABLE" below each have an exact counterpart there — a dictionary crosses between the frameworks, so a key's
 * string has to be the one the OTHER framework reads it by, which makes `"NSFontAttributeName"` a fact about
 * `kCTFontAttributeName` rather than a guess at it.
 *
 * THE TWO WITHOUT A COUNTERPART KEEP THE LIBRARY'S OTHER STANDING: an inference from that measured rule, with the name
 * first on the list of things to check when something can answer for them. They are `…ForegroundColorFromContext…`
 * and `…VerticalForms…`, and the REASON each has none is stated beside it.
 */
#include <CoreText/CTStringAttributes.h>

/* THE FONT, AND THE FIRST OF THE TWO VALUES THIS HEADER TOOK OVER FROM A LOCAL LITERAL. `CTLine.c` read this key to
 * find the face a line is set in and `CTTypesetter.c` to find the one it measures with; both now read THIS symbol,
 * which is why the value exists in exactly one place in the library. PROVABLE: `NSFontAttributeName`. */
const CFStringRef kCTFontAttributeName = CFSTR("NSFontAttributeName");

/* INFERRED: the AppKit declares no `NSForegroundColorFromContextAttributeName`, so this key has no counterpart to be
 * proven against. It takes the same form by inference, and it is the first thing to check. */
const CFStringRef kCTForegroundColorFromContextAttributeName = CFSTR("NSForegroundColorFromContextAttributeName");

const CFStringRef kCTKernAttributeName = CFSTR("NSKernAttributeName");

const CFStringRef kCTLigatureAttributeName = CFSTR("NSLigatureAttributeName");

const CFStringRef kCTForegroundColorAttributeName = CFSTR("NSForegroundColorAttributeName");

/* THE PARAGRAPH SETTINGS, AND THE SECOND VALUE THIS HEADER TOOK OVER: the line-break mode a typesetter obeys lives
 * under this key, and `CTTypesetter.c` carried its spelling as a literal until now. PROVABLE:
 * `NSParagraphStyleAttributeName`. */
const CFStringRef kCTParagraphStyleAttributeName = CFSTR("NSParagraphStyleAttributeName");

const CFStringRef kCTStrokeWidthAttributeName = CFSTR("NSStrokeWidthAttributeName");

const CFStringRef kCTStrokeColorAttributeName = CFSTR("NSStrokeColorAttributeName");

const CFStringRef kCTUnderlineStyleAttributeName = CFSTR("NSUnderlineStyleAttributeName");

const CFStringRef kCTSuperscriptAttributeName = CFSTR("NSSuperscriptAttributeName");

const CFStringRef kCTUnderlineColorAttributeName = CFSTR("NSUnderlineColorAttributeName");

/* INFERRED, and for a reason that is itself a fact about the era: this key exists for the VERTICAL-text support Core
 * Text has and the AppKit's attributed-string vocabulary does not name — the same support `CTFrame`'s progression
 * records as not honoured here. */
const CFStringRef kCTVerticalFormsAttributeName = CFSTR("NSVerticalFormsAttributeName");

const CFStringRef kCTGlyphInfoAttributeName = CFSTR("NSGlyphInfoAttributeName");

/* THE ONE KEY WITH NO APPKIT COUNTERPART IN THIS GROUP, so its literal is this tier's and is spelled the way the
 * others are. The AppKit half sets the SAME constant (its `NSLayoutManager` includes this header) rather than a
 * literal of its own, so the two frameworks read the box by one name. */
const CFStringRef kCTRunDelegateAttributeName = CFSTR("CTRunDelegateAttributeName");

const CFStringRef kCTCharacterShapeAttributeName = CFSTR("NSCharacterShapeAttributeName");
