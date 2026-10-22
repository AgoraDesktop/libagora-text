#!/usr/bin/env python3
# Copyright © 2026 Kyle J. Cardoza.
# SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
# SPDX-License-Identifier: GPL-3.0-or-later

"""Emit the C case table for the UAX #9 conformance cases, one case per feature.

The Unicode Character Database ships `BidiCharacterTest.txt` — 91,707 single-paragraph
cases, each with the resolved paragraph level, the resolved level of every character
(`x` for the ones X9 removes) and the visual order L2 produces — and the suite should run
them. It is 6.8 MB, so what is COMMITTED is this recipe and a curated table it produces:
**the shortest case that exercises each feature**, so that every construct the engine has
a code path for has a case in the suite and the table stays readable.

    python3 tools/gen-bidi-cases.py            # the curated table
    python3 tools/gen-bidi-cases.py --all      # every case, for the full run

THE FILE IS NOT IN THIS REPOSITORY, the same standing the UCD's other files have. The full
file was run when the gate was opened and its score is recorded in `CTBidi_internal.h` —
**91,691 of 91,707, 16 known-open** — and anyone can repeat that by pointing `BIDI_TEST` at
the downloaded file, which is what the suite's own case does when the variable is set.

    curl -o /tmp/BidiCharacterTest.txt \
        https://www.unicode.org/Public/15.1.0/ucd/BidiCharacterTest.txt
"""
import sys

PATH = "/tmp/BidiCharacterTest.txt"

# The features the engine has separate code paths for, and the code point that reaches each.
FEATURES = [
    ("paragraph level, auto and no strong type", 0x0030, "2"),
    ("paragraph level, auto, first strong is L", 0x0061, "2"),
    ("paragraph level, auto, first strong is R", 0x05D0, "2"),
    ("a stated right-to-left paragraph", 0x05D0, "1"),
    ("LRE", 0x202A, None),
    ("RLE", 0x202B, None),
    ("PDF", 0x202C, None),
    ("LRO", 0x202D, None),
    ("RLO", 0x202E, None),
    ("LRI", 0x2066, None),
    ("RLI", 0x2067, None),
    ("FSI", 0x2068, None),
    ("PDI", 0x2069, None),
    ("a European number", 0x0031, None),
    ("an Arabic number", 0x0661, None),
    ("a bracket pair", 0x0028, None),
    ("an NSM after a paired bracket", 0x0331, None),
    ("a trailing space (L1)", 0x0020, None),
    ("a combining mark at the end (L1)", 0x0300, None),
    ("a mirrored character, left to right", 0x0028, "0"),
    ("a mirrored character, right to left", 0x0028, "1"),
]


def cases():
    with open(PATH, encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            fields = line.strip().split(";")
            if len(fields) != 5:
                continue
            cps = [int(x, 16) for x in fields[0].split()]
            yield cps, fields


def main():
    every = "--all" in sys.argv
    chosen = {}
    for cps, fields in cases():
        if every:
            yield_line = ";".join(fields)
            sys.stdout.write('\t"%s",\n' % yield_line)
            continue
        for name, cp, direction in FEATURES:
            if cp not in cps:
                continue
            if direction is not None and fields[1] != direction:
                continue
            # The SHORTEST case for the feature, then the leftmost: a three-character case is
            # worth reading, and it is still the data's own answer.
            best = chosen.get(name)
            if best is None or len(cps) < len(best[0]):
                chosen[name] = (cps, fields)
    if every:
        return
    write = sys.stdout.write
    for name, _, _ in FEATURES:
        if name not in chosen:
            sys.stderr.write("no case for %s\n" % name)
            continue
        _, fields = chosen[name]
        write('\t/* %s */\n' % name)
        write('\t"%s",\n' % ";".join(fields))


if __name__ == "__main__":
    main()
