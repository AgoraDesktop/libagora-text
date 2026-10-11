# libagora-text

**Core Text for Agora** — the type-setting and layout half of Agora's Cocoa-compatible stack: fonts and
their descriptors, the attributes a run of text carries, and the line-, run- and frame-level layout that
turns a string and a width into glyphs in the right places.

Agora is an installable desktop environment for Linux and BSD; see the project README in the parent directory
(`../README.md`).

## STARTED OVER, FROM APPLE'S HEADERS (2026-10-11)

The Fiwix CoreText import that used to be here is **deleted**. It was Objective-C over Foundation, and it is
still in this repository's history (`d14dc67`) if anything in it is ever wanted as a reference — but it is not
the shape this library is supposed to have, and incrementally converting it was the wrong way round. This is
now Core Text written from scratch against **Apple's own 10.6 headers**.

## The two rules, both of them Apple's own as much as the user's

- **CORE FOUNDATION AND CORE GRAPHICS ONLY.** Apple's Core Text sits *below* Foundation: it depends on those
  two and nothing else, its API is Core-Foundation-typed (`CTFontCreateWithName(CFStringRef, …)`,
  `CTFramesetterCreateWithAttributedString(CFAttributedStringRef)`, `CTRunGetAttributes` → `CFDictionaryRef`),
  and Foundation is what bridges *to* it. There is no `find_dependency(Foundation)` here and there will not
  be one.
- **PLAIN C, NO OBJECTIVE-C.** Apple's Core Text has NO CLASSES AT ALL — `CTFontRef` is a typealias and its
  doors are C functions — which `../Fiwix/tools/coretext-sweep.py` states in its own words, measured from
  Apple's index (top-level children: classes 0, protocols 1). The target is `LANGUAGES C` and every
  implementation file is a `.c`. The import had fourteen `.m` files; this will have none.

## The guide, and where it lives

| | |
|---|---|
| Apple's 10.6 headers | **downloaded to `/tmp/apple-sdk106/`** — 16 CoreText headers and 8 from ATS. The fetch recipe is in `PROVENANCE.md`; **the headers themselves are not in this repository and must not be**, because the SDK is not redistributable |
| Apple's documented surface | `../Fiwix/docs/reference/coretext-apple-surface.txt` — 2032 rows with a status each. `../Fiwix/tools/coretext-sweep.py --check` verifies it offline and is the progress metric |
| the era's ground | `../Fiwix/docs/reference/coretext-sdk106.txt` — a presence oracle: 1836 names and the header each was found in |
| the plan | `../Fiwix/docs/design/coretext-plan.md` |

At the moment of the restart the ledger read **1038 shipped, 534 open, 460 struck** — and 515 of the open
rows are the AAT/SFNT structure vocabulary, which is a data port of two headers rather than behaviour.

## The order the headers go in, which is Apple's own include graph

`CTFontTraits` → `CTFontDescriptor` → `CTFont` → (`CTFontCollection`, `CTFontManager`) → `CTParagraphStyle` →
`CTLine` → (`CTRun`, `CTTypesetter`) → `CTFrame` → `CTFramesetter` → (`CTGlyphInfo`, `CTTextTab`,
`CTStringAttributes`) → `CoreText.h`, the umbrella. Read from what Apple's headers include, not guessed.

## Status

- **The leaf is done and the library BUILDS.** `CTFontTraits.h` and `CTFontTraits.c` — the four trait keys,
  `kCTFontClassMaskShift`, the symbolic traits and the eleven stylistic classes — written from Apple's header,
  **with Apple's published values rather than a guess at their order.** MEASURED: `cmake --build build`
  exits 0 with no errors from a clean directory, and `nm -D build/libagora-text.so` lists
  `kCTFontSymbolicTrait`, `kCTFontWeightTrait`, `kCTFontWidthTrait` and `kCTFontSlantTrait` as defined.
- **No test suite yet**, and the values above are the first thing worth holding: the trait bits, the class
  numbers and the four strings are all facts a case can assert, including that the strings are the AppKit's.
- **`CTFontDescriptor` is declared, and its keys are defined.** The header carries Apple's whole surface for
  it — the opaque `CTFontDescriptorRef`, `GetTypeID`, the twenty-two attribute keys **with the value type each
  carries**, `CTFontOrientation`, `CTFontFormat`, `CTFontPriority`, and the ten doors — and `CTFontDescriptor.c`
  defines the twenty-two key strings. MEASURED: the build is clean and the suite is `3 of 3`, including a case
  that names all twenty-two strings and checks they are distinct.
- **THE TWENTY-TWO KEY STRINGS ARE A RECORDED GUESS** (`dec-4795696b09d362f6`). Apple publishes NO VALUE for
  any of them — not in its headers, which declare them `extern const CFStringRef`, not in its documentation,
  and not in its documentation index, which answers only `let kCTFontNameAttribute: CFString`. EIGHT of the
  twenty-two pair by name with a constant the AppKit declares, so their value is provably
  `"NSFont<Suffix>Attribute"`; the other fourteen have no counterpart, and the user's decision is that all
  twenty-two take that form. `PROVENANCE.md` records where each part of that comes from.
- **Two doors are OWED WITH THEIR REASON**: `CTFontDescriptorCreateMatchingFontDescriptors` and its singular
  form answer "based on the original descriptor and system precedence" in Apple's own words, and this tier has
  no registry of installed faces to take a precedence from. They are declared and not defined, so a caller who
  reaches for one gets a link error rather than an invented answer.
- **The type itself is next**: `__CTFontDescriptor` registered through `<CoreFoundation/CFRuntime.h>`, with
  the attributes dictionary as its content and the accessors over it. Then `CTFont.h`.
- **The one thing Apple's headers do not specify** is the character→glyph step — the half the Core Graphics
  plan called out as missing: Apple deprecated CG's glyph doors, so this tree struck them, and **Core Text
  owns that step** (`CTFontGetGlyphsForCharacters`). How it reaches the font's `cmap` is the first
  architectural question of the restart, and it is not answered by a signature.
