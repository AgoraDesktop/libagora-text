/*
 * CTFontMatching_internal.h — the seam for MATCHING: what faces a system has, and which of them a query describes.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * NOT PART OF CORE TEXT'S SURFACE. FOUR PUBLIC DOORS WAIT ON THIS FILE AND NONE OF THEM COULD BE WRITTEN WITHOUT IT:
 * `CTFontDescriptorCreateMatchingFontDescriptors` and its singular form, and `CTFontCollectionCreateMatchingFontDescriptors`
 * with its sorted variant. They live in two different files (the descriptor's and the collection's) and they ask the
 * SAME question — "which installed faces does this query describe" — so the answer lives here rather than in whichever
 * of the two was written first.
 *
 * AND THIS IS WHERE THE TIER'S TWO RECORDED DECISIONS LIVE, because a caller of either file must get the same ones:
 *
 *   THE SET is Apple's own rule, as far as its header prints it: a candidate MATCHES a query when EVERY attribute the
 *   query names is present in the candidate with an equal value, and A QUERY WITH NO ATTRIBUTES MATCHES EVERYTHING
 *   (which is what makes an empty descriptor list mean "all the faces there are"). Apple's matching is subtler than
 *   that — it involves traits, family-name matching and substitution — and the parts of it that its headers do not
 *   print are NOT invented here.
 *
 *   THE ORDER IS THIS TIER'S, AND IT IS THE USER'S DECISION (2026-10-11): Apple's documentation defers the order to
 *   "system precedence" and never says what precedence IS, so the array is sorted BY FAMILY NAME with
 *   `CTFontManagerCompareFontFamilyNames` — the comparator `CTFontManager` already offers, and itself a recorded
 *   deviation from Apple's UI order. THE POINT OF CHOOSING IT IS THAT IT IS STABLE AND REPRODUCIBLE: two runs of the
 *   same program see the same array, which is the property a caller's own sorting actually needs.
 */
#ifndef CORE_TEXT_CTFONtMATCHING_INTERNAL_H
#define CORE_TEXT_CTFONtMATCHING_INTERNAL_H

#include <CoreText/CTFontDescriptor.h>

/* EVERY FACE THE SYSTEM HAS, AS A DESCRIPTOR, IN THE ORDER ABOVE. Retained; the caller releases. The descriptors carry
 * the three attributes this tier can state as FACTS about a file — its URL, its PostScript name and its family — and
 * nothing invented. */
CFArrayRef CTFontMatchingCopyAvailableDescriptors(void);

/* THOSE OF THEM THAT THE QUERIES DESCRIBE. `queries` is an array of descriptors; an EMPTY array (or NULL) means every
 * face, which is the collection's "all available" question. The result is a subset of the available descriptors, in
 * the same order, with no duplicates. */
CFArrayRef CTFontMatchingCopyMatches(CFArrayRef queries);

#endif /* CORE_TEXT_CTFONtMATCHING_INTERNAL_H */
