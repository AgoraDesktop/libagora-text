/*
 * CTFontManagerErrors.c — the two strings, and nothing else: the codes are an enum in the header.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * EACH VALUE IS AN INFERENCE, AND THE REASON IS RECORDED IN THE HEADER IN FULL: Apple publishes no value for
 * either constant, and the AppKit at 10.6 declares no counterpart to cross to — `NSFontManager.h` declares no
 * `extern const …StringRef` at all. Both therefore take the form of the constant's own name with the leading `k`
 * dropped, which is what Apple's newer Core domains look like, and both are the first thing to check when
 * something can answer for them.
 */
#include <CoreText/CTFontManagerErrors.h>

const CFStringRef kCTFontManagerErrorDomain = CFSTR("CTFontManagerErrorDomain");

const CFStringRef kCTFontManagerErrorFontURLsKey = CFSTR("CTFontManagerErrorFontURLsKey");
