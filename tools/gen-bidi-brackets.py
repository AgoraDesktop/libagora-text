# Copyright © 2026 Kyle J. Cardoza.
# SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
# SPDX-License-Identifier: GPL-3.0-or-later
# Portions of this file may derive from GNUstep; see THIRD-PARTY.md.

#!/usr/bin/env python3
"""Emit CTBidiBracketsData.c from the Unicode Character Database's BidiBrackets.txt.

The Bidi_Paired_Bracket and Bidi_Paired_Bracket_Type properties are DATA — a table of ~120 lines —
and the UCD file is on the machine (`/usr/share/unicode/BidiBrackets.txt` on this desktop). Python's
`unicodedata` does not expose them, which is why this reads the file rather than the module.

    python3 tools/gen-bidi-brackets.py > CTBidiBracketsData.c

Each line is `code; paired; type`, where type is `o` (opening), `c` (closing) or `n` (neither, and
those are dropped — a mirrored character that is not paired is not a bracket for BD16).
"""
import sys

PATH = "/usr/share/unicode/BidiBrackets.txt"


def entries():
    with open(PATH, encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            fields = [field.strip() for field in line.split(";")]
            if len(fields) < 3:
                continue
            code, paired, kind = fields[0], fields[1], fields[2]
            if kind not in ("o", "c"):
                continue
            yield int(code, 16), int(paired, 16), kind


def main():
    out = sys.stdout
    out.write("/*\n")
    out.write(" * CTBidiBracketsData.c — the Bidi_Paired_Bracket property, as a table of pairs.\n")
    out.write(" *\n")
    out.write(" * Copyright © 2026 Kyle J. Cardoza.\n")
    out.write(" * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza\n")
    out.write(" * SPDX-License-Identifier: LGPL-3.0-or-later\n")
    out.write(" * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.\n")
    out.write(" *\n")
    out.write(" * GENERATED, NOT WRITTEN, by `tools/gen-bidi-brackets.py` from the Unicode Character Database's\n")
    out.write(" * `BidiBrackets.txt` (the copy at /usr/share/unicode on the machine that generated it). A HAND EDIT\n")
    out.write(" * HERE IS LOST the next time the generator runs, which is the point.\n")
    out.write(" *\n")
    out.write(" * EACH ROW IS ONE BRACKET: its code point, its PAIRED code point, and whether it OPENS or CLOSES.\n")
    out.write(" * UAX #9's BD16 pushes an opener and matches a closer against it, so both directions are here.\n")
    out.write(" */\n")
    out.write("#include <CoreText/CTBidi_internal.h>\n\n")
    out.write("const struct ct_bidi_bracket ct_bidi_brackets[] = {\n")
    count = 0
    for code, paired, kind in entries():
        out.write("\t{ 0x%04X, 0x%04X, %s },\n" % (code, paired, "true" if kind == "o" else "false"))
        count += 1
    out.write("};\n\n")
    out.write("const size_t ct_bidi_bracket_count = %d;\n" % count)
    sys.stderr.write("wrote %d brackets\n" % count)


if __name__ == "__main__":
    main()
