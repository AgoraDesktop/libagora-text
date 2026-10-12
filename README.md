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
  doors are C functions — which `../../Fiwix/tools/coretext-sweep.py` states in its own words, measured from
  Apple's index (top-level children: classes 0, protocols 1). The target is `LANGUAGES C` and every
  implementation file is a `.c`. The import had fourteen `.m` files; this will have none.

## The guide, and where it lives

| | |
|---|---|
| Apple's 10.6 headers | **downloaded to `/tmp/apple-sdk106/`** — 16 CoreText headers and 8 from ATS. The fetch recipe is in `PROVENANCE.md`; **the headers themselves are not in this repository and must not be**, because the SDK is not redistributable |
| Apple's documented surface | `../../Fiwix/docs/reference/coretext-apple-surface.txt` — 2032 rows with a type and a status each. **This is the ROW LIST this library works from, and not the file that measures it** — see below |
| the era's ground | `../../Fiwix/docs/reference/coretext-sdk106.txt` — a presence oracle: 1836 names and the header each was found in |
| the plan | `../../Fiwix/docs/design/coretext-plan.md` |

**THE LEDGER IS THE ROW LIST HERE AND NOT THE PROGRESS METRIC, AND THE REASON IS A THING MEASURED RATHER THAN
ASSUMED.** `../../Fiwix/tools/coretext-sweep.py` computes its verdict over `HEADERS =` **the Fiwix tree's own
`userland/CoreText/*.h`** — the Objective-C import this repository DELETED, which still lives there — so its
`--check` says nothing about this library: every row for a type this library has not written yet still reads
`shipped` there, because THAT tree declares it. Run by its real path it is consistent and exits 0, and its
per-kind rows still sum to **1038 shipped, 534 open, 460 struck — THE SAME NUMBERS THE RESTART RECORDED**,
which is the point of saying so: this library's progress cannot move them. (515 of the open rows are the
AAT/SFNT structure vocabulary, a data port of two headers rather than behaviour.) What measures THIS library is
its own suite and `nm -D` on its own library. **AND THE PATHS IN THIS TABLE WERE WRONG TWICE OVER**: `../Fiwix/…`
resolved to a directory that does not exist, because the Fiwix tree is a sibling of `Agora` rather than of this
repository, and the command would have failed before it could mislead.
## The order the headers go in, which is Apple's own include graph

`CTFontTraits` → `CTFontDescriptor` → `CTFont` → (`CTFontCollection`, `CTFontManager`) → `CTParagraphStyle` →
`CTLine` → (`CTRun`, `CTTypesetter`) → `CTFrame` → `CTFramesetter` → (`CTGlyphInfo`, `CTTextTab`,
`CTStringAttributes`) → `CoreText.h`, the umbrella. Read from what Apple's headers include, not guessed.

## Build

Two dependencies, and both are Apple's own two: **Core Foundation** (`libagora-core`) and **Core Graphics**
(`libagora-drawing`). Each is a repository of its own, built and installed on its own into a **shared install
prefix** — the convention `libagora-core`'s README documents — and this library is then configured against
that prefix:

```sh
PREFIX="$PWD/../build-prefix"

# 1. the components below this library, in dependency order -- libagora-dispatch, libagora-objc,
#    libagora-core, then libagora-drawing -- each configured and built from its OWN repository and
#    installed with an overridden prefix (each component's README carries its own step):
cmake --install ../libagora-drawing/build --prefix "$PREFIX"

# 2. this library, against the prefix. MEASURED: this is the recipe that produced the status below.
cmake -S . -B build -DCMAKE_C_COMPILER=/usr/bin/clang-19 -DCMAKE_PREFIX_PATH="$PREFIX"
cmake --build build
ctest --test-dir build
```

- **`ctest` IS THE WHOLE COMMAND.** The loader's search path for the *transitive* `libobjc` is set from the
  imported targets in `tests/CMakeLists.txt`, so no `LD_LIBRARY_PATH` is needed here: `DT_RUNPATH` is not
  consulted for a dependency of a dependency, and `libagora-core.so` records no runtime path of its own, so
  no rpath on the test executable could have covered it.
- **`build-prefix/` sits outside every component's `build/`**, which is the rule the components share: a
  component consumes another only as that component's *built* output, never by building a sibling inside its
  own build tree. Being a sibling of the component directories, it is also outside every component's
  `git status`.
- **`clang-19` is written out because `clang` is not on this machine's `PATH`** — this is the compiler the
  build actually used. The project's rule is clang only; GCC is not a supported compiler.

## Status

- **The leaf is done and the library BUILDS.** `CTFontTraits.h` and `CTFontTraits.c` — the four trait keys,
  `kCTFontClassMaskShift`, the symbolic traits and the eleven stylistic classes — written from Apple's header,
  **with Apple's published values rather than a guess at their order.** MEASURED: `cmake --build build`
  exits 0 with no errors from a clean directory, and `nm -D build/libagora-text.so` lists
  `kCTFontSymbolicTrait`, `kCTFontWeightTrait`, `kCTFontWidthTrait` and `kCTFontSlantTrait` as defined.
- **The values above are held by the suite's first case**, which is what tells a published value apart from a
  plausible one: the trait bits, the class numbers — the gaps at 6 and 11 included, which a library that
  renumbered the classes would close up — and the four trait-key strings are all asserted, the strings
  against the AppKit's own names.
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
- **The first object exists.** `struct __CTFontDescriptor` is registered through
  `<CoreFoundation/CFRuntime.h>` — `CFRuntimeBase` first, a finalizer that releases the attributes, and
  `equal`/`hash` over the dictionary, because a descriptor's attributes ARE its value. The five creators and
  accessors are implemented: name-and-size (with a size of 0.0 **omitted** rather than stored, Apple's own
  sentence), from-attributes, copy-with-attributes (**replacing** what it names and merging the two keys
  Apple excepts), copy-attributes, copy-attribute, and copy-localized-attribute (answering from the attribute
  itself, which is what Apple's documentation says when localization is not possible).
- **AND ITS IDENTITY COMES FROM THE RUNTIME, WHICH IS THE POINT**: `CFGetTypeID(descriptor) ==
  CTFontDescriptorGetTypeID()` is the contract, and the suite holds it. The deleted library answered every
  type id from a table of constants kept in a file beside the objects, which cannot satisfy that contract —
  and the case fails if this one is written that way.
- **Three doors are OWED WITH THEIR REASONS**, each declared and not defined, so a caller gets a link error
  rather than an invented answer: the two matching doors need a registry of installed faces for the "system
  precedence" Apple's own sentence names; and `CTFontDescriptorCreateCopyWithFeature` needs
  `CTFontFeatureTypeIdentifierKey` and `…SelectorIdentifierKey`, which belong to `CTFont.h`, which INCLUDES
  this header and so cannot be included back. It lands with `CTFont.h`, which is next along with `CTFont.h`'s
  own surface.
- **The one thing Apple's headers do not specify** is the character→glyph step — the half the Core Graphics
  plan called out as missing: Apple deprecated CG's glyph doors, so this tree struck them, and **Core Text
  owns that step** (`CTFontGetGlyphsForCharacters`). How it reaches the font's `cmap` is the first
  architectural question of the restart, and it is not answered by a signature.

- **`CTFont` is implemented, and the glyph step — the reason this library exists — is live.**
  `CTFontGetGlyphsForCharacters` maps characters through the font engine behind `CGFont`, and Apple's contract
  for it is held: `true` only when **every** character mapped, with `0` in the slot of one that did not. The
  type, the creators, the accessors, the name table (through the sixteen keys Core Graphics has no door for)
  and the metrics (scaled from font units into points) are answered. MEASURED: `text: 5 of 5 cases passed`.
- **THE NAME ROUTE RESOLVES THROUGH THE ENGINE'S FACE REGISTRY, AND THE SUITE MEASURES IT EVERY RUN** rather
  than a comment asserting it: `CTFontCreateWithName` finds a face once it has been registered — the case
  PRINTS which of two routes answered, and every run says the name route did. An earlier attempt answered NULL
  because the face was unregistered, which is what a NULL there means.
- **THE GLYPH IS NOW GEOMETRY AS WELL AS AN INDEX, WHICH CLOSES THE SECOND HALF OF THE RESTART'S FIRST
  ARCHITECTURAL QUESTION.** `CTFontCreatePathForGlyph` builds a `CGPath` from the face's outline through
  `cg_font_glyph_outline`, and Apple's own sentence for the door — "the path will reflect the font point size,
  matrix, and transform parameter, **in that order**" — is held end to end, each of the three ingredients
  measured: the box doubles with the size, the caller's transform moves it by what it says (a translation of
  (100, 7) shifts the box by (100, 7)), and **the font's own matrix is in the path**, which is the only place
  in this object anything reads it. AND THE TWO NEGATIVES ARE HELD TOO, because that is what tells a real door
  from a plausible one: a glyph with no outline (a space) is an **empty path and not NULL**, while a glyph the
  engine will not load IS NULL — Apple's "an error occurred". MEASURED: `text: 6 of 6 cases passed`, with the
  unloadable-glyph case printing the engine's own `CG-REFUSE: the engine would not load glyph 65534 for its
  outline` above it. FALSIFIED: forcing the font's matrix out of the composition (`m = CGAffineTransformIdentity`)
  makes exactly that case fail — `the matrixed font's outline starts at x 0.193359 and the plain font's at
  0.193359, want a shift of 30` — and `text: 5 of 6 cases passed, 1 failed`; restored, `6 of 6`.
- **Doors OWED, EACH NAMED WITH ITS REASON in `CTFont.c`** rather than defined approximately: the traits, the
  character set and languages, the `fvar`/`avar`/`feat` tables, the raw table doors, the `post` table's
  underline metrics, the slant angle, the vertical translations, the UI-role creator, the string/family/trait
  copies, and the ATS pair.
- **THE BUILD COMES FROM THE PREFIX THE COMPONENTS' OWN READMEs DOCUMENT, AND `ctest` IS THE WHOLE COMMAND.**
  The two dependencies are installed into the shared `../build-prefix`, not a `/tmp` path, and the suite's
  runtime search path is taken FROM THE IMPORTED TARGETS in `tests/CMakeLists.txt` rather than from the
  caller's shell — the transitive `libobjc` needs it, and a dependency of a dependency is not reached by
  `DT_RUNPATH`. MEASURED: `cmake --build build` exits 0 with no warnings from that prefix, and a **bare**
  `ctest --test-dir build` is `1/1 passed` — `text: 6 of 6 cases passed, 0 failed` — with no
  `LD_LIBRARY_PATH` set. See the `## Build` section above.
- **`CTFontCollection` IS DECLARED AND IMPLEMENTED, AND IT IS THE FIRST OBJECT HERE THAT IS A QUESTION RATHER
  THAN A VALUE.** Apple's collection holds WHAT IT WAS ASKED — an array of descriptors, or nothing at all for
  "every face this system has" — plus the caller's options, and its creators are Apple's four. Its value is
  therefore that question, which is what makes the copy door holdable at all: a copy that adds a descriptor asks
  a different question, a copy with nothing to merge asks the same one, and **"every face there is" is
  deliberately NOT equal to "a question with no conditions"** even though neither names a descriptor.
  MEASURED: `text: 8 of 8 cases passed, 0 failed`, and a bare `ctest --test-dir build` is now `2/2`
  (`text` and the new `owed`).
- **ITS TWO MATCHING DOORS ARE OWED, AND THAT IS HELD BY MEASUREMENT RATHER THAN BY THE COMMENT THAT SAYS SO.**
  Apple's result for them is "an array of NORMALIZED font descriptors matching the collection definition": the
  SET is reachable — `cg_font_enumerate` walks the configured directories and the process's registrations — and
  the NORMALIZATION and the ORDER are Apple's, so this tier answers neither, and both doors are declared and not
  defined. The `owed` test reads `nm -D --defined-only` and FAILS if either acquires a definition; FALSIFIED by
  pointing it at a symbol that IS defined, which it names and rejects —
  `these doors are DECLARED AND OWED, and the library DEFINES them: CTFontCollectionGetTypeID`.
- **AND THE OPTION KEY'S VALUE IS RECORDED AS AN INFERENCE, WITH THE REASON IT CANNOT BE PROVED MEASURED.**
  `kCTFontCollectionRemoveDuplicatesOption` has no published value, and the rule that proves the other keys
  cannot be applied here: **at 10.6 the AppKit ships no `NSFontCollection.h` at all** — its font headers are
  `NSFont.h`, `NSFontDescriptor.h`, `NSFontManager.h` and `NSFontPanel.h` — so `NSFontCollection` is post-10.6
  and there is no counterpart in the era to pair the name with.
- **`CTFontManagerErrors` — THE FONT MANAGER'S ERROR VOCABULARY — IS DECLARED AND DEFINED, AND IT CARRIES ONE OF
  APPLE'S OWN MISTAKES RATHER THAN REPAIRING IT.** Apple ships this as a header of its own and
  `CTFontManager.h` INCLUDES it, so it is written first, in Apple's own include order. The five registration
  codes (101–105), the unregistration ones (201–202) and the `CTFontManagerError` typedef are Apple's published
  values copied exactly — AND `kCTFontManagerErrorInUse` AND `kCTFontManagerErrorSystemRequired` BOTH CARRY `202`
  IN APPLE'S OWN HEADER, so an in-use refusal and a system-required refusal are indistinguishable to a caller.
  **THIS LIBRARY KEEPS THE COLLISION** and the suite asserts the two constants are still one value, because a
  library that renumbered one would answer a question the era's header cannot. THE TWO STRINGS ARE INFERENCES
  WITH THE REASON MEASURED: Apple publishes no value for `kCTFontManagerErrorDomain` or
  `kCTFontManagerErrorFontURLsKey`, and the rule that carries the other Core Text keys across to the AppKit's
  spelling CANNOT APPLY — at 10.6 `NSFontManager.h` declares no `extern const …StringRef` at all, and
  `FoundationErrors.h` names its domain only in a comment. MEASURED: `text: 9 of 9 cases passed, 0 failed`.
- **`CTFontManager` IS DECLARED AND IMPLEMENTED, AND IT ANSWERS THE TWO QUESTIONS A FONT AND A COLLECTION CANNOT:
  WHICH FACES THIS SYSTEM HAS, AND MAKE THIS ONE AVAILABLE TO ME.** The three `Copy` doors, the family-name
  comparator, `CreateFontDescriptorsFromURL`, `IsSupportedFont`, `GetScopeForURL`, the four registration doors and
  the auto-activation setting pair are defined; both constants are defined. **THE READ-ONLY HALF IS ONE WALK SEEN
  THREE WAYS** — `cg_font_enumerate` walks the configured directories AND the process's registrations — so a
  registered face comes back through ALL of the name, family and URL doors, which is what the case holds.
  Registration carries **APPLE'S OWN CODES THE WAY APPLE CARRIES THEM**: `AlreadyRegistered` and `NotRegistered`
  arrive as a `CFError` in `kCTFontManagerErrorDomain` whose user-info dictionary names the failing URL under
  `kCTFontManagerErrorFontURLsKey`, and the plural doors answer one error per failed URL. MEASURED:
  `text: 13 of 13 cases passed, 0 failed`.
- **AND ITS ONE SCOPE IS THE PROCESS'S, WITH THE OTHERS REFUSED RATHER THAN FAKED.** Apple's `User` scope is
  "available to ALL processes … and in subsequent sessions" and `Session` covers the session; both need a
  PERSISTENT, CROSS-PROCESS font store this tier does not have — its registry is `cg_font_register`, which is the
  process's. So `GetScopeForURL` answers `Process` or `None` (Apple's own two answers), and a registration asking
  for another scope returns **false with the error out-parameter LEFT UNSET**, because Apple's era has no code
  meaning "unsupported scope" and inventing one is the one thing this library will not do. **A CALLER TELLS THE TWO
  FAILURES APART BY THE ERROR**: set means the file, unset means the scope. The case asserts exactly that,
  including that a refused registration changed nothing.
- **AND THE FAMILY-NAME ORDER IS A RECORDED DEVIATION — IN BOTH DOORS THAT HAVE ONE.** Apple documents the order
  as "the Apple preferred order, accounting for foundry prefix … recognized prefixes are sorted after the
  un-prefixed names", and it turns on **a list Apple does not publish**, so the order cannot be reproduced from the
  era's headers. `CompareFontFamilyNames` therefore does an ordinary localized, case-insensitive, numeric-aware
  comparison, and `CopyAvailableFontFamilyNames` returns the families **in the same order the comparator gives
  them** — so a caller that re-sorts the array gets the order it would have sorted to anyway. The case ASSERTS the
  deviation (`"Adobe Garamond"` before `"Garamond"`, where Apple's rule puts the un-prefixed name first) so that
  removing it fails the suite instead of silently passing.
- **TWO DOORS ARE OWED, AND THE `owed` TEST NAMES THEM ALONGSIDE THE COLLECTION'S TWO**: `EnableFontDescriptors`,
  which toggles descriptors "for font descriptor matching" and therefore waits for the tier that owns matching;
  and `CreateFontRequestRunLoopSource`, which needs a font server on the other end of a port. `InvalidFontData`
  (104) is recorded as **unreachable here** rather than guessed at: telling "not a font file" from "a font file
  whose data is invalid" needs a parse the engine's refusal does not expose.
