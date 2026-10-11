/*
 * CTFontDescriptor_internal — the seam between a descriptor and the font that is made from it.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * A `CTFontDescriptor` IS AN ATTRIBUTES DICTIONARY AND A REFERENCE COUNT, and that is the whole object:
 * Apple's own door says the dictionary "can contain arbitrary attributes that will be preserved", and
 * what the framework does with them is MATCH A FONT. The matching lives in `CTFontCreateWithFontDescriptor`
 * (`CTFont.c`), which is plain C — so this header is the third of the same three seams this library
 * carries (`CTFont_internal.h`, and CoreGraphics' two): the C half asks for the three attributes it can
 * act on BY NAME, and the Objective-C half that owns the dictionary answers them.
 *
 * AND THE THREE ASKED FOR ARE EXACTLY THE THREE THIS TIER CAN ACT ON — name, size, matrix. Everything
 * else in the dictionary is PRESERVED (the round trip through `CTFontDescriptorCopyAttributes` returns
 * it) and IGNORED at font creation, which is not a shortcut but Apple's own sentence: "unrecognized
 * attributes will be ignored on font creation and may not be preserved over the round trip".
 */
#ifndef CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H
#define CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H

#include <CoreText/CTFontDescriptor.h>
/* `CTFontRef` IS NAMED BY THE DOOR AT THE BOTTOM, so `CTFont.h` is included here — and the include is
 * safe in both directions because both headers guard themselves and `CTFont.h` includes THIS file's
 * public half already. */
#include <CoreText/CTFont.h>
#include <CoreGraphics/CGAffineTransform.h>
#include <stddef.h>
/* AND THE TWO C HEADERS THE DOORS ABOVE ARE IMPLEMENTED WITH, HERE RATHER THAN IN THE FILE THAT
 * IMPLEMENTS THEM: every implementation of `ctf_descriptor_name_utf8` needs `strlen`/`memcpy`, and the
 * object's own lifetime — which is the CONTRACT this seam declares, not an implementation detail —
 * needs `calloc`/`free`. A declaration and the declarations its contract requires belong together, and
 * the alternative cost a build: the Objective-C half included this header and nothing else, so the first
 * version of it compiled with `calloc`, `free`, `strlen` and `memcpy` called as IMPLICIT functions
 * (which C99 made an ERROR, not a warning). */
#include <stdlib.h>
#include <string.h>

/* THE OBJECT'S LAYOUT IS NO LONGER HERE, AND THAT IS THE POINT OF THE 2026-10-09 DECISION
 * (`dec-35c114bf75c7c2d7`): a `CTFontDescriptor` is a real Objective-C object now, so its ivars belong
 * to the CLASS (declared in `CTFontDescriptor.m`, where they are read) and this seam keeps only what a C
 * caller may ask for. A struct here would be a second declaration of one layout, and the runtime's
 * non-fragile ABI does not promise that the two would agree. */

/* THE DESCRIPTOR'S POSTSCRIPT NAME, UTF-8 INTO `buf`: the number of bytes written, 0 when the
 * descriptor carries no `kCTFontNameAttribute` (or carries one that is not a string). */
int ctf_descriptor_name_utf8(CTFontDescriptorRef descriptor, char *buf, size_t size);

/* THE POINT SIZE, 1 WHEN THE DESCRIPTOR SPECIFIES ONE and 0 when it does not — which is a distinction
 * Apple's own door makes: `CTFontDescriptorCreateWithNameAndSize` OMITS the size attribute entirely
 * when 0.0 is passed, so "absent" is a real state and not a zero. */
int ctf_descriptor_size(CTFontDescriptorRef descriptor, CGFloat *size);

/* THE MATRIX, 1 WHEN THE DESCRIPTOR SPECIFIES ONE. Packaged the way Apple documents it (the
 * `kCTFontMatrixAttribute`'s six `CGFloat`s as data), and absent means the identity. */
int ctf_descriptor_matrix(CTFontDescriptorRef descriptor, CGAffineTransform *matrix);

/* THE DESCRIPTOR A FONT WAS MADE FROM, built from the three attributes above — this is how
 * `CTFontCopyFontDescriptor` answers a font that was hand-made from a graphics font and a size, which
 * has no descriptor of its own until it is asked for one. */
CTFontDescriptorRef ctf_descriptor_from_font(CTFontRef font);

/* **THE ENABLE/DISABLE SET (CT-M7k), AND IT LIVES HERE BECAUSE THE MATCHER DOES.** Apple's
 * `CTFontManagerEnableFontDescriptors` "enables or disables the matching font descriptors for font descriptor
 * matching", so the set it maintains is consulted by `CTFontDescriptorCreateMatchingFontDescriptors` — which is
 * this file — and a set owned by the font manager would be a second list the matcher could miss. The two rules
 * of the relationship are: a MATCH never answers a disabled descriptor, and DISABLING never changes what is
 * AVAILABLE (the enumeration and the name lookup are untouched — Apple's own word is "matching").
 *
 * The set is keyed by the descriptor's NAME (its PostScript name, the same string `ctf_descriptor_name_utf8`
 * reports), because that is what a caller's descriptor and the matcher's candidate share: a caller disables
 * what a match gave it. A descriptor with no resolvable name changes nothing, for the same reason the CID door
 * refuses: an identity that cannot be stated cannot be remembered. */
void ctf_descriptor_set_enabled(NSArray *descriptors, BOOL enable);
int ctf_descriptor_name_is_enabled(const char *name);

#endif /* CORE_TEXT_CTFONTDESCRIPTOR_INTERNAL_H */
