/*
 * CTFrame_internal.h — the seam between a frame and the framesetter that fills it.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * NOT PART OF CORE TEXT'S SURFACE, AND NOT INCLUDED BY ANY PUBLIC HEADER — the third such seam in this library, after
 * the run's and the line's, and for the same reason each time: APPLE HAS NO PUBLIC CREATOR FOR A FRAME. A frame
 * exists because a framesetter filled one, so the two files that must agree about how that happens agree here rather
 * than in one of them `extern`ing what the other defines.
 */
#ifndef CORE_TEXT_CTFRAME_INTERNAL_H
#define CORE_TEXT_CTFRAME_INTERNAL_H

#include <CoreText/CTFrame.h>
#include <CoreText/CTLine.h>
#include <CoreText/CTTypesetter.h>

/* MAKE A FRAME from lines somebody else already cut. The LINES ARE BORROWED AND RETAINED (the array is copied and the
 * lines with it), the PATH is RETAINED, and `origins` is COPIED — one per line, in the path's coordinates. The
 * caller keeps everything it passed.
 *
 * THE LINES AND THE ORIGINS ARRIVE TOGETHER because a frame without either is not a frame: the lines are what it
 * draws and the origins are WHERE, and a seam that could be handed one without the other would be a seam that lets a
 * frame be built wrong. */
CTFrameRef CTFrameCreateWithLines(CFArrayRef lines, const CGPoint *origins, CFIndex lineCount, CFRange stringRange,
				  CFRange visibleRange, CGPathRef path, CFDictionaryRef frameAttributes);

#endif /* CORE_TEXT_CTFRAME_INTERNAL_H */
