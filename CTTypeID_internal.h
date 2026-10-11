/*
 * CTTypeID_internal — Core Text's type identities, and a position this unit had to correct.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **NINE CORE TEXT HEADERS SAID THE SAME THING AND IT WAS NEVER MEASURED.** Each recorded that its class's
 * `…GetTypeID` door "is not declared", and each gave the same reason: "this tier has no CoreFoundation to mint
 * a `CFTypeID`", or "the carve-out `CTFont.h` already records". THE NEIGHBOUR LIBRARY CONTRADICTS ALL NINE.
 * `CoreGraphics` has been minting ids all along: `CGBase.h` declares the type (`typedef unsigned long CGTypeID;
 * typedef CGTypeID CFTypeID;` — Apple's spelling kept as an alias so that "source written against Apple's
 * headers compiles unmodified"), `CGTypeID_internal.h` owns the table, `CGTypeID.c` owns the doors, and
 * **twelve `CG…GetTypeID` doors already ship**, `CGImageGetTypeID` and `CGFontGetTypeID` among them. A Core
 * Text font has an identity for exactly the reason a Core Graphics font does, and the argument for refusing one
 * was never about this tree — it was about Apple's runtime, which assigns ITS numbers and publishes none.
 *
 * **SO THE TYPE IS `CFTypeID` (WHICH IS `CGTypeID`), THE VALUE IS A CONSTANT, AND THE CONTRACT IS IDENTITY**:
 * the same class always answers the same number and two classes never share one. That is the whole of what
 * Apple's documentation promises, and it is what a caller uses these doors for — `CFGetTypeID(obj) ==
 * CTFontGetTypeID()` — never a portable constant, because Apple publishes none either.
 *
 * **AND THE NUMBERING IS ONE SEQUENCE SPLIT BY LAYERING.** CoreGraphics owns 1..63 (its own header says "ONE
 * REGISTRY, ONE FILE, FIXED NUMBERS") and **Core Text starts at 64**, so the two tables cannot collide without
 * either library reaching into the other's file. The alternative — adding Core Text's ids to
 * `CGTypeID_internal.h` — would be more literally "one registry" and would make CoreGraphics depend on a
 * framework built ON it; reserving a range keeps the dependency pointing the one way it goes and makes the
 * distinctness a property of the BASE rather than of review. When a real CoreFoundation arrives, the agreement
 * to make is that `CFGetTypeID` answers these numbers for these classes — the same agreement `CGBase.h` names.
 *
 * ONE IS NOT ZERO, AND ZERO IS NOT USED: the numbering starts at 1 in CoreGraphics' half and this half adds
 * sixty-four, so a zeroed field or an uninitialised variable can never be mistaken for an identity. */
#ifndef CORE_TEXT_CTTYPEID_INTERNAL_H
#define CORE_TEXT_CTTYPEID_INTERNAL_H

#include <CoreGraphics/CGBase.h>

/* THE TABLE. Adding a class means adding a line HERE and a door in `CTTypeID.c`, which is the whole point of
 * one file owning the numbers: no id is ever picked at a call site. */
#define CT_TYPE_ID_BASE			64u
#define CT_TYPE_ID_CTFONT		(CT_TYPE_ID_BASE + 0u)
#define CT_TYPE_ID_CTFONTDESCRIPTOR	(CT_TYPE_ID_BASE + 1u)
#define CT_TYPE_ID_CTFONTCOLLECTION	(CT_TYPE_ID_BASE + 2u)
#define CT_TYPE_ID_CTFRAME		(CT_TYPE_ID_BASE + 3u)
#define CT_TYPE_ID_CTFRAMESETTER	(CT_TYPE_ID_BASE + 4u)
#define CT_TYPE_ID_CTGLYPHINFO		(CT_TYPE_ID_BASE + 5u)
#define CT_TYPE_ID_CTLINE		(CT_TYPE_ID_BASE + 6u)
#define CT_TYPE_ID_CTPARAGRAPHSTYLE	(CT_TYPE_ID_BASE + 7u)
#define CT_TYPE_ID_CTRUN		(CT_TYPE_ID_BASE + 8u)
#define CT_TYPE_ID_CTTEXTTAB		(CT_TYPE_ID_BASE + 9u)
#define CT_TYPE_ID_CTTYPESETTER		(CT_TYPE_ID_BASE + 10u)

#endif /* CORE_TEXT_CTTYPEID_INTERNAL_H */
