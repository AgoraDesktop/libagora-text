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
- **`CTParagraphStyle` IS DECLARED AND IMPLEMENTED, AND IT IS THE FIRST TYPE HERE THAT IS PURELY A TABLE.** Apple's
  header prints a **type and a default value for each of its fourteen specifiers**, and those two facts are the
  whole object: `Create` copies what it is given **by SIZE**, the accessor writes the stored copy **or the default
  when nothing was stored** (Apple's own sentence for the door), and nothing has to know what a setting *means* to
  carry it across. MEASURED: `text: 15 of 15 cases passed, 0 failed`.
- **AND ITS THREE INVALID-INPUT CASES ARE ANSWERED THE WAY APPLE SPELLS THEM OUT, WHICH IS NOT THE SAME WAY
  TWICE.** An unknown specifier **zeroes** the buffer and answers false ("the buffer value will simply be zeroed
  out"); a buffer **too small** for the specifier's type is **not written to at all**, asserted by looking at the
  bytes rather than at the return value, because a partial write of a number is the failure that guards; and a
  setting whose size does not match its specifier is **skipped rather than misread**, which is the
  forward-compatible behaviour Apple asks for.
- **THE SUITE CAUGHT A REAL BUG HERE, WHICH IS THE POINT OF WRITING IT FIRST.** The paragraph style stores a
  scalar as the **address of its bytes** and a CFType as **the object itself**, and the accessor treated both as
  the first: it memcpy'd *from* the array instead of handing the array back, so a style's `TabStops` came out as
  the array object's first eight bytes — **zero — i.e. NULL**. The failure was `text: 14 of 15 cases passed,
  1 failed` with three `FAIL: the TabStops …` lines, and it is invisible for every scalar, which is why the case
  that holds a style's tab stops **through a release of the caller's own reference** is the one that found it.
  Fixed, `15 of 15`.
- **AND ONE ROW IS A RECORDED DEVIATION, WITH ITS LANDING POINT NAMED.** Apple's default for
  `kCTParagraphStyleSpecifierTabStops` is "12 left-aligned tabs, spaced by 28.0 points", and a tab is a
  `CTTextTabRef` — whose header is LATER in Apple's include order, so neither it nor the door that makes one
  exists yet. The default is answered as an **EMPTY ARRAY** of the right type, the case asserts both halves of
  that (the type, and the emptiness), and `ct_para_tab_fallback` is where the twelve tabs will be built.
- **`CTLine` IS DECLARED AND IMPLEMENTED — THE FIRST THING HERE THAT IS BUILT RATHER THAN STORED, AND THE FIRST
  THAT DRAWS.** `CTLineCreateWithAttributedString` makes **one run**: the string's characters become glyphs through
  `CTFontGetGlyphsForCharacters` (the door this library was started for), each glyph is placed by
  `CTFontGetAdvancesForGlyphs`, and the font's own ascent, descent and leading are the line's heights. Every
  measuring door is arithmetic over those — glyph count, string range, typographic bounds, flush offset, trailing
  whitespace, image bounds — the caret pair inverts, and **`CTLineDraw` puts the ink on a page** through
  `CGContextShowGlyphsAtPositions`. MEASURED: `text: 17 of 17 cases passed, 0 failed`.
- **THE SINGLE-RUN BOUNDARY IS STATED RATHER THAN LEFT TO BE DISCOVERED FROM A WRONG LAYOUT.** Apple's own sentence
  for the creator is that "without a typesetter object, the line cannot be properly broken", so this is the simple
  case and it says what it is: **no line breaking, no two fonts in one line, no bidirectional reordering and no
  shaping** — a character's glyph is drawn where its own advance puts it, in the order the characters appeared.
  Multi-run, bidi and complex scripts are the HarfBuzz milestone the Core Text plan already names.
- **AND A RUN DELEGATE IS THE ONE NON-GLYPH THE LINE HONORS** (`CTRunDelegate.h`, `kCTRunDelegateAttributeName`): a
  character carrying one is a BOX — its advance is the delegate's width, the line's ascent and descent grow to cover
  the delegate's, and `CTLineDraw` and `CTRunDraw` skip it, so an attachment or a tab occupies space without being a
  glyph. It is still ONE run, which is why it needs none of the machinery above. MEASURED (`case_run_delegate`): a
  box of width **30** in `ab<c>c` puts the character after it exactly **30** points past the box's own offset, not the
  placeholder glyph's advance; the line has one entry per character, box included; its ascent and descent grew to
  **12** and **3**; and `CTTypesetterSuggestLineBreak` at **40** points stops BEFORE the box, because the fit counted
  its width. WHAT IS STILL OWED: a box in a JUSTIFIED or TRUNCATED line keeps its place but not its delegate
  (`ct_line_with_positions` copies glyphs and positions, not the flags).
- **`CTLineDraw`'S ONE CROSS-FRAMEWORK SEMANTIC IS MEASURED RATHER THAN ASSUMED.** This Core Graphics' own header
  says `CGContextShowGlyphsAtPositions` takes positions "specified in user space" and that "the CTM alone places
  the pen" — unlike the three advance doors, whose pen "starts at the TEXT POSITION" — so the line's origin is
  applied by `CTLineDraw` itself. The case **draws the same line at two text positions into a real bitmap and
  checks the ink moved by 30 pixels**, because a comment cannot tell those two readings apart and a picture can.
- **THREE DOORS **WERE** OWED AT THAT COMMIT, AND THE `owed` TEST NAMED SEVEN THEN — SIX ONCE `CTRun` LANDED, which
  is the next bullet down.** `GetGlyphRuns` — a line's runs are `CTRunRef`s and `CTRun` did not exist yet, since
  Apple's own include order puts its header *after* `CTLine.h`, which is why this header never mentions the type —
  and `CreateTruncatedLine` / `CreateJustifiedLine`, which need the truncation and justification engines: where an
  ellipsis may fall, which clusters may not be split, and where justification slack goes are rules Apple states
  **nowhere** in the header, and spreading the slack evenly would look plausible and be wrong.
- **AND THE FONT-ATTRIBUTE KEY IS THE ONE PLACE `CTLine` REACHES OUTSIDE ITSELF.** A line must read
  `kCTFontAttributeName`, whose header (`CTStringAttributes.h`) is much later in the documented include order, so
  **the value is written once in `CTLine.c`** — with the reason and its removal point — rather than the constant
  invented. Nothing else in the library names the key, and the suite names the same string.
- **`CTRun` IS DECLARED AND IMPLEMENTED, AND IT IS THE TYPE `GetGlyphRuns` WAS WAITING FOR.** A run is what a line
  is made of: its four arrays (glyphs, positions, advances, string indices), the attributes it was laid out with,
  its string range, and a draw over a **range** of it. **`CTLineGetGlyphRuns` IS DEFINED AS OF THIS COMMIT** — it
  was owed for exactly as long as `CTRun` did not exist, which is why Apple's own include order puts `CTRun.h`
  *after* `CTLine.h` — and **the `owed` test lost that name in the same commit**, because that test fails if an owed
  door acquires a definition: the two could only move together. MEASURED: `text: 18 of 18 cases passed, 0 failed`.
- **IT OWNS ITS ARRAYS, WHICH DECIDES TWO THINGS A CALLER CAN SEE.** A run is independently retainable, so it cannot
  be a view over its line's storage: it **copies** the four arrays at creation — a second copy of the glyph data per
  line, paid deliberately rather than answering a retained run with a dangling pointer. And because it always keeps
  them contiguous, **the four `Ptr` doors never answer NULL** where Apple's contract allows it ("the caller should
  be prepared for this function to return NULL even if there are glyphs in the stream"); the case asserts that the
  pointer door and the copying door **agree**, which is a stronger check than either alone.
- **THE RANGE RULE IS APPLE'S OWN SENTENCE AND LIVES IN ONE PLACE.** *"If the length of the range is set to 0, then
  the copy operation will continue from the range's start index to the end of the run."* It is applied by
  `ct_run_clamp`, which all eight array doors and both range-taking measurement doors share — eight copies of the
  rule would be eight chances to disagree. The case holds it, and holds that a **part** of the run measures narrower
  than the whole, which is what the line's own doors cannot do.
- **TWO ANSWERS ARE THIS TIER'S RATHER THAN DERIVED, AND BOTH ARE ASSERTED SO A CHANGE IS DELIBERATE**:
  `CTRunGetStatus` is `NoStatus` and `CTRunGetTextMatrix` is the identity, because this library does no
  bidirectional reordering and sets no run matrix. The three status flags exist for the tier that does.
- **AND ONE NEW HEADER IS DELIBERATELY NOT PART OF THE SURFACE**: `CTRun_internal.h` carries the constructor seam
  between a run and the line that makes one — Apple has no public creator for a run, since a line is what produces
  one — so that the two files agree about how a run is built instead of one of them `extern`ing what the other
  defines.
- **AND A LINE IS KERNED, WHICH IS THE DIFFERENCE BETWEEN SPACING AND TYPESETTING.** An advance is a glyph's own
  opinion of how far the pen moves; a KERN is the font's opinion about a PAIR, and `CTLine` applies it between every
  neighbouring pair — MEASURED, `To` closes by 2.209 points at 13 in one face and 1.016 in another. **THE
  TYPESETTER'S WIDTH HAD TO LEARN IT TOO**: `ct_ts_fit` measured a line's fit from the advances alone, so with the
  positions kerned the fit and the line disagreed and a title measured to one width and laid out to the other
  WRAPPED — MEASURED in the AppKit's menu ink test as the first column's ink falling from 339 pixels to 147. The
  number crosses the files in `CTFont_internal.h`, because Apple's Core Text has NO public kerning door: the
  adjustment belongs to the typesetting layer and never leaves it as a number.
- **`CTTypesetter` IS DECLARED AND IMPLEMENTED, AND IT IS THE FIRST THING HERE THAT **DECIDES** RATHER THAN MEASURES.**
  Its whole algorithm is Apple's own sentence: *"The line break can be triggered either by a hard break character in
  the stream or by filling the specified width with characters."* So a newline ends the line **wherever it is**, and
  otherwise characters are taken until the width is full. `CTTypesetterCreateLine` turns a break into a line,
  `SuggestClusterBreak` asks the same question without the contextual part, and both option constants are defined.
  MEASURED: `text: 20 of 20 cases passed, 0 failed`.
- **AND THE PARAGRAPH STYLE REACHES IT, WHICH IS THE FIRST TIME A RECORDED SETTING CHANGES WHAT THIS LIBRARY DOES.**
  The typesetter reads the string's paragraph style (through the second attribute key this library carries as a
  recorded value, `kCTParagraphStyleAttributeName`) and honours the **line-break mode**: word wrapping backs off to the
  last space that fits and **includes it** — a line's trailing whitespace is part of its range, which is why
  `CTLineGetTrailingWhitespaceWidth` exists — while char wrapping breaks at the width, and clipping/truncating return
  the whole remainder because the DRAWN result is those modes' business. **THE CASE PROVES IT BY DIFFERENCE**: the
  same string at the same width breaks after **4** characters under the default and after **7** under a
  char-wrapping style.
- **THE TWO BREAKING DOORS DIFFER IN EXACTLY THE WAY APPLE NAMES.** *"similar to a character break, except that it will
  not break apart linguistic clusters. No other contextual analysis will be done"* — so the cluster door never backs
  off to a space and reads no paragraph style. Seven advances of `"one two three"` gives **4** from the line door and
  **7** from the cluster door, and the case asserts both numbers rather than that they differ.
- **WHAT IS NOT ATTEMPTED IS SAID WHERE A CALLER READS IT**: no UAX #14, no hyphenation, and no bidirectional
  reordering at all — so `kCTTypesetterOptionDisableBidiProcessing` is **recorded and cannot change an answer**, which
  is the difference between a door that cannot do something and one that pretends it did.
- **AND A TYPESET LINE NEEDS A LINE OVER A **RANGE**, WHICH NO PUBLIC HEADER NAMES.** `CTLine_internal.h` is that seam
  — and it carries the font-key accessor too, so the value is written **once** and the line and the typesetter cannot
  disagree about which attribute names a font. The case holds the property that makes it necessary: a line cut from
  characters 4..6 reports the range **(4, 3)**, in the STRING's coordinates rather than its own.
- **`CTFrame` AND `CTFramesetter` ARE DECLARED AND IMPLEMENTED, AND THE TEXT-LAYOUT HALF OF APPLE'S ORDER IS DONE.**
  A framesetter is *a typesetter with a shape to fill*: its five doors are `CreateWithAttributedString`,
  `CreateFrame`, `GetTypesetter`, `SuggestFrameSizeWithConstraints` and `GetTypeID`, and Apple's own sentence is the
  algorithm — fill "until it either runs out of **text** or it finds that text no longer **fits**". A frame's ten doors
  report the range it was asked for, the **prefix of it that fit** (two different doors, and the pair is the point), the
  path, the frame attributes, its **lines** and their **origins**, and draw them. MEASURED:
  `text: 21 of 21 cases passed, 0 failed`.
- **THE FRAME'S LAYOUT IS SIX LINES AND EVERY DECISION IN IT IS SOMEBODY ELSE'S**: the typesetter says where each line
  ends, the line says how tall the text is, the path says how much room there is, and the framesetter adds only the
  arithmetic between them. The first baseline is **one ascent below the path's top** and each line after it is one line
  height lower, which is what makes a frame's origin its **bottom-left** — and the case holds the property that
  matters: the lines **tile** the visible range (first line starts where the range does, each starts where the last
  ended, no gap and no overlap), and the origins **descend** and sit inside the box.
- **AND ITS TWO BOUNDARIES ARE RECORDED WHERE A CALLER READS THEM.** THE PATH IS READ AS ITS BOUNDING BOX: this tier
  fills the path's *box*, which is exact for every rectangle and wrong for a circle or a notch, because the rules for
  filling an arbitrary outline are the text system's and not a signature's. AND THE PROGRESSION IS READ AND NOT
  HONOURED: `CTFrameProgression` exists for **vertical** text, where lines stack in columns, and this tier lays out
  horizontally only — the frame attributes are kept whole and change no origin.
- **A FRAME HAS NO PUBLIC CREATOR EITHER, SO THERE IS A THIRD INTERNAL SEAM.** `CTFrame_internal.h` is where the
  framesetter and the frame agree about how one is built — the lines and their origins arrive **together**, because a
  frame without either is not a frame — and the arrays are **copied**, so a frame cannot be left pointing at a
  framesetter's freed stack.
- **`CTStringAttributes` IS DECLARED AND IMPLEMENTED, AND IT RETIRED TWO RECORDED BOUNDARIES.** Fourteen attribute
  keys and the two underline vocabularies — and `kCTFontAttributeName` and `kCTParagraphStyleAttributeName` are now
  read from **this** header by `CTLine.c` and `CTTypesetter.c`, which each carried the value as a **local literal**
  because the header did not exist. The comments that said *"this function goes away the day `CTStringAttributes.h`
  lands"* have been **paid**: the value exists in **exactly one place**, and the suite's substitution was checked by
  every earlier case still passing. MEASURED: `text: 22 of 22 cases passed, 0 failed`.
- **AND TWELVE OF THE FOURTEEN KEYS ARE NOW PROVEN RATHER THAN INFERRED.** The crossing rule is the font descriptor's
  — **a dictionary crosses between Core Text and the AppKit**, so a key's string must be the one the other framework
  reads it by — and the **measurement** is Apple's own `NSAttributedString.h`, which at 10.6 declares **twenty-six**
  `NS…AttributeName` constants. **Twelve of the fourteen pair exactly**; the two that do not are
  `kCTForegroundColorFromContextAttributeName` and `kCTVerticalFormsAttributeName`, and each keeps the standing the
  library's other unprovable values have. The case names all fourteen **and asserts the count of provable ones**, so a
  key moving between standings is a deliberate act.
- **AND THE UNDERLINE VOCABULARY HAS A GAP WORTH HOLDING.** The styles are **0x00, 0x01, 0x02 and 0x09** — so a library
  that COUNTED the members of that enum would put `Double` at 3 and be wrong from there on — and the patterns sit in
  the **high byte**, which is what lets a style and a pattern combine into the one number the attribute carries. The
  case asserts the gap, the byte, and `Single | Dash == 0x0201`.
- **`CTTextTab` IS DECLARED AND IMPLEMENTED, AND IT PAYS THE THIRD RECORDED DEVIATION.** Five doors over three
  values — an alignment in the PARAGRAPH's own vocabulary, a location in points, and an options dictionary this tier
  **retains and does not read**, because Apple names no key for one anywhere in the era's headers. **AND
  `kCTParagraphStyleSpecifierTabStops` NOW ANSWERS APPLE'S DEFAULT INSTEAD OF AN EMPTY ARRAY**: while this header did
  not exist that row could not be built, so it answered an empty array and its comment named its own landing point —
  *"the tabs arrive with CTTextTab, and this function is where they will be built"*. THEY HAVE ARRIVED: twelve
  **left-aligned** tabs at **28.0, 56.0 … 336.0**, and the case asserts the count, the alignment of each one and those
  locations rather than merely that the array is not empty. MEASURED: `text: 23 of 23 cases passed, 0 failed`.
- **AND THE TAB IS PROOF THAT THIS LIBRARY'S DEVIATIONS ARE DEBTS RATHER THAN DECISIONS.** Three have now been paid in
  the order they were recorded — the two attribute-key values, by `CTStringAttributes`, and this default, by
  `CTTextTab` — each in the commit that brought the thing it was waiting for, and each checked by the cases that were
  ALREADY asserting the deviation: a retirement that changed behaviour would have failed those cases rather than paid
  them.- **`CTGlyphInfo` IS DECLARED AND IMPLEMENTED — THE LAST OF THE VOCABULARY, AND THE VALUE OF A KEY ITS SIBLING ALREADY
  NAMES.** (`kCTGlyphInfoAttributeName`.) Its three creators name ONE glyph three ways: by **name** — resolved through
  the face with `CTFontGetGlyphWithName`, and the name **kept** so the getter answers what it was given — by **index**,
  and by **CID in a `CTCharacterCollection`**. The case holds what each one CAN answer, which is not the same thing
  three times: an index-made glyph info stores no name and the getter **resolves it lazily through the engine's own
  glyph-name door** (which is why that door is at the seam, and where a NULL would have been a shortage rather than a
  boundary); and **a CID-made one keeps its collection and has NO GLYPH AT ALL**, because a CID is a key into a
  collection's CMap and this tier has no CMap — asserted **as** a boundary. MEASURED: `text: 24 of 24 cases passed, 0 failed`.
- **AND ITS COLLECTION VOCABULARY HAS NO GAPS**, the opposite shape to the underline one the attribute case holds:
  `kCTIdentityMappingCharacterCollection` is 0 and the five Adobe collections are 1 through 5 in Apple's order, so this
  enum may be counted where that one may not.
- **`CoreText.h` IS THE UMBRELLA, AND WITH IT APPLE'S OWN INCLUDE GRAPH IS COMPLETE — FOURTEEN HEADERS IN THE ORDER
  THEY INCLUDE EACH OTHER.** A caller who wants the framework rather than a part of it now writes one line. IT CARRIES
  THE ONE DOOR NO OTHER HEADER COULD DECLARE — `CTGetCoreTextVersion`, because a version is a fact about the
  **framework** — and the six `kCTVersionNumber…` constants, which are **macros** in Apple's header and therefore
  macros here: a caller compares them without linking anything. **AND IT ANSWERS `kCTVersionNumber10_6`**, which is a
  claim about the SURFACE and not about the platform: every header in this library was written from the era's, so a
  10.6 caller asking for 10.6 gets the answer it expects and finds the surface it expects. MEASURED: `text: 25 of 25 cases passed, 0 failed`.
- **AND THE UMBRELLA IS VERIFIED AS A UMBRELLA RATHER THAN AS A FILE.** The suite includes it **FIRST**, before the
  individual headers, so that every door below is reachable *through it*; and the case asks each of the **eleven**
  headers that declare a type for its type id **through the umbrella**, calls the version door, and checks Apple's own
  recommended use of it (the pointer is callable and the number is at least 10.5). An include list that lost a header
  would fail that case rather than a caller months later.
- **AND ONE HEADER IS ABSENT FROM IT, WHICH IS APPLE'S OWN OMISSION REPRODUCED.** `CTFontManagerErrors.h` is not in
  Apple's umbrella list and not in ours; a caller who includes the umbrella still **has** it, because
  `CTFontManager.h` includes it — which is why that header was written first among its neighbours. Naming it here
  would be a kindness that made this file disagree with the framework it duplicates.- **`CTLineCreateTruncatedLine` IS IMPLEMENTED, AND IT IS THE FIRST OWED DOOR TO BE BUILT FROM ANOTHER DOOR'S
  ARRIVAL.** Its recorded reason was *"it waits for the typesetter, whose job the breaking is"* — **and the typesetter
  landed, so the door did.** `CTLine.c` implements Apple's printed contract and nothing more: a line that **fits** comes
  back whole, the token goes where the cutting happened, a **NULL** token cuts the line off, and a token **wider than the
  width** answers NULL — with the three types as Apple's own descriptions of which end stays visible, which the case
  MEASURES (an END truncation keeps the line's first characters, a START keeps its last, a MIDDLE spans both).
  **AND ONE REFUSAL IS HELD**: a token drawn in **another face** answers NULL, because a line here holds one font and the
  token's glyph indices mean nothing in the other — Apple's answer is a multi-run line, and this tier refuses rather than
  putting garbage where an ellipsis belongs. The result needed one thing the library did not have — **a line over GLYPHS
  rather than over a string range** (`CTLineCreateWithGlyphs`), because a MIDDLE truncation keeps a prefix **and** a
  suffix and so cannot be made from any one range. MEASURED: `text: 26 of 26 cases passed, 0 failed`, with `owed` down to **5** names.
- **AND ITS CASE'S FIRST RUN WAS WRONG IN THE INSTRUCTIVE DIRECTION.** It asserted that a token in another font was
  refused **even when the line fit** — which would have made a door that refuses lines it never had to touch. Apple's
  condition is about the **line's** width, and the token is only reached when there is cutting to do; the case now
  asserts that round the other way as well (a line that fits comes back whole even with a token this tier would
  refuse).- **`CTLineCreateJustifiedLine` IS IMPLEMENTED, AND WITH IT `CTLine` OWES NOTHING.** Its header prints LESS than the
  truncation engine's — a factor of 0 or less justifies nothing, 1.0 or more is full, and a narrower width "squishes"
  the line — so **the two things it does not print are DECIDED AND NAMED**: the slack goes to the **spaces**, which is
  what "fully justified" means and why the paragraph vocabulary says a last line "is naturally aligned" (it has no gap
  to widen), and the factor is the **fraction** of the slack taken up, linearly (Apple prints no curve for its "varying
  degrees"). A line with **no space** comes back unchanged; a line with room has its gaps widened **after each space**,
  and the case reads the positions through the run and holds that the **first** glyph does not move while the word after
  the first space does. MEASURED: `text: 27 of 27 cases passed, 0 failed`, with `owed` down to **4** names — **and none of them in this type**.
- **AND THE THREE DOORS `CTLine` ONCE OWED LEFT IN THE ORDER THEIR REASONS BECAME FALSE**, because every one of those
  reasons was a CONDITION that could be checked: `GetGlyphRuns` waited for `CTRun` to exist; `CreateTruncatedLine`
  waited for "the typesetter, whose job the breaking is"; `CreateJustifiedLine` waited for a decision about where
  justification slack goes — and that decision is made and written down rather than left as a sentence. **A REASON THAT
  CANNOT BE CHECKED IS A SENTENCE, NOT A REASON.**- **THE MATCHING TIER IS DECLARED AND IMPLEMENTED, AND FOUR DOORS LANDED WITH IT.** The descriptor's two and the
  collection's two are the SAME QUESTION one level apart: **which installed faces does this query describe**. THE SET is
  the engine's own list tested against a query — every attribute the query names must be present and equal, and a query
  with **none** describes every face — and **THE ORDER IS THE USER'S DECISION, RECORDED**: Apple defers it to "system
  precedence" and prints no rule for it, so the array is sorted **by family name** with
  `CTFontManagerCompareFontFamilyNames`, which is stable and reproducible (the property a caller's own sorting needs,
  and itself a recorded deviation from Apple's UI order). The collection's sorted door is Apple's own way out: the
  callback *"defines the sort order"*. MEASURED: `text: 28 of 28 cases passed, 0 failed`, with `owed` down to **2** names.
- **AND THE DESCRIPTOR DOOR'S SECOND PARAMETER IS A `CFSetRef`, NOT AN OPTIONS DICTIONARY — a correction my own first
  draft got wrong**, on the assumption that a Core Text door's second parameter takes options. It is
  `CFSetRef mandatoryAttributes`, and it **FILTERS THE RESULT**: every returned descriptor must carry those attributes,
  so demanding one this tier does not build gives an **EMPTY ARRAY** rather than an approximate face — the requirement
  cannot be met, and an empty answer says so. The case holds both directions.
- **AND IT CLOSED A HOLE IN THE `owed` TEST.** The descriptor's two matching doors had been declared and undefined
  since the descriptor was written; the test was built LATER, when `CTFontCollection` landed, and never picked them up
  — so two doors escaped the very check that test exists to make. They are defined now, and the test names what is
  left.- **AND THE `owed` LIST SHRANK BY FOUR, WHICH FALSIFIED ONE MORE REASON** — the one `CTFontManagerEnableFontDescriptors`
  carried: *"matching is the thing this tier does not have … It lands with the tier that owns matching, beside them."*
  **THAT TIER EXISTS NOW**, so the door is buildable and what it waits for is no longer a tier but a **DECISION**: it
  enables and disables fonts from a descriptor array, which is **process-global mutable state — the first this library
  would have**, since every object in it is immutable by construction, while Apple's header promises that all of its
  functions *"are thread safe unless otherwise specified"*. **A lock, or a recorded caveat, is the decision**; choosing
  one on the way past is what this tree refuses. `CTFontManager.c` now says exactly that in place of the reason the
  matching tier made false. MEASURED: `text: 28 of 28 cases passed, 0 failed`, `owed` at **2** — one needing that decision, one needing the font server
  itself.- **AND THE VISIBILITY DOOR LANDED, WITH THE DECISION MADE RATHER THAN DEFERRED: THE LOCK.** The long-standing
  reason *"It lands with the tier that owns matching, beside them"* was met, so `CTFontManagerEnableFontDescriptors` is
  **defined** — in `CTFontMatching.c`, because that is where its state lives — and the state it changes is **the first
  mutable global this library has**. It is guarded by a **statically initialised mutex** rather than caveated, because
  Apple's own header promises that all of its functions *"are thread safe unless otherwise specified"*; and **the list is
  never handed out** (a caller gets a boolean), so no caller can hold the lock at all.
- **AND THE CASE IT CAME WITH FOUND A REAL BUG, WHICH IS WHAT WRITING IT WAS FOR: `CFEqual` WAS THE WRONG QUESTION.** The
  first draft compared the disabled descriptor against each candidate with `CFEqual`, and the case **failed at once** —
  the descriptors this tier **builds** for a face carry **three** attributes (its URL, its name and its family), while
  the descriptor a **caller** hands the door carries only what the caller wrote down, so the two are never equal and a
  disabled face stayed matchable. **THE DISABLED LIST IS THEREFORE A LIST OF QUERIES, EXACTLY AS A FONT COLLECTION IS**:
  a face is disabled when some descriptor in the list **describes** it — the same `ct_match_satisfies` test a matching
  query gets — so a caller disables a font by **naming** it and re-enables it by naming it again. MEASURED: `100% tests passed, 0 tests failed out of 2` over
  `text: 29 of 29 cases passed, 0 failed`, **135 public doors**, and `owed` down to **1**.- **AND THE LAST OWED DOOR HAS A DECISION BEHIND IT NOW RATHER THAN A SENTENCE: the Agora font service.** The
  maintainer chose to define one, and `FONT-SERVICE.md` records its shape — **DESIGNED, NOT BUILT**, with the door still
  declared and not defined, and the `owed` test still naming it. The survey behind it is MEASURED and it changes what
  the work is: **Agora's Core Foundation already has the port namespace** (`CFSocketRegisterSocketSignature` and the
  name-registry family), the run-loop integration (`CFSocketCreateRunLoopSource`), the codec (`CFPropertyListCreateData`)
  and a name to derive from (`kCTFontManagerBundleIdentifier`) — while **`CFMessagePort` is absent** and
  **`window-server/` is an empty skeleton**, so this would be **Agora's first server**. The honest reason for a daemon is
  now stated rather than assumed: a responder registers a per-process port, and **only a system component can enumerate
  processes to find one.**- **AND THE FONT SERVICE'S WIRE IS BUILT — AND ITS CASE FOUND FOUR REAL FAULTS BEFORE IT PASSED, ONE OF THEM A DESIGN
  FAULT NO PROBE WOULD HAVE CAUGHT.** `CTFontService_internal.h` / `CTFontService.c` carry **both halves of the
  conversation** — a requester's encode and a responder's answer, produced by calling the matching tier — with no socket,
  no port and no daemon, so it is testable in-process. The faults, in the order they were found: the format was asked for
  as **BINARY** and never measured; the version was **released although a `CFSTR` is a constant**; the decoded request
  was **leaked on the reply path**; and the blocker — **A MATCHED FACE'S ATTRIBUTES CARRY `kCTFontURLAttribute`, A
  `CFURL`, AND A PROPERTY LIST CANNOT HOLD ONE**, so the reply never encoded at all. The wire now carries
  **property-list types and nothing else**, crossing a URL as its string. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 30 of 30 cases passed, 0 failed`, **137 public doors**,
  `owed` at **1** — the door itself, waiting on its transport and on one build flag.- **AND THE `owed` TEST WAS BLIND TO TWENTY-TWO DECLARED DOORS — FOUND BY COMPARING APPLE'S HEADERS WITH OURS, WHICH
  IS THE CHECK THE LIBRARY DID NOT HAVE.** `nm -D` on the built library against Apple's 15 `CT*.h` gives an arithmetic
  that closes: **151 Apple-declared names, 127 defined, 24 unaccounted — 23 declared here and undefined, plus 1 Apple
  declares and we never mention** (`CTFontCreateWithQuickdrawInstance`, now recorded in `CTFont.c` with its reason: a
  drawing system Agora does not have). ALL 23 ARE HONESTLY DOCUMENTED — `CTFont.c`'s owed block names twenty-two of
  them with a checkable reason each: the font-table readers (`fvar`, `avar`, `feat`, `post`, raw `CTFontCopyTable`) and
  the doors where a SYSTEM CHOOSES A FACE (UI roles, cascade/fallback). WHAT WAS WRONG WAS THE GUARD: the `owed` test
  named **one** of the twenty-three, so a door that quietly acquired a definition would have passed it. The list is now
  all twenty-three. MEASURED: `text: 30 of 30 cases passed, 0 failed`, `owed` **23** names.- **AND ONE OF THE TWENTY-THREE OWED DOORS IS DEFINED: `CTFontCopyCharacterSet`, answered by the INVERSE OF THE GLYPH
  STEP THIS LIBRARY ALREADY HAD.** The seam maps a BATCH of code units per call, so the whole basic multilingual plane
  is **sixteen calls** and no table access is needed — which is exactly why this door did not have to wait for the seam
  to grow one, where the rest of its family does. **A code unit is in the set when the face maps it to a NON-ZERO glyph**
  (`0` is `.notdef` — "nothing here"), so surrogates fall out by themselves and a face that maps `U+0000` keeps it. The
  case holds the equivalence that IS the contract: **membership and the mapping door must agree**, code unit by code
  unit, over a sample including a character the face has, one it may not, and one that is not a character at all.
- **AND ONE BOUNDARY IS RECORDED WHERE THE DOOR IS, because the answer is narrower than Apple's**: the seam takes
  **UTF-16 code units**, so the scan covers the **BMP** and coverage of the planes above it is **not reported** — a
  surrogate pair is two code units and the second alone maps to nothing. Apple reads a cmap and would include them. The
  condition that closes it is a seam door taking **scalar values**, and nothing else about this door would change.
- **AND THE REST OF THE OWED FAMILY NOW HAS A NAMED KEYSTONE**: the font seam sits on **FreeType**, and
  `libagora-drawing/CGFont.c` **already includes `FT_TRUETYPE_TABLES_H`** — so `FT_Load_Sfnt_Table` is one function
  away, and with it `fvar`/`avar` (variation), `feat` (features), `post` (underline metrics), and `OS/2`+`head`
  (traits, whose current reason is that guessing them from a subfamily STRING *"would be a guess wearing a fact's
  clothes"* — a table read is the fact). **That door belongs to another repository**, so the family is a cross-repo unit
  rather than a gate. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 31 of 31 cases passed, 0 failed`, and `owed` down to **22**.- **AND TWO MORE OWED DOORS ARE DEFINED: `CTFontCopyTable` and `CTFontCopyAvailableTables` — the pair of the table family
  that needed NO PARSING AT ALL**, because Apple's answer for a table *is* the file's bytes. They were owed with the
  reason that **no seam door reads a table**, and that reason is now **false rather than lifted**: `libagora-drawing`
  grew `cg_font_copy_table` and `cg_font_table_tags` in its own commit, and **the owed block was corrected in the SAME
  commit that used them** — which is this tree's rule, since an owed reason is a *condition* and a condition that
  becomes true moves a door. This is the first time a reason changed **because of work in another repo**.
- **AND THE CASE HOLDS THE PART THAT IS NOT NOTHING**: **the directory and the table door must agree** — every tag the
  face lists is one the door can produce, a tag no face carries answers `NULL` rather than inventing a table, and the
  bytes are **that table's** (`head` begins with its own version, `0x00010000`, which is a fact about the format and not
  about this library). The **option is obeyed trivially**, and that is the honest form of "not obeyed":
  `kCTFontTableOptionExcludeSynthetic` asks for tables that are not synthesised, and **nothing this door returns is
  synthesised** — every table comes out of the file the face was opened from, so there is nothing to exclude.
- **AND WHAT IS LEFT OF THE FAMILY IS NOW PARSING AND NOT ACCESS**: `fvar`/`avar` (variation, `CTFontCopyVariationAxes`
  and `CTFontCopyVariation`), `feat` (features), `post` (the underline metrics) and `OS/2`+`head` (traits) all read
  through the same two seam doors and each needs the table's **versioned binary layout** understood on this side. The
  owed block now says exactly that, one table at a time. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 32 of 32 cases passed, 0 failed`, **140 public doors**, `owed` down to
  **20**.- **AND THE FIRST TWO DOORS THAT READ A TABLE AND INTERPRET IT: `CTFontGetUnderlinePosition` and `…Thickness`, out of
  `post`.** Their reason said *"the `post` table; the seam reports no underline metrics"* — the seam reads the table now,
  and **the two fields sit at fixed offsets 8 and 10 in EVERY version of `post`** (versions differ in what *follows*
  them), so **there is no version check to get wrong** — unlike `OS/2`, whose version decides whether a field exists at
  all. **The parse is two big-endian signed 16-bit values**, which is the one thing about tables that is easy to get
  wrong and impossible to notice: a little-endian read gives a *plausible* number, not an error.
- **AND THE CASE DOES THE INTERPRETING ITSELF, which is the only way to hold a parser to anything**: it reads `post`
  through the table door, takes the two values at their offsets big-endian, scales them by the point size itself, and
  requires both metric doors to agree. **A parser checked against its own output checks nothing.** The case also holds
  that the answers are **scaled and not raw** — a font unit is a fraction of a point, so a metric in the tens would mean
  the scale was never applied.
- **AND THE SIGN IS THE TABLE'S, NOT INVERTED, AND THAT IS RECORDED WHERE THE DOORS ARE**: a negative `underlinePosition`
  places the line **below the baseline**, which is `post`'s own convention and how the value is written down. Apple's
  header says the door returns *"the scaled underline position"* and says nothing about a sign — so inverting it would be
  a guess about a convention neither document states, and a caller who wants the other sign is one subtraction away.
  MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 33 of 33 cases passed, 0 failed`, **142 public doors**, `owed` down to **18**.- **AND THE SYMBOLIC TRAITS ARE READ RATHER THAN GUESSED: `CTFontGetSymbolicTraits`, out of `OS/2.fsSelection`,
  `head.macStyle` and `post.isFixedPitch`.** The reason it was owed was a **REFUSAL, not a shortage** — *"deriving traits
  from a subfamily STRING would be a guess wearing a fact's clothes"* — and **the fact was in three tables the whole
  time.** Where the format states the same thing twice, **either source setting it sets the trait**; and
  **`usWidthClass` is consulted for the first time**: 1–4 condensed, 6–9 expanded, **5 is the middle and says neither**
  (the field's *own* meaning, not a threshold anybody picked), while **0 means the table says nothing** — not "medium".
- **AND FOUR TRAITS ARE DELIBERATELY NOT SET, RECORDED RATHER THAN ASSUMED**: the **class mask** is a mapping from
  `sFamilyClass` that no era header prints (the header itself says the classes *"correspond closely"*, which is a
  correspondence and not a table); **vertical** needs the font's direction, which no door here reports; `UIOptimized`
  and the modern colour/composite traits have no source in these tables at all. **A trait this door does not know is left
  out rather than guessed** — a caller who checks one bit can survive silence where a plausible wrong bit is believed.
- **AND THE SPELLING IS THE ERA'S, which cost one attempt**: this header declares `kCTFontBoldTrait`, not the modern
  `kCTFontTraitBold`, and the first draft assumed the modern names — **an assert against `CTFontTraits.h` stopped it
  before a line was written**, which is the whole argument for checking a name instead of remembering it. The case holds
  three things, and the third keeps the other two honest: the loaded face is named `-Bold` so bold **must** be set; the
  door **must** agree with `OS/2` and `head` recomputed from their raw bytes; and **the tables must actually state it**,
  since a check that passes with both sides silent proves nothing. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 34 of 34 cases passed, 0 failed`, **143 public doors**, `owed`
  **17**.- **AND `CTFontCopyVariation` IS DEFINED — WITHOUT A TABLE, BECAUSE ITS REASON WAS WRONG ABOUT IT.** The reason said the
  variation doors were *"the `fvar` and `avar` tables"*; **Apple's own words say otherwise**: this door returns *"the
  current variation **instance**"*, while the axes door returns *"an array of variation **axes**"* — **an instance is a
  CHOICE and an axis is a CAPABILITY**, and the choice is the descriptor's, which this tier has been holding all along.
  `fvar` is still owed for the axes door, and the owed block now says so — **with a note for whoever takes it: no variable
  font is installed here, so a case could only exercise the no-`fvar` path.**
- **AND ONE BOUNDARY IS RECORDED RATHER THAN IMPLIED BY THE DOOR'S PRESENCE: this tier can REPORT the instance it was
  asked for and CANNOT APPLY it.** Nothing in the seam sets a variation coordinate on a face, so the glyphs a caller
  draws are the **default** instance's. The door answers what the font *represents* — honest — and that is not the same
  as what it *renders*, which needs a seam door that does not exist. A font with no instance answers **NULL** rather
  than an empty dictionary, a choice recorded as one.
- **AND THE DOOR EXPOSED A REAL BUG IN FONT CREATION**: `CTFontCreateWithFontDescriptor` read the **name** out of the
  caller's descriptor and let every other attribute fall — **against this file's own struct, whose field says "RETAINED:
  what this font was asked for."** What it cost was **every attribute that is not a name**: a variation instance, a
  feature setting, a trait request. The font now keeps **the caller's attributes merged with the size it actually
  resolved** (from the font, not the parameter, because the parameter is allowed to be `0.0`). MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 35 of 35 cases passed, 0 failed`,
  **144 public doors**, `owed` **16**.
- **AND THREE MORE DOORS ARE DEFINED, OF WHICH TWO HAD REASONS THAT WERE FALSE RATHER THAN UNMET.** `CTFontCopyVariationAxes`
is the `fvar` parse — AND THE NOTE THAT SAID "NO VARIABLE FONT IS INSTALLED ON THIS MACHINE, SO A CASE HERE COULD ONLY
EXERCISE THE NO-`fvar` PATH" WAS WRITTEN WITHOUT LOOKING: `/usr/share/fonts/opentype/cantarell/Cantarell-VF.otf` is a
variable face (`wght`, 100–800, default 400), so the case now exercises the REAL path, reads `fvar` itself through the
public table door and recomputes every number. **`avar` is not owed by this door either** — it maps a coordinate when one
is SET and is not one of the five facts an axis dictionary carries — making this the SECOND reason in this file corrected
rather than quietly implemented. THE LAYOUT IS READ FROM THE TABLE (`offsetToData`, `axisSize`) rather than from a
version's offsets, the version is checked anyway, a face with no `fvar` answers an EMPTY ARRAY (a recorded choice) and a
version this parser does not know answers NULL.
- **`CTFontCopyFeatureSettings` IS NOT THE `feat` DOOR IT WAS OWED AS.** `CTFontCopyFeatures` answers what a face OFFERS;
this door answers "the NON-DEFAULT settings that should be applied to the font, or NULL if the default settings should be
used" — WHAT THE FONT WAS ASKED FOR, which is a choice its descriptor has held all along. Same shape of correction
`CTFontCopyVariation` needed, one unit earlier. The FILTER that would drop a setting equal to the face's own default is
recorded as a boundary: it needs `feat`'s default flags, and `feat` is still owed (NO FACE INSTALLED HERE CARRIES ONE,
measured over every installed face — so a parse could not be held by a case).
- **AND `CTFontDescriptorCreateCopyWithFeature` FOUND A REAL BUG IN THE DESCRIPTOR'S COPY DOOR — one that had been
unreachable for three units.** Apple names `kCTFontFeatureSettingsAttribute` among the keys a copy MERGES, its value is an
ARRAY of setting dictionaries, and the merge branch merged DICTIONARIES ONLY: a caller toggling a second feature silently
dropped the first. There was no supported way to build a two-entry settings array before this door, which is why the bug
waited for it and why the case was written WITH it. The merge is now BY (TYPE, SELECTOR). FALSIFIED: with the array branch
forced out, exactly that case fails on four lines and the suite is `36 of 37`; restored, `37 of 37`. The fix needed a
SEAM, because the two keys identifying a setting belong to `CTFont.h`, which includes the descriptor's header and cannot
be included back — `CTFontDescriptor_internal.h` carries it and the merge is defined in `CTFont.c`, the tree's own rule
for this ("so that the two files agree … instead of one of them `extern`ing what the other defines").
- **AND ONE STALE PARAGRAPH OF THE OWED BLOCK WAS CORRECTED, WHICH IS WHAT THE OWED LIST IS FOR.** `CTFont.c` still said the
descriptor's two matching doors were owed for want of a registry of installed faces; the registry exists and the doors are
defined. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 37 of 37 cases passed, 0 failed`,
  **148 `CT…` definitions**, and `owed` down to **13**.
- **AND THE FOUR CREATORS THAT ASK A REGISTRY RATHER THAN A NAME ARE DEFINED — THREE OF THE FOUR ON A REASON THAT HAD
GONE FALSE: `CTFontCreateCopyWithSymbolicTraits`, `CTFontCreateCopyWithFamily` and `CTFontCreateForString` all waited
on "a face … to be found", and the walk that finds faces has been in `CTFontMatching.c` since the matching tier
landed. The fourth, `CTFontCreateUIFontForLanguage`, waited on a reason that is STILL TRUE ("a ROLE is a request to a
system that chooses a face") and is answered by a **recorded decision**: the roles are answered against the faces the
registry has, in the matching tier's order, the fixed-pitch role by the **monospace trait** (a fact read from the
tables) and the emphasized roles by `CTFontCreateCopyWithSymbolicTraits`, so an emphasized role is the same search a
caller could make. **AND THE BOUNDARY IS STATED WHERE A CALLER READS IT: these answer from the faces THIS PROCESS has
registered** — MEASURED, a fresh process sees ZERO faces through `cg_font_enumerate` — which is the same boundary the
font service exists to remove.
- **AND TWO DECISIONS IN THE FAMILY DOOR ARE ASSERTED RATHER THAN DESCRIBED.** `CTFontCreateCopyWithSymbolicTraits`
computes the request as the font's own traits with the mask's bits replaced, so CLEARING bold is a question with an
answer; `CTFontCreateCopyWithFamily` compares under a **style mask** (bold/italic/condensed/expanded) and not over the
whole trait word, because **monospace and the stylistic class are properties OF a family** — asking DejaVu Sans for a
fixed-pitch face asks for something no face in it can be. The case asserts the mono bit does **not** cross, and that
dropping the mask makes the door answer NULL. FALSIFIED: with the mask removed, `FAIL: no face of "DejaVu Sans"
answered the family door`; with the caller's cascade list bypassed, `FAIL: the door answered from somewhere other than
the cascade list it was given` — `37 of 39` on both counts, `39 of 39` restored. MEASURED: `100% tests passed, 0 tests
  failed out of 2` over `text: 39 of 39 cases passed, 0 failed`, **152 `CT…` definitions**, `owed` **9**.
- **AND THE LAST THREE METRICS ARE DEFINED, WITH EVERY CURVE THEY NEED WRITTEN DOWN AS A DEVIATION.** `CTFontCopyTraits`
answers the four documented keys; `CTFontGetSlantAngle` and `CTFontGetVerticalTranslationsForGlyphs` are the two
metrics whose owed reasons were "the GLYPH-SPACE slant, not the italic angle" and "the vertical metrics the advances
door also lacks". **THE TRAITS ENTRY'S OWED REASON SAID "NO HEADER IN THE ERA PRINTS THE CURVE" — THE CURVE IS
UNPRINTED AND THE ANCHORS ARE**: `CTFontTraits.h` states, for each of the three floats, what 0.0 means and (for the
slant) that 1.0 is 30 degrees. So the three mappings honour every anchor Apple prints — the format's own
`usWeightClass` 400/`usWidthClass` 5 at 0.0, distances scaled by the widest the format has on that side — and each is
**recorded as a deviation with its landing point**. The case does TWO things: it recomputes the three floats from the
raw `OS/2` and `post` bytes with its own copy of the mapping, and it asserts the anchors on real faces (a regular face
is 0.0, an oblique face positive, a condensed one negative, a light one negative). The entries are also 64-bit integers
for the symbolic word, because the class mask sits in the top four bits.
- **AND THE SLANT IS TWO INGREDIENTS ADDED, WITH A UNIT APPLE NEVER STATES.** Apple's sentence names both — "the italic
or caret angle WITH ANY SKEW FROM THE TRANSFORMATION MATRIX APPLIED" — so the door reads `post.italicAngle` (turning
the sign over, since the format measures it counter-clockwise) and adds the angle the matrix gives the vertical,
`atan2(c, d)`. **DEGREES IS A CHOICE, RECORDED**, and the case MEASURES it against the other door that speaks of
slant: `kCTFontSlantTrait`'s 1.0-is-30-degrees must be exactly this door's answer divided by thirty, and a radians
mistake would show up as a factor of fifty-seven. The case also measures the transform's own convention with
`CGPointApplyAffineTransform` rather than trusting the sentence about `(c, d)`.
- **AND THE VERTICAL ORIGIN IS A COMPOSITION OF THREE DOORS THAT ALREADY EXISTED** — half the advance minus the left
side bearing, and the ascent — with the bearing taken from the ORIGIN OF THE BOUNDING BOX. **A SPACE IS THE CHECK THAT
NEEDS NO ARITHMETIC**: it has no outline and therefore no bearing, so its offset must be exactly half its advance.
The vertical metrics (`vhea`/`vmtx`/`VORG`) are **not read, and that is a recorded boundary — CORRECTED IN A LATER
COMMIT OF THE SAME SESSION, because the sentence first written here said "no face installed here carries them
(measured over every installed face)" AND THE MEASUREMENT HAD NEVER BEEN TAKEN**: the scan asked for `fvar`, `avar`,
`feat`, `morx` and `mort`, and not for `vhea`/`vmtx` at all. Asked properly, **two faces here carry both tables** —
`DroidSansFallbackFull.ttf` and `NotoColorEmoji.ttf` — so the condition that was written to move this door is
**already true**, and what remains is the work (read `vhea`'s ascent and line gap, `vmtx`'s per-glyph origin and
advance, `VORG` for a CFF face) rather than the environment. What the door answers is the format's fallback, which is
exact for every face here that carries no `vmtx` — every face the suite draws with and all but two installed.
Everything else is held against the doors it composes, and the array is asserted UNTOUCHED for a zero count, a
negative count, a NULL font, a NULL glyph array and a NULL output — which is the only way to see "nothing was
written".
- **AND ONE DETAIL IS RECORDED BECAUSE IT PRINTED ITSELF IN A CASE'S NOTE: a negative zero.** An upright face's slant
came out `-0.0` — equal to zero to every comparison, read as negative by `signbit` and printed as "-0" — so the door
normalises it, and the case's own recomputation does too. MEASURED: `100% tests passed, 0 tests failed out of 2` over
`text: 42 of 42 cases passed, 0 failed`, **155 `CT…` definitions**, and `owed` down to **6** — none of them waiting on
work this library can do: `feat` (no face here carries one), the two language/encoding mappings (Apple's own tables,
the user's decision is that they stay owed), the ATS pair (permanent), and the font service's transport (a system).