# libagora-text

**Core Text for Agora** — the type-setting and layout half of Agora's Cocoa-compatible stack: fonts and their
descriptors, the attributes a run of text carries, and the line-, run- and frame-level layout that turns a
string and a width into glyphs in the right places.

Agora is an installable desktop environment for Linux and BSD; see the project README in the parent directory
(`../README.md`).

**APPLE'S CORE TEXT SITS BELOW FOUNDATION, AND SO MUST THIS ONE** (user, 2026-10-11): it depends on Core
Foundation and Core Graphics and on nothing else, and Foundation is what bridges *to* it.

**AND THE WHOLE FRAMEWORK IS PLAIN C** (same decision): Apple's Core Text has NO CLASSES AT ALL — `CTFontRef`
is a typealias and its doors are C functions, measured from Apple's own index by
`../Fiwix/tools/coretext-sweep.py` — so the eight Objective-C classes this import declares are its own
invention and are being replaced with Core Foundation types registered through
`<CoreFoundation/CFRuntime.h>`, exactly as `libagora-drawing` registers `CGColor`. The end state is eighteen
`.c` files and no `.m` at all.

THE RECONSTRUCTION IS MEASURED AGAINST APPLE'S OWN SURFACE, which the Fiwix tree keeps as a ledger:
`../Fiwix/docs/reference/coretext-apple-surface.txt` — 2032 rows, of which **1038 are already matched** (144
of the 152 functions among them) and **534 are open**: 388 properties, 93 structs, 38 typealiases, 8
functions, 5 unions, 2 vars. `../Fiwix/tools/coretext-sweep.py --check` verifies it offline and is the
progress metric. `PROVENANCE.md` says what the guide is and what the disagreement costs.

That is two dependencies, not three:

- **`libagora-core`** (Core Foundation) — the types its API is spelled with once the conversion below is made;
- **`libagora-drawing`** (Core Graphics) — `CGFontRef`, `CGPathRef`, `CGRect`, and the rest of its geometry.

**THE IMPORT SITS ON FOUNDATION AS WELL, AND THAT IS THE HALF OF THE CONVERSION STILL OWED.** It imports
`Foundation/Foundation.h` in eighteen files and spells its public API with Foundation classes where Apple's
headers name Core Foundation types — `NSString` in fourteen of the twenty-five headers, `NSDictionary` in
nine, `NSArray` in eight, `NSAttributedString` in three. That is why a C compiler cannot read
`CTFontDescriptor.h` today, which is the first thing the build says once its imports are satisfied.

## Provenance

Taken from the CoreText in the Fiwix tree (`../Fiwix/userland/CoreText`) at `6f5d90d9`, verbatim — 41 files.
See [`PROVENANCE.md`](PROVENANCE.md) for the revision, the extraction, what the sources depend on, and why the
licence took a second look.

## Status

**Imported, and the build is written: it configures and gets as far as the public headers.** The one
mechanical include it was missing — Fiwix's `libconfig`, for the font manager's preference store — is answered,
and the store is now Core Foundation's own `CFPreferences`, which is Apple's mechanism for it. What it stops on
next is the layering: **38 × `error: unknown type name 'NSString'` in `CTFontDescriptor.h`**, because a public
header is Foundation-typed where Apple's is not. That conversion is the next unit and it is not tidying —
`PROVENANCE.md` says why.

### What the import brings with it, measured rather than assumed

- **It is mostly Objective-C**: 14 `.m` implementations against 2 `.c` files, because the classes are objects.
  `libagora-drawing` was mostly C and had ten Objective-C files to convert; this one starts at fourteen.
- **It already sits on Core Graphics**: `CGBase.h` in 15 files, plus `CGPath.h`, `CGGeometry.h`, and
  `CGFont_internal.h` — so Core Text's fonts and geometry need no conversion at all, only a link.
- **It names Foundation 272 times for `NSString` alone** (80 `NSArray`, 68 `NSUInteger`, 67 `NSDictionary`,
  51 `NSRange`, 20 `NSAttributedString`), which is the size of the conversion owed. For scale: the whole
  `libagora-drawing` audit found **four lines** of live Foundation residue.
- **Its fonts come from FreeType**, through the same engine `libagora-drawing` uses, so the tree gains no new
  dependency.

### Two things the sibling repository learned that apply here

- **`CGFont_internal.h` is installed.** `libagora-drawing` installs every header it has, twelve of them
  `_internal.h`, so Core Text's `#include <CoreGraphics/CGFont_internal.h>` resolves from the prefix. A
  comment in the drawing repository claimed otherwise for a while; the install rule was always right.
- **A consumer needs clang**, because `libagora-drawing` exports `-fblocks` on its interface and GCC does not
  accept that flag. This is a clang-only tree, and a client that lets CMake choose its compiler gets a message
  from the package config rather than a confusing one from the linker.

### What is not done

- **The build does not finish, on the layering.** The public headers are Foundation-typed, so a C
  translation unit cannot include them: 38 errors, all `unknown type name 'NSString'`. Converting them is the
  next unit, and the conversion is a LAYER rather than a spelling — `PROVENANCE.md` has the shape of it.
- **No test suite.** `libagora-drawing`'s suite exists because nothing else held that library; the same will
  be true here, and its own history shows a suite finds far more defects in its *instrument* than in the
  library — nineteen cases against twelve instrument errors.
- **The NS types are still NS types**, and the conversion is the larger half of the work.
- **No consumers.** There is nothing in this workspace that uses Core Text yet — though Core Text is itself a
  consumer of Core Graphics now, and getting it to configure required exporting `libagora-base`, which had no
  CMake package either. `libagora-app`, `window-server` and `widget-zoo` are still empty repositories with
  `.gitignore` files.
