# Provenance

`libagora-text` is Core Text for Agora. Its source is taken from the CoreText that lives in the Fiwix tree.

| | |
|---|---|
| Source | `../Fiwix/userland/CoreText` |
| Source revision | Fiwix `6f5d90d97763e042c3653cce376e4c1b00ef4e93` |
| Extraction date | 2026-10-11 |
| Extraction | the whole `userland/CoreText` tree, verbatim — 41 files (25 `.h`, 14 `.m`, 2 `.c`) |
| Licence | MIT, © 2026 Kyle J. Cardoza (per the per-file headers; see `LICENSE`) |

THE REVISION IS NOT THE ONE `libagora-drawing` RECORDED. That import names Fiwix
`df3147a451e88d8d2256537e6d6dcdf69471695f`; this one is from `6f5d90d9` because the tree has moved since, and a
provenance that named the wrong revision would make the import unreproducible. The import was checked by
`diff -r`: the copy is identical to the source, file for file.

AND THE LICENCE TOOK A SECOND LOOK. `Fiwix/LICENSE` is FIWIX'S OWN — "Copyright (c) 2018 Jordi Sanfeliu, et
al.", with the kernel's authors listed below it — and the first pass at this import copied it, which is the
wrong licence for MIT-licensed userland code whose every file header says "Copyright © 2026 Kyle J. Cardoza.
MIT licensed — see LICENSE". `libagora-drawing/LICENSE` is the right one and this repository's is byte-identical
to it. All sixteen implementation files carry the MIT header.

## What it is

Core Text: the type-setting and layout half of the text stack. Thirteen public headers — `CTFont`,
`CTFontCollection`, `CTFontDescriptor`, `CTFontManager`, `CTFrame`, `CTFramesetter`, `CTGlyphInfo`, `CTLine`,
`CTRun`, `CTParagraphStyle`, `CTStringAttributes`, `CTTextTab`, `CTTypesetter` — plus the private ones each
of those is written against, and two C files (`CTTypeID.c`, `CTVersion.c`) that answer type identities and
the version. There is no build file in the source directory; the dependency surface below is read from what
the sources include.

## Why it is being changed

The Fiwix Core Text was written **against Foundation and on top of Core Graphics**, and Agora has both, so the
question is which of its two bindings has to move. MEASURED, from the sources themselves:

| | |
|---|---|
| `#import <Foundation/Foundation.h>` | 18 files |
| `#include <CoreGraphics/CGBase.h>` | 15 files, plus `CGPath.h`, `CGGeometry.h`, and **`CGFont_internal.h`** |
| `<ft2build.h>` | 4 files — FreeType, the same engine `libagora-drawing` uses for fonts |
| `NSString` / `NSArray` / `NSDictionary` / `NSUInteger` / `NSRange` / `NSAttributedString` | 272 / 80 / 67 / 68 / 51 / 20 uses |

SO IT SITS ON BOTH OF AGORA'S TIERS ALREADY, and neither has to be invented: `CGFontRef` is where its fonts
come from, `CGPathRef`/`CGRect` where its geometry does, and `NSAttributedString` is the one Foundation class
its public API is built around — which `libagora-base` has, and which the toll-free bridge work has already
taken through the rounded trip of being a Core Foundation object that Foundation owns.

WHAT IS OWED IS THE SAME CONVERSION `libagora-drawing` HAD, AND IN THE SAME ORDER: a build first, so the thing
compiles and links against what exists, and the CF types after that, name by name, with each one recorded
where it changed. The drawing repository's own history is the precedent — its first commit was the import and
its second gave it a build.

## Agora changes

**Not yet made — the import is verbatim**, and this section is where the changes land as they land.

A NOTE ON HOW THIS FILE STAYS TRUE, because the same sentence in `libagora-drawing/PROVENANCE.md` said "not
yet made" for a long stretch after the work was finished, and its README went on claiming the NS types had not
been converted and that lcms2 was not installed on the machine. THAT IS THE ONE MISTAKE THIS TREE HAS MADE
REPEATEDLY: a status sentence that is not re-read against the code. So: this section is changed IN THE SAME
COMMIT as the change it describes, and `README.md`'s status section with it.
