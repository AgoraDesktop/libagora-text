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

**THE IMPORT IS STILL VERBATIM — no source file has been edited — and this section records what has been added
around it and what has been DECIDED about it.** It is being written in the same commit as the change, which is
what the note below asks for.

### 1. A build (`CMakeLists.txt`), and the one thing it stops on

The house shape, copied from `libagora-drawing`: the include shim so these headers are `<CoreText/...>`, every
`.m` and `.c` compiled as one target, the tree's Objective-C runtime and `-fconstant-cfstrings`, and an EXPORT
BLOCK FROM THE FIRST COMMIT, which is where the drawing library's should have been. It configures, and then:

    CTFontManager.m:32:10: fatal error: 'libconfig.h' file not found

**ONE, OUT OF SIXTEEN IMPLEMENTATION FILES AND TWENTY-FIVE HEADERS.** That is the measurement this build exists
to make: the imported source is otherwise ready to compile against what this tree already has.

### 2. `libconfig` is not coming, and the preference format is APPLE'S PLIST (user, `dec-750b32baf39ba5cb`)

The font manager keeps the set of ENABLED FONTS through Fiwix's `libconfig` — "universal configuration access
for the FNX OS userland", five files, about 135 KB, with a `Configuration/` directory tree and first-party
domains as its model. `CTFontManager.h` does not expose it: its doors are Apple's own
(`CTFontManagerCopyAvailableFontFamilyNames`, `…RegisterFontsForURL`, `…GetScopeForURL`,
`…CompareFontFamilyNames`), and `libconfig` appears there in comments only. The use is two calls in
`CTFontManager.m` — `config_set_int` and `config_get_int` — behind `CTFontManagerSetAutoActivationSetting` and
`CTFontManagerGetAutoActivationSetting`. A PLIST-BACKED STORE REPLACES THEM, which is what Apple's Core Text
does with a per-application preference.

### 3. AND WHAT THAT WOULD BE BUILT ON IS NOT READY EITHER — MEASURED, AND IT IS A FINDING ABOUT `libagora-base`

Foundation HAS `NSUserDefaults`, and it cannot work on this machine:

    *** Terminating due to uncaught exception 'NSInternalInconsistencyException', reason:
        'NSUserDefaults: cannot create /Users/kyle/Configuration: Read-only file system'

IT COMPUTED ITS STORE AS `/Users/kyle/Configuration` — the macOS home shape — AND IT IGNORED
`XDG_CONFIG_HOME`, WHICH WAS SET. Core Foundation, by contrast, implements the XDG config directories in
`CFFileUtilities.c` (`$XDG_CONFIG_HOME`, `$XDG_CONFIG_DIRS`), so **the two halves of this tree disagree about
where a preference lives**, and the Foundation half is the one that cannot write.

THAT IS NOT THIS LIBRARY'S BUG AND IT IS NOT THIS COMMIT'S TO FIX. It is recorded here because this is where it
surfaced, and it gives the replacement above two routes: a small plist store local to `CTFontManager.m`, or
Foundation's `NSUserDefaults` fixed to use the directories Core Foundation already resolves — the second being
the one that would make this library's code look like Apple's.

A NOTE ON HOW THIS FILE STAYS TRUE, because the same sentence in `libagora-drawing/PROVENANCE.md` said "not
yet made" for a long stretch after the work was finished, and its README went on claiming the NS types had not
been converted and that lcms2 was not installed on the machine. THAT IS THE ONE MISTAKE THIS TREE HAS MADE
REPEATEDLY: a status sentence that is not re-read against the code. So: this section is changed IN THE SAME
COMMIT as the change it describes, and `README.md`'s status section with it.
