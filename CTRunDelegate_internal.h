/*
 * CTRunDelegate_internal.h — the three answers the layout asks a delegate, without a cast to its callbacks struct.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER. `CTRunDelegateGetRefCon` is Apple's door and
 * the reason a caller sets a delegate at all — it is how the LAYOUT reads the attachment back — but the engine itself
 * needs the three NUMBERS, and reaching them by casting the object here would put the struct's shape in two files.
 * So the seam is a header, the same way `CTRun::CTRunCreateWithAttributesAndGlyphs` and `CTLine`'s range creator are.
 */
#ifndef CORE_TEXT_CTRUNDELEGATE_INTERNAL_H
#define CORE_TEXT_CTRUNDELEGATE_INTERNAL_H

#include <CoreText/CTRunDelegate.h>

/* THE WIDTH, ASCENT AND DESCENT THE DELEGATE ANSWERS, or zero when it has no callback for one of them. A NULL
 * delegate answers zero for all three, so a caller that reads the attribute and finds nothing gets a box of no width
 * rather than a crash. */
CGFloat CTRunDelegateWidth(CTRunDelegateRef runDelegate);
CGFloat CTRunDelegateAscent(CTRunDelegateRef runDelegate);
CGFloat CTRunDelegateDescent(CTRunDelegateRef runDelegate);

#endif /* CORE_TEXT_CTRUNDELEGATE_INTERNAL_H */
