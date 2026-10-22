#!/usr/bin/env python3
# Copyright © 2026 Kyle J. Cardoza.
# SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
# SPDX-License-Identifier: GPL-3.0-or-later
# Portions of this file may derive from GNUstep; see THIRD-PARTY.md.

"""Emit CTBidiMirroringData.c from the Unicode Character Database's BidiMirroring.txt.

The MIRRORED GLYPH of a character is DATA — a table of ~360 lines — and the UCD file is on the machine
(`/usr/share/unicode/BidiMirroring.txt` on this desktop). UAX #9's L4 says a character is depicted by its
mirrored glyph when its RESOLVED directionality is R and the Bidi_Mirrored property is Yes; the level says
the first, and this table says what the mirror IS.

    python3 tools/gen-bidi-mirroring.py > CTBidiMirroringData.c

Each line is `code; mirrored`, and the mirrored value is what the character becomes.
"""
import sys

PATH = "/usr/share/unicode/BidiMirroring.txt"


def entries():
    with open(PATH, encoding="utf-8") as handle:
        for line in handle:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            fields = [field.strip() for field in line.split(";")]
            if len(fields) < 2:
                continue
            yield int(fields[0], 16), int(fields[1], 16)


def main():
    out = sys.stdout
    out.write("/*\n")
    out.write(" * CTBidiMirroringData.c — the MIRRORED GLYPH of a character, as a table.\n")
    out.write(" *\n")
    out.write(" * Copyright © 2026 Kyle J. Cardoza.\n")
    out.write(" * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza\n")
    out.write(" * SPDX-License-Identifier: LGPL-3.0-or-later\n")
    out.write(" * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.\n")
    out.write(" *\n")
    out.write(" * GENERATED, NOT WRITTEN, by `tools/gen-bidi-mirroring.py` from the Unicode Character Database's\n")
    out.write(" * `BidiMirroring.txt` (the copy at /usr/share/unicode on the machine that generated it). A HAND EDIT\n")
    out.write(" * HERE IS LOST the next time the generator runs, which is the point.\n")
    out.write(" */\n")
    out.write("#include <CoreText/CTBidi_internal.h>\n\n")
    out.write("const struct ct_bidi_mirroring ct_bidi_mirrorings[] = {\n")
    count = 0
    for code, mirrored in entries():
        out.write("\t{ 0x%04X, 0x%04X },\n" % (code, mirrored))
        count += 1
    out.write("};\n\n")
    out.write("const size_t ct_bidi_mirroring_count = %d;\n" % count)
    sys.stderr.write("wrote %d mirrored characters\n" % count)


if __name__ == "__main__":
    main()
