/*
 * CTFontDescriptor_internal.h — the seam between the descriptor and `CTFont.c`, for the one thing the
 * descriptor's copy door needs and cannot declare for itself.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WHY THIS FILE EXISTS RATHER THAN A LOCAL `extern` IN THE CONSUMER, WHICH IS THE REASON THE OTHER INTERNAL
 * HEADERS EXIST: `CTLine_internal.h` and its neighbours are here "so that the two files agree about how a thing is
 * built instead of one of them `extern`ing what the other defines". The same seam is needed here and for the same
 * kind of reason.
 *
 * AND THE ONE THING IT CARRIES IS A CONSEQUENCE OF APPLE'S OWN INCLUDE GRAPH. The descriptor's copy door merges
 * `kCTFontFeatureSettingsAttribute` — Apple says so in as many words — and that key's value is an ARRAY OF SETTING
 * DICTIONARIES whose identity is the pair `kCTFontFeatureTypeIdentifierKey` / `kCTFontFeatureSelectorIdentifierKey`.
 * BOTH OF THOSE KEYS BELONG TO `CTFont.h`, WHICH INCLUDES `CTFontDescriptor.h` AND SO CANNOT BE INCLUDED BACK: a
 * merge written inside `CTFontDescriptor.c` cannot name the two keys it merges by, and a merge that guessed would
 * be worse than the absence it replaced.
 *
 * SO THE MERGE IS DEFINED WHERE ITS KEYS ARE VISIBLE — in `CTFont.c`, beside `CTFontDescriptorCreateCopyWithFeature`,
 * which is defined there for the same reason and is the door that made the merge reachable — and this file carries
 * the seam. IT IS NOT PART OF APPLE'S SURFACE and is named beside the other internal headers for that reason:
 * Apple's Core Text has no such door, a caller must not reach for one, and nothing in this library's own public
 * headers mentions it.
 */
#ifndef CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H
#define CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H

#include <CoreFoundation/CFArray.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MERGES TWO FEATURE-SETTINGS ARRAYS BY (TYPE, SELECTOR): a setting for a pair already present REPLACES it, and a
 * setting for a new pair is APPENDED. Answers a NEW array the caller owns, or NULL when one could not be made —
 * never the caller's own array, which would make the two callers disagree about who releases it. */
CFArrayRef CTFontDescriptorMergeFeatureSettings(CFArrayRef original, CFArrayRef additions);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H */
