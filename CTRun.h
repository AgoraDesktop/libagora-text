/*
 * CTRun.h — a run: a sequence of glyphs that share one set of attributes.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A RUN IS WHAT SHAPING PRODUCES**, and this unit's shaper emits ONE PER LINE for now: a line whose
 * attributes change mid-string becomes several runs once the attribute-splitting half of M2 lands
 * (M2b), which is why `CTLineGetGlyphRuns` exists here at all rather than being deferred with it.
 *
 * **AND THIS HEADER SHIPS THE POINTER FORMS RATHER THAN THE RANGED ONES, FOR A REASON A READER SHOULD
 * BE ABLE TO CHECK RATHER THAN TRUST.** Apple's ranged doors (`CTRunGetGlyphs(run, range, buffer)`) take
 * a `CFRange` — a CoreFoundation type — and **this tree has no C-visible range type at all**: measured,
 * no header under `userland/CoreGraphics/` or `userland/CoreText/` names one, and there is no `CFRange`
 * here to inherit. The `*Ptr` doors are Apple's OWN alternative for reading a run without one ("returns a
 * direct pointer for the glyph array stored in the run … valid until the run is deallocated"), so this
 * tier ships the doors it can spell honestly and **NAMES THE RANGED ONES AS WAITING ON THE RANGE-TYPE
 * DECISION** — which every future C framework here will share, and which is a unit of its own rather than
 * a type invented in passing.
 *
 * **`CTRunGetTypeID` IS DECLARED NOW** (CT-M7g): this sentence said it "is absent for the same family of
 * reasons: it returns a `CFTypeID`, and this tree has no CoreFoundation to get one from". It DOES return a
 * `CFTypeID` — and `CTTypeID_internal.h` is the file that says what one is worth here.
 */
#ifndef CORE_TEXT_CTRUN_H
#define CORE_TEXT_CTRUN_H

#include <CoreGraphics/CGBase.h>
/* THE DRAWING CONTEXT, for `CTRunGetImageBounds` — the same include its neighbour
 * `CTLine.h` carries for the same reason. */
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGGeometry.h>
#include <CoreText/CTFont.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A REAL OBJECTIVE-C OBJECT, like every other Core Text object since `dec-35c114bf75c7c2d7`. */
#ifdef __OBJC__
@class CTRun;
#else
typedef struct objc_object CTRun;
#endif
typedef CTRun *CTRunRef;

/* THE RUN'S POINTERS, BORROWED — valid until the run is deallocated, and NULL when the run has no
 * glyphs. These are Apple's own signatures: no range, no destination buffer, no ownership. */
long CTRunGetGlyphCount(CTRunRef run);
const CGGlyph *CTRunGetGlyphsPtr(CTRunRef run);
const CGPoint *CTRunGetPositionsPtr(CTRunRef run);
const CGSize *CTRunGetAdvancesPtr(CTRunRef run);

/* **AND THE RANGED FORMS, WHICH EXIST NOW BECAUSE THE RANGE TYPE DOES** (`dec-5245eae215922f08`, the
 * user's decision): Apple's signatures take a range and a destination buffer, and until this unit there
 * was no C-visible range in this tree to spell one with — so the `*Ptr` doors above were what this tier
 * could ship honestly, and the ranged ones waited. `CGBase.h` now carries the range for C consumers (the
 * same `NSRange` Foundation declares, under Apple's own `struct _NSRange` tag).
 *
 * **THE DOCUMENTED EMPTY-RANGE RULE IS APPLE'S, AND IT IS IMPLEMENTED**: a `length` of 0 means THE WHOLE
 * RUN rather than nothing — which is the one thing about these doors a caller cannot guess. A range that
 * would run past the end of the run is CLAMPED rather than refused, because Apple documents the range as
 * the caller's assertion about a run the caller holds. */
void CTRunGetGlyphs(CTRunRef run, NSRange range, CGGlyph buffer[]);
void CTRunGetPositions(CTRunRef run, NSRange range, CGPoint buffer[]);
void CTRunGetAdvances(CTRunRef run, NSRange range, CGSize buffer[]);
void CTRunGetStringIndices(CTRunRef run, NSRange range, long buffer[]);

/* AND THE CLUSTER MAPPING, IN THE UNITS THIS TIER'S STRINGS ACTUALLY HAVE: **UTF-8 BYTE OFFSETS**, which
 * is what the shaper reports (`hb_glyph_info_t.cluster`) and what this tree's `NSString` is based on.
 * Apple's indices are UTF-16 units — a stated divergence rather than an unnoticed one, and the unit that
 * converts between the two is the same one that owns these doors. */
const long *CTRunGetStringIndicesPtr(CTRunRef run);

/* THE RANGE OF THE LINE'S STRING THIS RUN CAME FROM, in the same byte units, so a caller can ask "which
 * characters is this run?" — the run's first cluster and the bytes it consumed. */
NSRange CTRunGetStringRange(CTRunRef run);

/* THE ATTRIBUTES THE RUN WAS SHAPED WITH, RETAINED (+1). Apple adds "this dictionary will contain the
 * minimum number of attributes to fully specify this particular run"; THIS TIER ANSWERS THE DICTIONARY IT
 * WAS GIVEN, exactly as `CTFontDescriptorCopyAttributes` does and for the same reason (the minimum is a
 * matcher's idea of what matters, and slimming is a claim this tier has not measured). */
NSDictionary *CTRunGetAttributes(CTRunRef run) CG_RETURNS_RETAINED;


/* **THE VALUES ARE OURS**: Apple publishes enum case NAMES and no values, and the tree's Core Text
 * vocabulary follows the same rule as the rest of it — the default or "none" case is 0, a MASK's cases are
 * powers of two, and everything else is the order the era's header declares them in. What is NOT ours is the
 * set of names: every one below is a name 10.6's own headers carry. */
/* the run's status is a MASK, so its cases are bits and `NoStatus` is zero (Apple's own home). */
typedef enum {
	kCTRunStatusNoStatus = 0,
	kCTRunStatusRightToLeft = (1 << 0),
	kCTRunStatusNonMonotonic = (1 << 1),
	kCTRunStatusHasNonIdentityMatrix = (1 << 2)
} CTRunStatus;

/* **M6n: THE RUN'S STATUS, MATRIX AND METRICS DOORS** — the rest of the run's public surface, and the
 * ranged convention is the same one the accessors above follow (a length of 0 means the whole run).
 *
 * `CTRunGetStatus` is DERIVED rather than stored: this tier's runs carry no status field, and the three bits
 * the era defines are answered from what a run actually knows — `HasNonIdentityMatrix` from the font's
 * matrix, and the other two zero, because this tier's layout is left-to-right and monotonic and says so.
 *
 * `CTRunGetImageBounds` measures in the LINE's space for the same reason `CTLineGetImageBounds` does (see
 * that header): the context is accepted and not consulted.
 *
 * `CTRunGetTypeID` is declared in the block above (CT-M7g corrected the sentence that said otherwise). */
CTRunStatus CTRunGetStatus(CTRunRef run);
CFTypeID CTRunGetTypeID(void);
CGAffineTransform CTRunGetTextMatrix(CTRunRef run);
double CTRunGetTypographicBounds(CTRunRef run, NSRange range, CGFloat *ascent, CGFloat *descent,
				 CGFloat *leading);
CGRect CTRunGetImageBounds(CTRunRef run, CGContextRef context, NSRange range);

#ifdef __cplusplus
}
#endif
#endif /* CORE_TEXT_CTRUN_H */
