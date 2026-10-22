/*
 * CTFramesetter.h — the framesetter: a typesetter with a shape to fill.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * WRITTEN FROM APPLE'S 10.6 HEADER (PROVENANCE.md), whose include list says what this type IS: `CTFrame.h` and
 * `CTTypesetter.h`, and nothing else of the framework's. A framesetter is A TYPESETTER THAT KNOWS WHERE THE TEXT HAS
 * TO GO — which is why all five of its doors are about a frame or about the typesetter underneath one.
 *
 * AND IT IS THE LAST TYPE IN THIS LAYER THAT IS NOT A SETTING OR A CONTAINER: a caller with a string and a rectangle
 * needs exactly this and the frame it makes. Everything above it — the collection, the manager, the paragraph style —
 * is a vocabulary; everything below it — the typesetter, the line, the run — is a mechanism; this is the door a text
 * view would call.
 */
#ifndef CORE_TEXT_CTFRAMESETTER_H
#define CORE_TEXT_CTFRAMESETTER_H

#include <CoreText/CTFrame.h>
#include <CoreText/CTTypesetter.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE FRAMESETTER. Opaque and a Core Foundation object, so `CFGetTypeID` of one IS `CTFramesetterGetTypeID()`. */
typedef const struct __CTFramesetter *CTFramesetterRef;

CFTypeID CTFramesetterGetTypeID(void);

/* ONE CREATOR AT 10.6, AND NO OPTIONS DOOR — Apple adds `…AndOptions` later in the era, and that is a fact about
 * this header rather than a decision here: a typesetter's options exist at 10.6 and a framesetter's do not, so this
 * type carries none. */
CTFramesetterRef CTFramesetterCreateWithAttributedString(CFAttributedStringRef string);

/* FILL A FRAME, and Apple's own sentence is the whole contract: the framesetter "will continue to fill the frame
 * until it either runs out of text or it finds that text no longer fits". So the frame may cover LESS than the range
 * it was asked for, and `CTFrameGetVisibleStringRange` is how a caller finds out how much — a zero-length range means
 * the rest of the string, as it does for every range-taking door in this framework. */
CTFrameRef CTFramesetterCreateFrame(CTFramesetterRef framesetter, CFRange stringRange, CGPathRef path,
				    CFDictionaryRef frameAttributes);

/* THE TYPESETTER UNDER IT, BORROWED — the type a framesetter is built from, which is why this door exists at all: a
 * caller that wants to break lines ITSELF asks the framesetter for the engine rather than rebuilding one. */
CTTypesetterRef CTFramesetterGetTypesetter(CTFramesetterRef framesetter);

/* HOW MUCH ROOM A RANGE NEEDS, optionally constrained: Apple's own account is that this "may be used to determine how
 * much space is needed to display a string, optionally by constraining the space along either dimension", and
 * `fitRange` (which may be NULL) receives the part of the range that fits the constraints — the same prefix the
 * visible-range door reports for a frame, measured without making one. */
CGSize CTFramesetterSuggestFrameSizeWithConstraints(CTFramesetterRef framesetter, CFRange stringRange,
						    CFDictionaryRef frameAttributes, CGSize constraints,
						    CFRange *fitRange);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFRAMESETTER_H */
