/*
 * CTTextTab.h — a tab stop in a paragraph style.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **APPLE'S OWN HEADER AND APPLE'S OWN SENTENCE**: "A CTTextTab represents a tab in an CTParagraphStyle
 * object, storing an alignment type and location. CoreText supports four alignment types: left, center, right,
 * and decimal. These alignment types are absolute, not based on the line sweep direction of text. … A tab's
 * location, on the other hand, is relative to the BACK margin."
 *
 * **SO THE ALIGNMENT IS THIS TIER'S EXISTING `CTTextAlignment`** (declared in `CTParagraphStyle.h`, which
 * 10.6 also uses for it — the era declares NO alignment constants in this header at all, and the "decimal"
 * type the sentence names is not in this era's surface). The absolute-not-directional rule costs nothing here
 * because this tier's layout is left-to-right only and refuses a base writing direction by name.
 *
 * **`CTTextTabGetTypeID` IS DECLARED NOW** (CT-M7g). This paragraph said it "answers a `CFTypeID` and this
 * tier has no Core Foundation, so the name stays owed on the ledger rather than shipping as a number with no
 * meaning" — and `CTTypeID_internal.h` answers both halves.
 */
#ifndef CORE_TEXT_CTTEXTTAB_H
#define CORE_TEXT_CTTEXTTAB_H

#include <CoreGraphics/CGBase.h>
#include <CoreText/CTParagraphStyle.h>

/* THE PUBLIC TYPE, IN THE TWO-ARMED SPELLING EVERY CORE TEXT TYPE USES. */
#ifdef __OBJC__
@class CTTextTab;
#else
typedef struct objc_object CTTextTab;
#endif
typedef CTTextTab *CTTextTabRef;

/* "alignment type and location", with the options dictionary the getter may answer NULL for. The reference
 * count doors are this tier's, as for every Core Text class here (a `CTTextTabRef` is a `CFType` and there is
 * no CoreFoundation). */
CTTextTabRef CTTextTabCreate(CTTextAlignment alignment, CGFloat location, NSDictionary *options);
CFTypeID CTTextTabGetTypeID(void);
CTTextTabRef CTTextTabRetain(CTTextTabRef tab);
void CTTextTabRelease(CTTextTabRef tab);
CTTextAlignment CTTextTabGetAlignment(CTTextTabRef tab);
CGFloat CTTextTabGetLocation(CTTextTabRef tab);
NSDictionary *CTTextTabGetOptions(CTTextTabRef tab);

/* "Used to specify the terminating character for a tab column", whose value is a character set in the era and
 * this tier's `NSCharacterSet` here — the same substitution every other attribute key makes. */
extern NSString *const kCTTabColumnTerminatorsAttributeName;

#endif /* CORE_TEXT_CTTEXTTAB_H */
