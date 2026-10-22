/*
 * CTRunDelegate.h — THE BOX: a delegate that says how much room a character wants when it is not a glyph.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md). **A RUN DELEGATE IS HOW A CHARACTER THAT IS NOT A GLYPH GETS A
 * SIZE.** Attachment characters, inline images, text fields in a document and tab stops are all the same problem: a
 * placeholder character occupies the string and something ELSE occupies the width, and the layout engine has to know
 * how wide that something is. Apple's answer is this object — the caller supplies three callbacks (ascent, descent,
 * width) and a reference constant, and sets the delegate as the value of `kCTRunDelegateAttributeName` over the
 * character; the line breaker and the line then reserve that width instead of the glyph's advance.
 *
 * **AND THE LAYOUT HONOURS IT.** `CTLine` marks such a character as a BOX: no ink is drawn for it, its advance is the
 * delegate's width, the line's ascent and descent grow to cover the delegate's, and every glyph after it is shifted —
 * so a `CTRun` still has one entry per character, `CTLineGetOffsetForStringIndex` still maps a character to an offset,
 * and `CTLineDraw` and `CTRunDraw` simply skip the boxes. WHAT IS STILL OWED IS NAMED WHERE IT WOULD BE: a box in a
 * JUSTIFIED or TRUNCATED line keeps its place but not its own delegate (`ct_line_with_positions` copies glyphs and
 * positions, not the box flags), and a box does not yet take part in `CTTypesetter`'s kerning neighbours.
 */
#ifndef CORE_TEXT_CTRUNDELEGATE_H
#define CORE_TEXT_CTRUNDELEGATE_H

#include <CoreFoundation/CFBase.h>
#include <CoreGraphics/CGGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE OBJECT. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTRunDelegateGetTypeID()`. */
typedef const struct __CTRunDelegate *CTRunDelegateRef;

/* THE THREE QUESTIONS THE LAYOUT ASKS, each `refCon` being whatever the caller passed to `CTRunDelegateCreate` — the
 * attachment, the tab stop, the picture. Apple's signatures and their order are kept. */
typedef CGFloat (*CTRunDelegateGetAscentCallback)(void *refCon);
typedef CGFloat (*CTRunDelegateGetDescentCallback)(void *refCon);
typedef CGFloat (*CTRunDelegateGetWidthCallback)(void *refCon);

/* THE CALLBACKS, VERSIONED `0` AS APPLE'S IS. A NULL `getWidth` would leave the box with no width, which is why
 * `CTRunDelegateCreate` refuses a NULL callbacks pointer and treats a missing width as zero rather than guessing. */
typedef struct {
	CFIndex version;
	CTRunDelegateGetAscentCallback getAscent;
	CTRunDelegateGetDescentCallback getDescent;
	CTRunDelegateGetWidthCallback getWidth;
} CTRunDelegateCallbacks;

CFTypeID CTRunDelegateGetTypeID(void);

/* MAKE ONE. The callbacks are COPIED (the struct, not what its pointers point at) and `refCon` is kept as given, so
 * the caller's object must outlive the delegate — the same borrowed-reference rule a run delegate exists to carry. */
CTRunDelegateRef CTRunDelegateCreate(const CTRunDelegateCallbacks *callbacks, void *refCon);

/* AND THE REFERENCE CONSTANT, asked back — which is how a layout that receives a delegate reads the attachment it
 * stands for. Apple's door. */
void *CTRunDelegateGetRefCon(CTRunDelegateRef runDelegate);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTRUNDELEGATE_H */
