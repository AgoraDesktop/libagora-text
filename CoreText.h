/*
 * CoreText.h — the umbrella: every header of this framework, and the one door that belongs to none of them.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), AND IT IS THE LAST HEADER IN THE ORDER FOR A REASON THAT IS NOT
 * ALPHABETICAL: an umbrella is what a caller writes when they want the framework rather than a part of it, so it can
 * only be written once there IS a framework — fourteen headers, in Apple's own order, which here is also alphabetical
 * because Apple's is.
 *
 * AND ONE HEADER IS CONSPICUOUS BY ITS ABSENCE: `CTFontManagerErrors.h` IS NOT IN APPLE'S LIST, and it is not in this
 * one either. A caller who includes this umbrella still HAS it — `CTFontManager.h` includes it, which is why that
 * header was written first among its neighbours — so the omission is Apple's and it is reproduced rather than
 * repaired. Naming it here would be a kindness that made this file disagree with the framework it duplicates.
 *
 * THE ONE DOOR IS THE ONE NO OTHER HEADER COULD DECLARE: a version number is a fact about the FRAMEWORK, so it belongs
 * to the file that means the whole framework. `CTGetCoreTextVersion` answers the version of the surface THIS LIBRARY
 * implements, and the six constants it is compared against are Apple's published values, written as the macros Apple
 * writes them as. See `CoreText.c` for why the answer is `kCTVersionNumber10_6` rather than a number of this
 * repository's own.
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE VERSION OF THE FRAMEWORK, for comparison with the constants below — and Apple's own note about the door is
 * worth keeping because it is about the ADVERTISED number rather than the code: "framework version is not always an
 * accurate indicator of feature availability", so the recommended use is to check the function pointer and then
 * compare its result, which is exactly what the suite does. */
uint32_t CTGetCoreTextVersion(void);

/* APPLE'S PUBLISHED VERSION NUMBERS, AS APPLES WRITES THEM — as MACROS, because that is what they are: a caller
 * compares `CTGetCoreTextVersion()` against them without linking anything, so they are not symbols and this library
 * does not declare them as any. THE VALUES ARE THE ERA'S, and the two 10.6 entries differ in their last digit for the
 * same reason the 10.5 ones do — the framework's number moved without the API surface changing. */
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
