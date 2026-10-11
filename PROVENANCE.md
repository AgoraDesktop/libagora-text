# Provenance

`libagora-text` is Core Text for Agora.

**THE REPOSITORY WAS STARTED OVER ON 2026-10-11.** Its first content was the whole CoreText tree from Fiwix
(`../Fiwix/userland/CoreText`, revision `6f5d90d97763e042c3653cce376e4c1b00ef4e93`), imported verbatim as 41
files. That import is **deleted**, and it was the wrong shape twice over: it was Objective-C over Foundation,
and it declared eight classes that Apple's framework does not have. It remains in this repository's history at
`d14dc67` — nothing is lost, and anything wanted as a reference can be read there — but it is not what this
library is. Two of its fourteen implementation files had been converted to Core Foundation types before the
restart; that work is in `e303320`, `8edaa04` and `1d3c4dd`.

| | |
|---|---|
| The library | Core Text, written from scratch against Apple's 10.6 headers |
| Dependencies | **two**: Core Foundation (`libagora-core`) and Core Graphics (`libagora-drawing`) |
| Language | **plain C**. Every implementation file is a `.c`; there are no classes |
| Licence | MIT, © 2026 Kyle J. Cardoza — see `LICENSE` |

## The two rules, and both are Apple's

**CORE FOUNDATION AND CORE GRAPHICS ONLY.** Apple's Core Text sits *below* Foundation (user, 2026-10-11): its
API is Core-Foundation-typed and Foundation bridges *to* it. This library has no `find_dependency(Foundation)`
and will not have one.

**PLAIN C, NO OBJECTIVE-C**, which is not a style preference but a fact about the framework that the tree's own
tooling measures. `../Fiwix/tools/coretext-sweep.py` says it outright:

> NO CLASS-CHAIN OR BLOCK-SCOPED CREDIT … CORE TEXT HAS NO CLASSES AT ALL — `CTFontRef` is a typealias, its
> doors are C functions, and the one `protocol` in the whole framework owns one method — so there is no chain
> to walk and no block to scope a credit to.

measured from Apple's index's top-level children: **classes 0, protocols 1**. The deleted import disagreed: it
declared nine classes (eight in its `.m` files and `CTFont` in its internal header), carried 145 message sends
and 38 `@`-directives, and had sixteen `.m` files against two `.c`.

## The guide

**APPLE'S 10.6 HEADERS ARE THE GUIDE, AND THEY ARE NOT IN THIS REPOSITORY.** They were downloaded (user,
2026-10-11) from the canonical SDK mirror into `/tmp/apple-sdk106/` — 16 CoreText headers, 8 from ATS — and
the tree's own ledger states the rule that keeps them out of here: "THE SDK ITSELF IS NOT IN THIS TREE AND IS
NOT REDISTRIBUTABLE: what ships is this NAME LIST". What is committed is the recipe:

    BASE=https://raw.githubusercontent.com/phracker/MacOSX-SDKs/master/MacOSX10.6.sdk/System/Library/\\
Frameworks/ApplicationServices.framework/Versions/A/Frameworks
    API=https://api.github.com/repos/phracker/MacOSX-SDKs/contents/MacOSX10.6.sdk/System/Library/\\
Frameworks/ApplicationServices.framework/Versions/A/Frameworks
    curl -sS "$API/CoreText.framework/Versions/A/Headers" | grep -oE '"name": *"[^"]+"'
    # then, per name:  $BASE/CoreText.framework/Versions/A/Headers/<name>
    # and the ATS headers, where the SFNT vocabulary lives:  .../ATS.framework/Versions/A/Headers/<name>

**AND THE HEADERS ARE A GUIDE TO THE SURFACE, NOT A SOURCE TO COPY.** An API surface is a fact — a name, a
parameter list, a value — and this library declares the same facts in its own files with its own comments.
Apple's expression of them is Apple's.

Two more instruments live in the Fiwix tree and both were used before the restart:

| | |
|---|---|
| `../Fiwix/docs/reference/coretext-apple-surface.txt` | Apple's documented surface as **2032 rows** with a status each, generated from Apple's documentation index. Read at the restart: 1038 shipped, 534 open, 460 struck — and 515 of the open rows are the AAT/SFNT structure vocabulary |
| `../Fiwix/tools/coretext-sweep.py --check` | verifies that ledger **offline** ("the only mode that touches the network" is `--refresh`) and is therefore the progress metric for all of this work |
| `../Fiwix/docs/reference/coretext-sdk106.txt` | a presence oracle for the era: 1836 names, each with the header it was found in |
| `../Fiwix/docs/design/coretext-plan.md` | the plan this work is a milestone of |

## What Apple's headers settled that no local file could

- **The dependency set is two, not three** — and the leading one of the sixteen headers states it: no
  `#import`, no `@interface`, no `@class` anywhere in them.
- **`ATSFontRef` IS A `UInt32`**, so `CTFont.h`'s include of `ATS/ATSTypes.h` needs no ATS header: one typedef
  of an integer, and the doors that take it are owed doors anyway.
- **The include graph**, which fixes the order the headers have to be written in (see `README.md`).
- **The character→glyph step is Core Text's own**, and it is the one thing a signature cannot answer: Apple
  deprecated Core Graphics' glyph doors, this tree struck them, and reaching a font's `cmap` is the first
  architectural question of the restart.

## Agora changes

**The restart itself is this section's first entry**: the import deleted, the build rewritten for a C-only
library with two dependencies, and the package config's `find_dependency(Foundation)` removed.

This section is changed IN THE SAME COMMIT as the change it describes, and `README.md`'s status with it. The
previous repository got that wrong — its `PROVENANCE.md` said "not yet made" long after the work was finished
and its `README.md` went on claiming the NS types were unconverted — and the only defence is the rule.
