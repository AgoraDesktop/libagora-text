/*
 * CTGlyphInfo_internal.h — the class behind `CTGlyphInfoRef`.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A ROOT-LIKE CLASS IN THE TREE'S OWN SENSE** (`dec-35c114bf75c7c2d7`): Apple's `CTGlyphInfoRef` is a
 * `CFType`, and this tier's Core Text objects are Objective-C classes, so this one is an `NSObject` subclass
 * with the same doors. What it HOLDS is exactly what the era's three create functions take, and nothing else:
 *
 *   * the GLYPH — by name, by index, or by character identifier (`cid`);
 *   * the FONT the glyph is to be read from;
 *   * and the BASE STRING: "the part of the string the returned object is intended to override", which the
 *     attribute key's own sentence makes the FIRST of its two conditions.
 */
#import <Foundation/Foundation.h>
#include <CoreText/CTGlyphInfo.h>

/* **THE CLASS IS DECLARED WITH ITS IVARS IN ITS OWN IMPLEMENTATION FILE**, and this header carries only the
 * forward declaration and the accessors the drawing side calls. That is not a style choice: the ivar-offset
 * symbols clang emits are SCALED and per-class, and they must be EMITTED IN THE FILE THAT OWNS THE LAYOUT —
 * a unit that merely *reads* another class's ivars ends up asking for a symbol nothing defines, which is
 * exactly the host-link failure this header was split to end. `CTLine.m` needs the questions, not the
 * storage. */
@class CTGlyphInfo;

/* THE INTERNAL ACCESSORS the drawing side uses, in the same shape as `ctf_*` and `ctd_*`. */
NSString *ctg_base_string(CTGlyphInfo *info);
/* "MAY THIS OBJECT STAND IN FOR THIS RANGE?" — the era's second and third conditions. The run's font is a
 * PARAMETER because the key's own sentence names it ("the font specified by kCTFontAttributeName"), and it is
 * not always the font the info was created with. */
int ctg_is_resolvable(CTGlyphInfo *info, CTFontRef run_font, CGGlyph *out_glyph);
