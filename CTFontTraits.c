/*
 * CTFontTraits.c — the four trait keys, which are the only things in this header with a value at run time.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * THE STRINGS ARE THE APPKIT'S, and that is deliberate rather than borrowed: `NSFontSymbolicTrait` and its
 * three siblings are the names Foundation uses for the same four traits, because a traits dictionary crosses
 * between the two frameworks. A traits dictionary this library answers with can therefore be read by
 * Foundation's own tools, and one Foundation made can be read here.
 *
 * READ FROM APPLE'S HEADER FOR THE NAMES AND FROM THE APPKIT'S VOCABULARY FOR THE VALUES — the header
 * declares them `extern const CFStringRef` and does not print the strings (PROVENANCE.md records where the
 * values came from and why they are facts rather than choices).
 */
#include <CoreText/CTFontTraits.h>

const CFStringRef kCTFontSymbolicTrait = CFSTR("NSFontSymbolicTrait");
const CFStringRef kCTFontWeightTrait = CFSTR("NSFontWeightTrait");
const CFStringRef kCTFontWidthTrait = CFSTR("NSFontWidthTrait");
const CFStringRef kCTFontSlantTrait = CFSTR("NSFontSlantTrait");
