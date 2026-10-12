/*
 * CTTextTab.h — the tab: one stop in a paragraph's tab vocabulary.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), whose include list is the whole story of this type:
 * `CTParagraphStyle.h` — for `CTTextAlignment`, because a tab's alignment is the SAME VOCABULARY a paragraph's is —
 * and Core Foundation's character-set and dictionary types. Five doors: a type id, a creator, and three getters.
 *
 * AND THIS TYPE IS WHY `CTParagraphStyle` HAS ONE ROW THAT ANSWERED NOTHING. Apple's header for that type prints the
 * default for `kCTParagraphStyleSpecifierTabStops` as "12 left-aligned tabs, spaced by 28.0 points" — and A TAB IS A
 * `CTTextTabRef`, so the default could not be built while this header was unwritten. The row answered an EMPTY ARRAY,
 * recorded as a deviation with its landing point named; THAT IS NOW PAID, and `CTParagraphStyle.c`'s fallback builds
 * the twelve tabs Apple names.
 */
#ifndef CORE_TEXT_CTTEXTTAB_H
#define CORE_TEXT_CTTEXTTAB_H

#include <CoreText/CTParagraphStyle.h>

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFDictionary.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE TAB. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTTextTabGetTypeID()`. */
typedef const struct __CTTextTab *CTTextTabRef;

CFTypeID CTTextTabGetTypeID(void);

/* MAKE ONE. THE ALIGNMENT IS THE PARAGRAPH'S OWN VOCABULARY (see `CTTextAlignment` in CTParagraphStyle.h) because a
 * tab stop and a paragraph are aligned the same ways; the LOCATION IS IN POINTS along the line; and the OPTIONS are a
 * per-tab dictionary this tier RETAINS AND DOES NOT READ — Apple names no key for it anywhere in the era's headers, so
 * there is nothing to obey. */
CTTextTabRef CTTextTabCreate(CTTextAlignment alignment, double location, CFDictionaryRef options);

CTTextAlignment CTTextTabGetAlignment(CTTextTabRef tab);

double CTTextTabGetLocation(CTTextTabRef tab);

/* THE OPTIONS AS THEY WERE GIVEN, borrowed, or NULL — never a dictionary this library manufactured. */
CFDictionaryRef CTTextTabGetOptions(CTTextTabRef tab);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTTEXTTAB_H */
