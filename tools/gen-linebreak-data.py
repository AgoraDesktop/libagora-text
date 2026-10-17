#!/usr/bin/env python3
"""Emit CTLineBreakData.c from the Unicode Character Database's LineBreak.txt and EastAsianWidth.txt.

The Line_Break property is DATA, not a rule: LineBreak.txt is a table of ~3500 ranges, and the only
honest way to have it is to take it from the database and record WHERE it came from. Python's
`unicodedata` does not expose it, which is why this reads the files rather than the module.

    python3 tools/gen-linebreak-data.py > CTLineBreakData.c

**THREE DEFAULTS ARE IMPLEMENTED HERE, AND THEY ARE THE FILE'S OWN — NOTHING INVENTED.** The file's
`@missing` line says every code point it does not list is `XX`, and its header then names the blocks
where an UNLISTED code point is something else instead: `ID` for the CJK blocks and for undesignated
points in Planes 2 and 3 and two Plane 1 ranges, and `PR` for the Currency Symbols block. A table
that skipped that would answer `XX` for tens of thousands of unassigned ideographs that UAX #14
calls `ID`, which is the difference between breaking between two Han characters and not.

AND THE SECOND TABLE IS FOR ONE RULE. LB30 breaks after `CP` / before `OP` only when the bracket is
NOT East Asian, and "East Asian" is `East_Asian_Width` being F, W or H — another property in another
file. So the ranges of `OP`/`CP` whose width is wide are emitted beside the classes, and the rule
reads them.
"""
import sys

LINE_BREAK = "/usr/share/unicode/LineBreak.txt"
EAST_ASIAN = "/usr/share/unicode/EastAsianWidth.txt"
GENERAL_CATEGORY = "/usr/share/unicode/extracted/DerivedGeneralCategory.txt"
EMOJI_DATA = "/usr/share/unicode/emoji/emoji-data.txt"
LAST = 0x10FFFF

# The classes in the order the C enum gives them, so an index in this file IS the C constant. The
# names are Unicode's own; the order is this file's and the header's must agree with it.
CLASSES = [
    "XX", "AI", "AK", "AL", "AP", "AS", "B2", "BA", "BB", "BK", "CB", "CJ", "CL", "CM", "CP",
    "CR", "EB", "EM", "EX", "GL", "H2", "H3", "HL", "HY", "ID", "IN", "IS", "JL", "JT", "JV",
    "LF", "NL", "NS", "NU", "OP", "PO", "PR", "QU", "RI", "SA", "SG", "SP", "SY", "VF", "VI",
    "WJ", "ZW", "ZWJ",
]
INDEX = {name: i for i, name in enumerate(CLASSES)}
DEFAULT = "XX"

# The file's header, transcribed: (first, last, class) for UNLISTED code points only.
UNLISTED_DEFAULTS = [
    (0x3400, 0x4DBF, "ID"),
    (0x4E00, 0x9FFF, "ID"),
    (0xF900, 0xFAFF, "ID"),
    (0x20000, 0x2FFFD, "ID"),
    (0x30000, 0x3FFFD, "ID"),
    (0x1F000, 0x1FAFF, "ID"),
    (0x1FC00, 0x1FFFD, "ID"),
    (0x20A0, 0x20CF, "PR"),
]


def parse(path, field):
    """Every `range ; value` line in a UCD file, as (first, last, value)."""
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            codes, value = line.split(";")[0].strip(), line.split(";")[1].strip()
            if value not in field:
                continue
            if ".." in codes:
                first, last = codes.split("..")
                yield int(first, 16), int(last, 16), value
            else:
                yield int(codes, 16), int(codes, 16), value


def classes():
    table = bytearray([INDEX[DEFAULT]]) * (LAST + 1)
    listed = bytearray(LAST + 1)
    for first, last, name in parse(LINE_BREAK, INDEX):
        for cp in range(first, last + 1):
            table[cp] = INDEX[name]
            listed[cp] = 1
    # AND THE HEADER'S DEFAULTS, FOR WHAT THE FILE DID NOT LIST.
    for first, last, name in UNLISTED_DEFAULTS:
        for cp in range(first, last + 1):
            if not listed[cp]:
                table[cp] = INDEX[name]
    return table


def ranges(table):
    start = 0
    current = table[0]
    for cp in range(1, LAST + 1):
        if table[cp] != current:
            if current != INDEX[DEFAULT]:
                yield start, cp - 1, current
            start = cp
            current = table[cp]
    if current != INDEX[DEFAULT]:
        yield start, LAST, current


def general_category():
    """The General_Category of every code point, as a list of (first, last, value)."""
    out = []
    with open(GENERAL_CATEGORY, encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            codes, value = [x.strip() for x in line.split(";")[:2]]
            if ".." in codes:
                first, last = codes.split("..")
                out.append((int(first, 16), int(last, 16), value))
            else:
                out.append((int(codes, 16), int(codes, 16), value))
    return out


def category_of(category, target):
    """The code points whose General_Category is `target`, as a byte table."""
    table = bytearray(LAST + 1)
    for first, last, value in category:
        if value != target:
            continue
        for cp in range(first, last + 1):
            table[cp] = 1
    return table


def compressed(settable):
    """The runs of a byte table, as (first, last) pairs."""
    start = None
    previous = None
    for cp in list(range(LAST + 1)) + [None]:
        if cp is not None and settable[cp]:
            if start is None:
                start = cp
            previous = cp
            continue
        if start is not None:
            yield start, previous
        start = None
        previous = None


def east_asian_widths():
    wide = bytearray(LAST + 1)
    for first, last, value in parse(EAST_ASIAN, ("F", "W", "H")):
        for cp in range(first, last + 1):
            wide[cp] = 1
    return wide


def main():
    table = classes()
    wide = east_asian_widths()
    write = sys.stdout.write

    write("""/*
 * CTLineBreakData.c — the Line_Break property of every code point, and the wide brackets LB30 asks about.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * GENERATED, NOT WRITTEN, by `tools/gen-linebreak-data.py` from the Unicode Character Database
 * (`LineBreak.txt` and `EastAsianWidth.txt`, Unicode 15.1.0, the copies at /usr/share/unicode on the
 * machine that generated them). A HAND EDIT HERE IS LOST the next time the generator runs, which is
 * the point.
 *
 * RANGES OF THE DEFAULT CLASS ARE OMITTED, because the lookup answers `XX` for anything the table does
 * not name — the same shape the Unicode file itself uses. **AND THE FILE'S OWN DEFAULTS ARE BAKED IN**:
 * an UNLISTED code point in a CJK block, in Planes 2 and 3, or in two Plane 1 ranges is `ID`, and one
 * in the Currency Symbols block is `PR`. Those are the header's rules and not this table's, which is
 * why they were applied when it was generated rather than left to a lookup that cannot know whether a
 * code point was LISTED.
 *
 * AND THE SECOND TABLE EXISTS FOR ONE RULE: LB30 treats a `CP`/`OP` as a break opportunity only when it
 * is NOT East Asian, which is `East_Asian_Width` being F, W or H — so the wide brackets are here.
 */
#include <CoreText/CTLineBreak_internal.h>

/* THE CLASS OF EVERY CODE POINT THAT IS NOT `XX`, as half-open ranges. */
const struct ct_lb_range ct_lb_ranges[] = {
""")
    for first, last, name in ranges(table):
        write("\t{ 0x%04X, 0x%04X, kCTLB%s },\n" % (first, last, CLASSES[name]))
    write("""};

const CFIndex ct_lb_range_count = (CFIndex)(sizeof ct_lb_ranges / sizeof ct_lb_ranges[0]);

/* AND THE `CP`/`OP` CODE POINTS WHOSE EAST ASIAN WIDTH IS F, W OR H — the ones LB30 refuses to treat as
 * break opportunities. */
const struct ct_lb_range ct_lb_wide_brackets[] = {
""")
    brackets = bytearray(LAST + 1)
    for cp in range(LAST + 1):
        if CLASSES[table[cp]] in ("OP", "CP") and wide[cp]:
            brackets[cp] = 1
    for first, last in compressed(brackets):
        write("\t{ 0x%04X, 0x%04X, 0 },\n" % (first, last))
    write("""};

const CFIndex ct_lb_wide_bracket_count =
\t(CFIndex)(sizeof ct_lb_wide_brackets / sizeof ct_lb_wide_brackets[0]);

/* **AND THE THREE TABLES THE 15.1 RULES NEED, EACH FOR ONE RULE.** LB15a and LB15b turn on whether a `QU` is
 * INITIAL or FINAL punctuation — `General_Category` being Pi or Pf — which is a different property in a different
 * UCD file; LB30b's second clause is about UNASSIGNED extended pictographs, which is the emoji property minus the
 * assigned code points. All three are ranges of code points, like the class table above. */
const struct ct_lb_range ct_lb_pi_qu[] = {
""")
    category = general_category()
    pi = category_of(category, "Pi")
    pf = category_of(category, "Pf")
    qu = bytearray(LAST + 1)
    for cp in range(LAST + 1):
        if CLASSES[table[cp]] == "QU":
            qu[cp] = 1
    for name, settable in (("pi", pi), ("pf", pf)):
        for cp in range(LAST + 1):
            if qu[cp] and settable[cp]:
                pass
    # emit the Pi table, then close and open the Pf table
    pi_qu = bytearray(LAST + 1)
    pf_qu = bytearray(LAST + 1)
    for cp in range(LAST + 1):
        if qu[cp]:
            if pi[cp]:
                pi_qu[cp] = 1
            if pf[cp]:
                pf_qu[cp] = 1
    for first, last in compressed(pi_qu):
        write("\t{ 0x%04X, 0x%04X, 0 },\n" % (first, last))
    write("""};

const CFIndex ct_lb_pi_qu_count = (CFIndex)(sizeof ct_lb_pi_qu / sizeof ct_lb_pi_qu[0]);

const struct ct_lb_range ct_lb_pf_qu[] = {
""")
    for first, last in compressed(pf_qu):
        write("\t{ 0x%04X, 0x%04X, 0 },\n" % (first, last))
    write("""};

const CFIndex ct_lb_pf_qu_count = (CFIndex)(sizeof ct_lb_pf_qu / sizeof ct_lb_pf_qu[0]);

/* LB30b's second clause: an UNASSIGNED code point that is still extended pictographic. */
const struct ct_lb_range ct_lb_unassigned_pictographic[] = {
""")
    pictographic = bytearray(LAST + 1)
    with open(EMOJI_DATA, encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            codes, value = [x.strip() for x in line.split(";")[:2]]
            if value != "Extended_Pictographic":
                continue
            if ".." in codes:
                first, last = codes.split("..")
                first, last = int(first, 16), int(last, 16)
            else:
                first = last = int(codes, 16)
            for cp in range(first, last + 1):
                pictographic[cp] = 1
    cn = category_of(category, "Cn")
    unassigned = bytearray(LAST + 1)
    for cp in range(LAST + 1):
        if pictographic[cp] and cn[cp]:
            unassigned[cp] = 1
    for first, last in compressed(unassigned):
        write("\t{ 0x%04X, 0x%04X, 0 },\n" % (first, last))
    write("""};

const CFIndex ct_lb_unassigned_pictographic_count =
\t(CFIndex)(sizeof ct_lb_unassigned_pictographic / sizeof ct_lb_unassigned_pictographic[0]);
""")


if __name__ == "__main__":
    main()
