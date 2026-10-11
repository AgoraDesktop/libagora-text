# libagora-text

**Core Text for Agora** — the type-setting and layout half of Agora's Cocoa-compatible stack: fonts and their
descriptors, the attributes a run of text carries, and the line-, run- and frame-level layout that turns a
string and a width into glyphs in the right places.

Agora is an installable desktop environment for Linux and BSD; see the project README in the parent directory
(`../README.md`). `libagora-text` sits on three of its siblings already:

- **`libagora-core`** (Core Foundation) — the types its API is spelled with once the conversion below is made;
- **`libagora-drawing`** (Core Graphics) — `CGFontRef`, `CGPathRef`, `CGRect`, and the rest of its geometry;
- **`libagora-base`** (Foundation) — `NSAttributedString`, which its public API is built around, and the
  `NSDictionary` of attributes every one of its calls takes.

## Provenance

Taken from the CoreText in the Fiwix tree (`../Fiwix/userland/CoreText`) at `6f5d90d9`, verbatim — 41 files.
See [`PROVENANCE.md`](PROVENANCE.md) for the revision, the extraction, what the sources depend on, and why the
licence took a second look.

## Status

**Imported, and the build is written: it configures and stops on exactly one include.** The NS types have
not been converted back to CF yet, which is the larger half and comes after the build — `PROVENANCE.md` says
why that order, and records the decisions taken in the same commits as the changes.

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

- **The build does not finish, on one include.** `CTFontManager.m` wants Fiwix's `libconfig` for the set of
  ENABLED FONTS — a 135 KB FNX-OS configuration library that is not coming, because the preference format is
  Apple's plist (`dec-750b32baf39ba5cb`). A plist-backed store replaces its two calls, and the thing that store
  would naturally sit on — Foundation's `NSUserDefaults` — cannot work on this machine: it computes
  `/Users/<user>/Configuration`, ignores `XDG_CONFIG_HOME`, and throws. Measured, and recorded in
  `PROVENANCE.md` because it is a finding about `libagora-base` rather than about this library.
- **No test suite.** `libagora-drawing`'s suite exists because nothing else held that library; the same will
  be true here, and its own history shows a suite finds far more defects in its *instrument* than in the
  library — nineteen cases against twelve instrument errors.
- **The NS types are still NS types**, and the conversion is the larger half of the work.
- **No consumers.** There is nothing in this workspace that uses Core Text yet — though Core Text is itself a
  consumer of Core Graphics now, and getting it to configure required exporting `libagora-base`, which had no
  CMake package either. `libagora-app`, `window-server` and `widget-zoo` are still empty repositories with
  `.gitignore` files.
