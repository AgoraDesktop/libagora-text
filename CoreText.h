/*
 * CoreText.h — the umbrella, and the version.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **APPLE'S UMBRELLA IS A LIST OF INCLUDES AND A VERSION SECTION, AND THIS IS BOTH.** The fourteen includes
 * below are Apple's own, in Apple's order (10.6's `CoreText.h`, less `AvailabilityMacros.h`, which is Apple's
 * toolchain's and not a Core Text header) — so `#include <CoreText/CoreText.h>` means here what it means there.
 * The `_internal.h` headers next door are deliberately NOT included: they are a seam between the two languages
 * this library is written in, not part of the surface.
 *
 * **THE VERSION IS THE ONE THING IN THIS UMBRELLA THAT IS NOT AN INCLUDE**, and its values are APPLE'S — which
 * makes this the third vocabulary in this framework whose numbers the era publishes rather than leaving to us
 * (with `CTFontUIFontType`, `CTCharacterCollection` and `CTFontPriority` in §51). They are a SEQUENCE, and the sequence
 * is the interface: the era's own doc block tells a caller to compare (`CTGetCoreTextVersion() >=
 * kCTVersionNumber10_5`), so a number that did not sit where Apple's sits would make every such comparison
 * wrong in a way no single value could reveal.
 *
 * **AND ONE NAME HERE HAS NO LEDGER ROW, WHICH IS THE INSTRUMENT'S BLIND SPOT SEEN FROM THE OTHER SIDE.**
 * `kCTVersionNumber10_6_7` is declared by 10.6's own header and appears in NO Apple index entry, so the ledger —
 * whose universe is Apple's index — never made a row for it and `--work-list` could never have named it. §46
 * and §51 turned up the opposite defect three times (OUR names that Apple does not have); this is an APPLE name
 * the index drops, which means **the ledger UNDERSTATES the era surface wherever the index is thinner than the
 * headers**. It ships here because it is Apple's, and it is recorded because the worth of a row count depends on
 * knowing what the count leaves out.
 */
#ifndef CORE_TEXT_CORETEXT_H
#define CORE_TEXT_CORETEXT_H

#include <CoreText/CTFont.h>
#include <CoreText/CTFontCollection.h>
#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFontManager.h>
#include <CoreText/CTFontTraits.h>
#include <CoreText/CTFrame.h>
#include <CoreText/CTFramesetter.h>
#include <CoreText/CTGlyphInfo.h>
#include <CoreText/CTLine.h>
#include <CoreText/CTParagraphStyle.h>
#include <CoreText/CTRun.h>
#include <CoreText/CTStringAttributes.h>
#include <CoreText/CTTextTab.h>
#include <CoreText/CTTypesetter.h>

#ifdef __cplusplus
extern "C" {
#endif

/* **"RETURNS THE VERSION OF THE CoreText FRAMEWORK" — AND THIS TIER'S ANSWER IS THE ERA IT IMPLEMENTS.** The
 * surface here is pinned to 10.6 (§2), so the number is `kCTVersionNumber10_6`; a caller that compares it
 * against the constants below gets the truth about which doors exist, which is the whole purpose the era's doc
 * block gives the function ("framework version is not always an accurate indicator of feature availability ...
 * the recommended way ... is ... comparing its result to a defined constant"). */
uint32_t CTGetCoreTextVersion(void);

/* THE CONSTANTS, WITH APPLE'S VALUES. They are a SEQUENCE and the sequence is what a caller compares against. */
#define kCTVersionNumber10_5 0x00020000
#define kCTVersionNumber10_5_2 0x00020001
#define kCTVersionNumber10_5_3 0x00020002
#define kCTVersionNumber10_5_5 0x00020003
#define kCTVersionNumber10_6 0x00030000
#define kCTVersionNumber10_6_7 0x00030007

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CORETEXT_H */
