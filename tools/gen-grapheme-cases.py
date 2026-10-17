#!/usr/bin/env python3
"""Emit the C case table for the UAX #29 conformance cases, one case per rule.

`GraphemeBreakTest.txt` states, for 1,187 strings, where a grapheme cluster boundary falls — `÷` where there is one and
`×` where there is not — and its comments name the rule that decided each position. The suite should run them; what is
COMMITTED is this recipe and a curated table it produces: **one case per rule number**, shortest first, so that every
rule the data exercises has a case in the suite and the table stays readable.

    python3 tools/gen-grapheme-cases.py > /tmp/rows.c

THE FILE IS NOT IN THIS REPOSITORY, the same standing every other UCD file has: the suite's own table is generated
from it and the recipe is what is kept. The full file is run when `GRABREAK_TEST` names it —

    curl -o /tmp/GraphemeBreakTest.txt \\
        https://www.unicode.org/Public/15.1.0/ucd/auxiliary/GraphemeBreakTest.txt
"""
import re
import sys

PATH = "/tmp/GraphemeBreakTest.txt"


def cases():
    with open(PATH, encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            body, _, comment = line.partition("#")
            points, marks = [], []
            for token in body.split():
                if token == "\u00f7":
                    marks.append(True)
                elif token == "\u00d7":
                    marks.append(False)
                else:
                    points.append(int(token, 16))
            if not points or len(marks) != len(points) + 1:
                continue
            yield points, marks, re.findall(r"\[([0-9.]+)\]", comment)


def main():
    chosen = {}
    for points, marks, rules in cases():
        for rule in rules:
            if rule not in chosen or len(points) < len(chosen[rule][0]):
                chosen[rule] = (points, marks)
    write = sys.stdout.write
    for rule in sorted(chosen, key=lambda r: [int(p) for p in r.split(".")]):
        points, marks = chosen[rule]
        write('\t{ "%s", "%s", "%s" },\n'
              % (" ".join("%04X" % c for c in points), "".join("1" if m else "0" for m in marks), rule))


if __name__ == "__main__":
    main()
