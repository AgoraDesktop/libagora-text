#!/usr/bin/env python3
"""Emit the C case table for the UAX #14 conformance cases, one case per rule.

The Unicode Character Database ships `LineBreakTest.txt` — every rule's own examples, with Unicode's
answer for every position — and the suite should run them. It is 1.5 MB, so what is COMMITTED is this
recipe and a curated table it produces: **one case per rule number**, shortest first, so that every
rule the data exercises has a case in the suite and the table stays readable.

    python3 tools/gen-linebreak-cases.py > /tmp/cases.txt

THE FILE IS NOT IN THIS REPOSITORY, the same standing the UCD's other files have: the suite's own
table is generated from it and the recipe is what is kept. The full file was run when the engine was
written and its score is recorded in `CTLineBreak_internal.h` — 10,274 cases, no failures — and
anyone can repeat that by pointing `LB_TEST` at the downloaded file.

    curl -o /tmp/LineBreakTest.txt \
        https://www.unicode.org/Public/15.1.0/ucd/auxiliary/LineBreakTest.txt
"""
import re
import sys

PATH = "/tmp/LineBreakTest.txt"


def cases():
    """Every conformance case as (code points, expected breaks, the rules it names)."""
    with open(PATH, encoding="utf-8") as handle:
        for line in handle:
            if line.startswith("#") or not line.strip():
                continue
            body, _, comment = line.partition("#")
            cps, expected = [], []
            for token in body.split():
                if token == "\u00f7":
                    expected.append(True)
                elif token == "\u00d7":
                    expected.append(False)
                else:
                    cps.append(int(token, 16))
            if not cps or len(expected) != len(cps) + 1:
                continue
            yield cps, expected, re.findall(r"\[([0-9.]+)\]", comment)


def main():
    chosen = {}
    for cps, expected, rules in cases():
        for rule in rules:
            # The SHORTEST case naming the rule, and the first one of that length: a rule's own example
            # is worth having in the suite, and a three-character one is worth reading.
            if rule not in chosen or len(cps) < len(chosen[rule][0]):
                chosen[rule] = (cps, expected)
    write = sys.stdout.write
    for rule in sorted(chosen, key=lambda r: [int(p) for p in r.split(".")]):
        cps, expected = chosen[rule]
        points = " ".join("%04X" % c for c in cps)
        marks = "".join("1" if x else "0" for x in expected)
        write('\t{ "%s", "%s", "%s" },\n' % (points, marks, rule))


if __name__ == "__main__":
    main()
