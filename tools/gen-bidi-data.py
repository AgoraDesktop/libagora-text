# Copyright © 2026 Kyle J. Cardoza.
# SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
# SPDX-License-Identifier: GPL-3.0-or-later
# Portions of this file may derive from GNUstep; see THIRD-PARTY.md.

#!/usr/bin/env python3
"""Emit CTBidiData.c from the Unicode Character Database that Python carries.

The Bidi_Class of a code point is DATA, not a rule: `DerivedBidiClass.txt` is a table of ~1400
ranges, and the only honest way to have it is to take it from the database and record WHERE it
came from. Python's `unicodedata` IS that database (its `unidata_version` is printed into the
header), so this script is the recipe — run it and the table is what the version says it is.

    python3 tools/gen-bidi-data.py > CTBidiData.c

Ranges of the DEFAULT class (L) are omitted, because the lookup answers L for anything the table
does not name; that is the same shape the Unicode file itself uses.
"""
import sys
import unicodedata

# THE CLASSES THIS TIER NAMES, in the order the C enum gives them. The names are Unicode's own
# (`DerivedBidiClass.txt`), and the order is the enum's, so index == the C constant.
CLASSES = ["L", "R", "AL", "EN", "ES", "ET", "AN", "CS", "NSM", "BN", "B", "S", "WS", "ON",
           "LRE", "RLE", "LRO", "RLO", "PDF", "LRI", "RLI", "FSI", "PDI"]
INDEX = {name: i for i, name in enumerate(CLASSES)}
DEFAULT = "L"
LAST = 0x10FFFF


def bidi_class(code_point):
    name = unicodedata.bidirectional(chr(code_point))
    # An UNASSIGNED code point answers the empty string; UAX #9 gives most of them L, and that is
    # the default this table already carries (see the header of the generated file).
    return name if name in INDEX else DEFAULT


def ranges():
    start = 0
    current = bidi_class(0)
    for cp in range(1, LAST + 1):
        name = bidi_class(cp)
        if name != current:
            if current != DEFAULT:
                yield start, cp - 1, current
            start = cp
            current = name
    if current != DEFAULT:
        yield start, LAST, current


def main():
    out = sys.stdout
    out.write("/*\n")
    out.write(" * CTBidiData.c — the Bidi_Class of every code point, as a table of RANGES.\n")
    out.write(" *\n")
    out.write(" * Copyright © 2026 Kyle J. Cardoza.\n")
    out.write(" * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza\n")
    out.write(" * SPDX-License-Identifier: LGPL-3.0-or-later\n")
    out.write(" * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.\n")
    out.write(" *\n")
    out.write(" * GENERATED, NOT WRITTEN. `tools/gen-bidi-data.py` reads Python's `unicodedata` — the\n")
    out.write(" * Unicode Character Database itself, version %s here — and emits this file; ranges of the\n" % unicodedata.unidata_version)
    out.write(" * DEFAULT class (L) are omitted, because the lookup answers L for anything the table does not\n")
    out.write(" * name. A HAND EDIT HERE IS LOST the next time the generator runs, which is the point.\n")
    out.write(" *\n")
    out.write(" * UNASSIGNED CODE POINTS ARE L. UAX #9 gives a handful of ranges (noncharacters, default-\n")
    out.write(" * ignorable) other defaults, and this table does not carry them: the scripts this tier lays\n")
    out.write(" * out are ASSIGNED, and a table that pretended to a rule it had not read would be worse than\n")
    out.write(" * one that says so.\n")
    out.write(" */\n")
    out.write("#include <CoreText/CTBidi_internal.h>\n\n")
    out.write("const struct ct_bidi_range ct_bidi_ranges[] = {\n")
    count = 0
    for first, last, name in ranges():
        out.write("\t{ 0x%04X, 0x%04X, kCTBidi%s },\n" % (first, last, name))
        count += 1
    out.write("};\n\n")
    out.write("const size_t ct_bidi_range_count = %d;\n" % count)
    sys.stderr.write("wrote %d ranges (Unicode %s)\n" % (count, unicodedata.unidata_version))


if __name__ == "__main__":
    main()
