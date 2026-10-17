/*
 * CTLineBreakThai_internal.h — the UAX #14 `SA` tailoring for Thai: where a DICTIONARY says a word ends, which is
 * the one thing the rule list cannot answer.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * NOT PART OF CORE TEXT'S SURFACE, for the reason `CTLineBreak_internal.h` gives for itself: Apple publishes no door
 * for line breaking, the algorithm runs inside the typesetter, and this tier's rules and this tier's typesetter are
 * two files that must agree.
 *
 * **WHY THE RULES ALONE ARE NOT ENOUGH HERE, IN THE SPECIFICATION'S OWN WORDS.** UAX #14 §6.1, "Tailoring", says
 * that the `SA` class — "Complex context dependent (South East Asian) scripts" — "require[s] morphological analysis to
 * determine break opportunities", and §LB1 resolves `SA` to `AL` for want of that analysis. `AL`'s own rule forbids a
 * break inside an alphabetic run, so a Thai sentence is ONE unbreakable word as far as the rule list is concerned:
 * MEASURED, a Thai line does not wrap at all here, and a word longer than its container takes the
 * word-doesn't-fit fallback. That is the specification's answer to an unanalysed script and not this tier's gap, but
 * it is a poor answer for Thai, which is written without spaces between words.
 *
 * **AND THE ANALYSIS IS A DICTIONARY, WHICH IS DATA THIS TIER DOES NOT OWN — SO IT IS READ AT RUNTIME, NEVER
 * VENDORED.** The resource is `libthai`'s word-break trie (`/usr/share/libthai/thbrk.tri`, `libthai-data` 0.1.29 on
 * the machine this was measured on), which is `LGPL-2.1-or-later` — the same standing `libagora-core/CFHyphenation.c`
 * has with the `hyphen-*` pattern files, and for the same reason: a copyleft data set may be USED by a program that
 * reads it, and may not be shipped inside this tree. There is therefore NO TABLE IN THIS REPOSITORY for Thai, and
 * `tools/gen-linebreak-data.py`'s output is untouched by this file.
 *
 * **AND WHAT THE FILE IS IS LIBDATRIE'S, WHICH IS A FORMAT AND NOT A LIBRARY DEPENDENCY.** `thbrk.tri` is a
 * double-array trie serialised by libdatrie: three sections, each introduced by a big-endian signature, with a
 * one-byte alphabet mapped through an explicit range list. `CTLineBreakThai.c` reads those three sections itself —
 * 588,096 bytes here — so this tier gains a file format and no link-time dependency. The format is recorded in that
 * file's header where a reader looks for it.
 *
 * **AND THE SEARCH IS GREEDY LONGEST MATCH, WHICH IS NOT WHAT LIBTHAI DOES.** libthai's own breaker
 * (`src/thbrk/brk-maximal.c`) keeps a SET of candidate paths and contests them with penalties for recovering from a
 * failed walk. That algorithm is not reproduced here — it is 699 lines of contest bookkeeping, and reproducing it
 * would be copying rather than implementing — so this file does the simple thing the dictionary supports: at each
 * position take the longest dictionary word, and if none starts there advance one character. **MEASURED against
 * `th_brk()` (libthai 0.1.29) over 800 runs of real Thai prose from a Wikipedia article, 24,520 characters: 774 runs
 * identical, 26 different.** The differences are where the contest picks a different candidate path, and they are
 * recorded in `README.md` beside this slice rather than smoothed over.
 */
#ifndef CORE_TEXT_CTLINEBREAKTHAI_INTERNAL_H
#define CORE_TEXT_CTLINEBREAKTHAI_INTERNAL_H

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

/* WHETHER THE DICTIONARY COULD BE LOADED. False on a machine without libthai's data, and then `ct_thai_breaks` does
 * nothing and the caller keeps the rule list's answer — `SA` staying `AL` — which is what this tier did before this
 * file existed. ASK IT ONCE PER RUN rather than per position: the first call is what reads 588 KB. */
bool ct_thai_available(void);

/* **WHERE THE DICTIONARY PUTS A WORD BOUNDARY.** `breaks[i]` is set to 1 when a Thai word ends at index `i` — that is,
 * before the character at `i` — for `1 <= i < count`, and `breaks[0]` is never set, because there is nothing to break
 * before at the start of a run. The caller passes a run of code points; characters outside the trie's alphabet make
 * the match fail there and one character is skipped, which is how a run of mixed script is walked.
 *
 * THE DOOR IS FED A WHOLE RUN RATHER THAN ASKED ONE POSITION AT A TIME because a line breaker asks about every
 * position in order, and a per-position door would re-walk the dictionary from every one of them.
 *
 * TIES ARE NOT POSSIBLE: the answer at a position is the LONGEST word starting there, so a shorter word that is also
 * a prefix of it never ends the walk. */
void ct_thai_breaks(const UniChar *chars, CFIndex count, uint8_t *breaks);

#endif /* CORE_TEXT_CTLINEBREAKTHAI_INTERNAL_H */
