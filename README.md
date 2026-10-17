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
  THAT DRAWS.** `CTLineCreateWithAttributedString` makes **one font run per face the string names** — one, for a
  string set in a single face: the characters become glyphs through `CTFontGetGlyphsForCharacters` (the door this
  library was started for), each glyph is placed by `CTFontGetAdvancesForGlyphs`, and the faces' ascent, descent and
  leading are the line's heights. Every
  measuring door is arithmetic over those — glyph count, string range, typographic bounds, flush offset, trailing
  whitespace, image bounds — the caret pair inverts, and **`CTLineDraw` puts the ink on a page** through
  `CGContextShowGlyphsAtPositions`. MEASURED: `text: 17 of 17 cases passed, 0 failed`.
- **THE LINE'S BOUNDARY IS STATED RATHER THAN LEFT TO BE DISCOVERED FROM A WRONG LAYOUT.** Apple's own sentence
  for the creator is that "without a typesetter object, the line cannot be properly broken", so this is the simple
  case and it says what it is: **no line breaking and no shaping** — a character's glyph is drawn where its own
  advance puts it. (TWO FONTS IN ONE LINE and GLYPH REORDERING, refused here at the time, were both built later — see
  the bidi and multi-run bullets far below; SHAPING was built later too, and the bullet below says what it left
  owed.) **AND THE BASE
  WRITING DIRECTION IS READ NOW**, the first piece of the bidi milestone: the paragraph's direction reaches the run
  (`CTRunGetStatus` reports `kCTRunStatusRightToLeft`), while the Unicode Bidirectional Algorithm that would reorder
  the glyphs was built afterwards, in slices this file records.
- **AND A RUN DELEGATE IS THE ONE NON-GLYPH THE LINE HONORS** (`CTRunDelegate.h`, `kCTRunDelegateAttributeName`): a
  character carrying one is a BOX — its advance is the delegate's width, the line's ascent and descent grow to cover
  the delegate's, and `CTLineDraw` and `CTRunDraw` skip it, so an attachment or a tab occupies space without being a
  glyph. It is still ONE run, which is why it needs none of the machinery above. MEASURED (`case_run_delegate`): a
  box of width **30** in `ab<c>c` puts the character after it exactly **30** points past the box's own offset, not the
  placeholder glyph's advance; the line has one entry per character, box included; its ascent and descent grew to
  **12** and **3**; and `CTTypesetterSuggestLineBreak` at **40** points stops BEFORE the box, because the fit counted
  its width. WHAT IS STILL OWED: a box in a JUSTIFIED or TRUNCATED line keeps its place but not its delegate
  (`ct_line_with_positions` copies glyphs and positions, not the flags).
- **AND A TAB GOES TO ITS STOP, WHICH THE PARAGRAPH STYLE NAMES** (`CTTextTab`, the `kCTParagraphStyleSpecifierTabStops`
  and `…DefaultTabInterval` settings): a `\t` is a box whose width is the distance from the pen to the first stop past
  it, so `CTLine` and `CTTypesetter` compute it while they walk the positions — the same stops, so a line breaks where
  it will actually end. MEASURED (`case_text_tab_stops`): stops at **50** and **100** over `a\tb\tc` put the characters
  after the tabs exactly at **50** and **100**; and `CTTypesetterSuggestLineBreak` at **60** points reached the first
  tab (the word backoff then stopped there). WHAT IS OWED: a line spanning paragraphs with DIFFERENT tab vocabularies
  reads the stops at its own start; and `CTTextTab`'s options dictionary is carried and not read.
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
- **ONE ANSWER IS DERIVED AND ONE IS STILL THIS TIER'S, AND BOTH ARE ASSERTED SO A CHANGE IS DELIBERATE**:
  `CTRunGetStatus` REPORTS THE LINE'S BASE WRITING DIRECTION — `kCTRunStatusRightToLeft` for a right-to-left
  paragraph, `NoStatus` otherwise — and `CTRunGetTextMatrix` is the identity, because this library does no GLYPH
  REORDERING and sets no run matrix. The remaining status flags exist for the tier that reorders.
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
- **WHAT IS NOT ATTEMPTED IS SAID WHERE A CALLER READS IT**: no hyphenation, and the scripts UAX #14 sends to a
  dictionary. (UAX #14 ITSELF ARRIVED LATER — see the bullet at the end of this file.) AND THE TWO TYPESETTER
  OPTIONS ARE **OBEYED NOW**: `kCTTypesetterOptionDisableBidiProcessing` and
  `kCTTypesetterOptionForcedEmbeddingLevel` are read into a `CTLineBidiDirective` and handed to the LINE, where the
  algorithm runs — so the breaker still consults neither (bidi changes no advance) and both change the line the
  typesetter makes. MEASURED: a Hebrew line from a typesetter with bidi disabled reports NO `NonMonotonic`, keeps its
  glyphs in string order and does not mirror, `ForcedEmbeddingLevel` 0 turns that same line left-to-right, a forced 1
  turns a LATIN line right-to-left, and a value of the wrong type or a level that is not 0 or 1 is IGNORED rather than
  refused. (The base writing direction is read by the LINE, and its run reports it.)
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
  **AND ONE REFUSAL IS HELD**: a token drawn in **another face** answers NULL, because truncation appends no run of its
  own and the token's glyph indices mean nothing in the line's faces — Apple's answer is a line the framesetter builds
  with runs of its own, and this tier refuses rather than putting garbage where an ellipsis belongs. The result needed one thing the library did not have — **a line over GLYPHS
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
- **AND THE BIDI MILESTONE HAS ITS FIRST TWO PIECES: THE CLASS DATA AND THE PARAGRAPH LEVEL.** `CTBidiData.c` holds
  the Bidi_Class of every code point as a table of **770** ranges, GENERATED by `tools/gen-bidi-data.py` from Python's
  `unicodedata` (Unicode **15.1.0**) so the data's provenance is a RECIPE rather than a hand-copy; `ct_bidi_class`
  answers it, and anything the table does not name is L. **AND P2/P3 ARE BUILT ON IT** (`ct_bidi_paragraph_level`): the
  FIRST STRONG character — L is level 0, R/AL is level 1 — while STEPPING OVER an isolate run, with the caller's
  fallback when there is none. The line uses it to RESOLVE a natural direction, so a paragraph of Hebrew is level 1
  without the caller stating anything, and the run it makes REPORTS it through `CTRunGetStatus`. **AND THE EXPLICIT
  PHASE IS BUILT TOO** (`ct_bidi_explicit_levels`, X1-X8): a STACK of directional statuses carries embeddings,
  overrides and ISOLATES — an RLE raises the level to 1 and its PDF brings it back, an RLO OVERRIDES the class, a
  right-to-left paragraph makes an LRE level 2, an LRI/RLI isolates what follows with the PDI bringing the level back,
  and an FSI asks P2/P3 OF ITS OWN TEXT to decide which of the two it is. X9 marks the explicit formatting characters
  BN — removed for the rules that follow. **AND THE WEAK TYPES ARE BUILT ON TOP** (`ct_bidi_resolve_weak`, W1-W7, over
  the LEVEL RUNS with each run's own sos/eos): a combining mark takes the type it follows, a European number after an
  Arabic letter becomes an ARABIC number, an Arabic letter becomes R, a separator between two numbers of a kind
  becomes one, a European terminator adjacent to a number becomes one, what is left of them is OTHER NEUTRAL, and a
  European number after a Latin strong type becomes L. **AND THE NEUTRALS ARE BUILT ON THOSE**
  (`ct_bidi_resolve_neutral`, N1/N2): a run of neutrals between two strong types of the SAME direction takes that
  direction — a NUMBER counting as R — and anything else takes the EMBEDDING direction of its run. **AND X10 BUILT THE
  JOIN THEY NEED**: `fn_sequences` chains each level run whose LAST character is an isolate initiator to the run its
  MATCHING PDI STARTS, so the text on BOTH SIDES of an isolate is ONE isolating run sequence — and the W and N rules
  now run over SEQUENCES rather than raw runs, which is BEHAVIOUR-PRESERVING where there are no isolates (every case
  before this one is unchanged). **AND THE IMPLICIT LEVELS CLOSE THE LEVEL PHASE** (`ct_bidi_resolve_implicit`, I1/I2):
  a character whose direction DISAGREES with its embedding level is raised — R by one and a number by two at an even
  level, and L and the numbers by one at an odd level — which is what puts a Hebrew letter in a Latin paragraph at
  level 1, the ODD level L2 reorders on. **AND N0 CLOSES THE LEVEL PHASE** (`ct_bidi_resolve_brackets`): the bracket
  pairs come from Unicode's `BidiBrackets.txt`, GENERATED by `tools/gen-bidi-brackets.py` into **128** rows of
  open/paired/kind, and BD16's stack matches an opener with its closer. A matched pair takes ONE direction — the
  embedding direction when the inside agrees with it (N0.b), the OPPOSITE when the inside and the context before BOTH
  disagree (N0.c), and NOTHING when the inside is not strong, which leaves the brackets to N1/N2 (N0.d). **AND THE
  REORDERING IS BUILT** (`ct_bidi_reset_levels` for L1, `ct_bidi_reorder` for L2): L1 takes the separators, the
  whitespace/isolates BEFORE them and the whitespace/isolates at the END of the line to the paragraph's level, and L2
  turns the LEVELS into a VISUAL ORDER by reversing, from the highest level down to the lowest ODD one, every maximal
  run at that level or higher. **AND THE LINE IS WIRED** (`CTLine.c`): it runs the WHOLE pipeline over its own
  characters and, when the visual order is not the logical one, PLACES each glyph where the reordering says — the
  advances are KEPT (a reordered glyph's advance is not recoverable from its neighbours' positions), kerning is taken
  between VISUAL neighbours, the DRAW follows the positions, the HIT TEST walks the visual slots, and the run reports
  `kCTRunStatusNonMonotonic`. MEASURED (`tests/text.c`): a Hebrew line's FIRST character's glyph sits at x=**15.65**
  while the LAST is at **0**, the width unchanged at **23.48**. **AND L4 MIRRORS THE PAIRED CHARACTERS**
  (`ct_bidi_mirror`, from `BidiMirroring.txt` GENERATED by `tools/gen-bidi-mirroring.py` into **428** rows): a
  character whose resolved level is ODD is depicted by its MIRROR, so `(` shows as `)` in Arabic text — and the
  mirroring runs BEFORE the glyphs are looked up, so the engine never sees the unmirrored character. MEASURED: a Hebrew
  line whose second character is `(` looks up glyph **12**, which IS the font's `)` glyph. **AND THE LEADING EDGE IS THE
  CHARACTER'S OWN SIDE** (`CTLine -getOffsetForStringIndex:`): an EVEN level's leading edge is its left one and an ODD
  level's is its RIGHT one, so a Hebrew line's carets run RIGHT to LEFT — MEASURED, `23.48 > 15.65 > 7.83 > 0` with the
  width `23.48` — and one past the last character is its TRAILING edge. THE HIT TEST FOLLOWS: the half of a glyph the
  point fell in decides which side of the character the caret is on, by that character's OWN direction. **AND WHAT IS
  STILL NAMED**: the SHAPING a line whose glyphs are not one per character wants. MEASURED: `text: 57 of 57 cases passed, 0 failed`.
- **AND THE MULTI-RUN MODEL THE REORDERING ASKED FOR IS BUILT: A LINE IS AS MANY RUNS AS ITS STRING HAS FACES.**
  `struct __CTLine` carries a FONT RUN per maximal span of one face, and `fn_font_runs` finds them by asking for the
  LONGEST range over which `kCTFontAttributeName` is the SAME VALUE — **not** the effective range of the attribute RUN
  the character sits in, which is a different question: a string that sets a run delegate or a tab in the middle would
  otherwise split into runs that share ONE face, and the delegate case caught exactly that. Each run's glyphs are looked
  up in ITS OWN font, the advances come from that font too, and a kerning pair is only asked about when **both** glyphs
  are in the same run, because two faces have no shared typographer. The line's ascent, descent and leading are the
  runs' UNION rather than the first font's. `CTLineGetGlyphRuns` now returns one `CTRun` per font run — each with its own
  font, its own slice of the string, and its glyphs INDEXED IN THAT FACE — and `CTLineDraw` sets the face (and its size)
  on the context once per run, because a glyph index means nothing without the face it came from. MEASURED: a
  four-character line set in `DejaVuSansMono-Bold` and then `DejaVuSerif` reports **2** runs, each run's attributes name
  its OWN face, and the mixed line's width is **52.9688** — the two faces measured apart, **24.082** and **28.8867**,
  summed. MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 58 of 58 cases passed, 0 failed`.- **AND THE SHAPING ENGINE IS BUILT, WHICH IS THE ONE THING THIS LIBRARY TAKES FROM OUTSIDE ITSELF.**
  `CTShaping.c` shapes a run over **HarfBuzz** (`libharfbuzz-dev` **10.2.0**, a REQUIRED `pkg-config` dependency),
  and the decision is recorded at length in `CTShaping_internal.h`: shaping is OpenType's `GSUB`/`GPOS` plus a
  per-script rule set that runs to thousands of lines of published behaviour for Devanagari alone, so a hand-written
  shaper would be wrong for most of the world's scripts and **silently** wrong for many — the failure this tree
  refuses. **AND THE FONT DOES NOT CROSS AS A FILE**: `hb_face_create_for_tables` is given a callback, and the
  callback is `CTFontCopyTable`, so `cmap`, `GSUB`, `GPOS`, `GDEF`, `glyf`/`loca` and `hmtx` all come through the one
  reader of a font's tables this library already had and HarfBuzz is a consumer of it rather than a second
  implementation of it. The scale is the face's own UNITS PER EM, so every number HarfBuzz answers is an exact
  integer of font units and the conversion to points happens once, in this file.
  MEASURED, on five faces and every number the engine's own header promises: `ffi` in `DejaVuSans` is **1 glyph**
  (cluster 0) and in `DejaVuSerif` **2** (an `ff` ligature and the `i`), while in `LiberationSerif` it is **3** — same
  three characters, three answers, which is why the case carries a face WITHOUT the ligature as its control; `To` in
  `DejaVuSerif` gives the `T` **7.66162** where its own advance is **8.6709** — a pair adjustment of **-1.00928**,
  which is what the face's `kern` table says too — so shaping reaches the advance; `a`+U+0301 is **1 glyph** in
  `DejaVuSerif`, equal to the glyph the face already has for U+00E1, and **2
  glyphs** in `FreeSerif` whose clusters are BOTH **0** and whose second glyph carries an offset of **-0.689** — two
  faces, two different and equally correct answers; Devanagari `ka`+VIRAMA+`ssa` is **1 glyph** from **3** characters;
  and an Arabic word's three letters come back in **VISUAL order** (clusters **2, 1, 0**) with the first letter's
  form **8249** against **1471** alone, which is joining.
  **AND IT IS NOT WIRED INTO `CTLine`, AND THAT IS SAID RATHER THAN LEFT TO BE FOUND.** The line's arrays, its caret
  doors and the AppKit's glyph ranges all assume one glyph per character, so connecting the engine to the line is one
  change with the shaper's clusters as the new source of truth — and **nothing in the layout path calls it**, so
  nothing that existed before this commit has been made silently wrong. What is not attempted by the ENGINE is named
  in its own header too: itemization, font fallback, and reconciling HarfBuzz's visual order for a right-to-left run
  with the line's own L2. MEASURED: `100% tests passed, 0 tests failed out of 2` over
  `text: 64 of 64 cases passed, 0 failed`.
- **AND SHAPING IS WIRED IN: THE LINE'S GLYPHS ARE THE SHAPER'S NOW, AND THE CLUSTER MAP IS WHAT TIES THEM BACK.**
  `ct_line_create_with_font` shapes each run instead of asking the font for one glyph per character, so a line's
  glyph count is the SHAPER's answer and not the text's. Three things had to be true together, and each is a
  measurement rather than a hope:
  **(1) SHAPING IS PER BIDI LEVEL RUN, NOT PER FONT RUN** — HarfBuzz's direction is the run's and a level run inside a
  right-to-left paragraph is left-to-right — so a font run is cut at every level change and each piece is shaped with
  its own direction. **(2) THE VISUAL ORDER IS BUILT OVER GLYPHS FROM THE ORDER OVER CHARACTERS**, by walking the
  character-level order and emitting each glyph ONCE at the first character it covers: that leaves the entire bidi
  pipeline untouched, and it is correct for a ligature (emitted at its first character), for a right-to-left run
  (already ordered by the shaper, and kept so) and for plain left-to-right text (left exactly as it was).
  **(3) THE KERN LOOP IS GONE, AND ITS WORK IS THE SHAPER'S** — the pair adjustment is already inside the advance a
  shaped glyph carries, so applying it again would apply it twice. MEASURED, and it is why deleting that loop is safe:
  the shaper's answer for `To` in this suite's face is the `kern` table's own **-1.00928** points (8.6709 alone,
  7.66162 shaped).
  MEASURED end to end: `ffi` is **1 glyph** in `DejaVuSans`, **2** in `DejaVuSerif` and **3** in `LiberationSerif`;
  Devanagari `ka`+VIRAMA+`ssa` is **1 glyph** from **3** characters; an Arabic word comes back in the shaper's visual
  order with its letters in their joined forms; and a line of plain Latin is glyph-for-glyph what it always was.
- **AND ONE FEATURE HAD TO BE TURNED OFF, WHICH THE WIRING IS WHAT FOUND.** HarfBuzz implements UAX #9's **L4** itself
  — it mirrors a character whose script runs right to left before looking the glyph up — and L4 is ALREADY THIS
  LIBRARY'S (`ct_bidi_mirror` runs over the line's characters so the font is asked for the character that will be
  drawn). With both, a `)` in Hebrew text was mirrored to `(` by L4 and mirrored BACK by HarfBuzz, and the line drew
  the wrong parenthesis: MEASURED, the mirrored bracket came back as glyph **11** (the `(`) where the face's `)` is
  **12**. **AND THE MECHANISM IS NOT THE `rtlm` FEATURE, WHICH IS WHAT IT LOOKS LIKE** — disabling `rtlm` through
  `hb_shape`'s feature list does NOT stop it (MEASURED, `rtlm=0` still answers 11 for `)` in a right-to-left run),
  because the mirroring is not a GSUB feature at all: it is a UNICODE FUNCTION. The only way to say "this text is
  already mirrored" is to hand HarfBuzz a Unicode-functions object whose mirroring answers the character unchanged,
  and `CTShaping.c` builds one per shaper for exactly that.
- **AND `kCTRunStatusNonMonotonic` IS NOW APPLE'S OWN FACT RATHER THAN THE PIPELINE'S.** Apple's flag says the run's
  STRING INDICES do not ascend; the pipeline moving the characters is one reason that happens and **the shaper is a
  second**, because a right-to-left run comes back in visual order whether or not the pipeline ran. So both are asked,
  and two case expectations changed with it rather than being adjusted to fit: the L4 case now asks the RUN which
  glyph stands for the bracket (a glyph is no longer a character, so `glyphs[2]` was the shaper's order and not the
  bracket's), and the typesetter-options case records that with bidi processing OFF the flag is still SET, because the
  shaper's order descends with nothing in the pipeline having moved.
  MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 65 of 65 cases passed, 0 failed`, with SIXTY-FOUR
  of those cases passing byte-identical to the commit before the wiring — the sixty-fifth being the cluster map's own.
  **AND WHAT IS OWED IS NOW THE APPKIT'S**: the manager's glyph space is one glyph per character BY CONSTRUCTION
  (`glyphBase` advances by the range's length), so the shaped glyph count is not yet visible through it. That is
  stated in `NSLayoutManager.m` where the two mapping doors are, with what changing it takes.
- **AND UAX #14 IS BUILT, WHICH IS THE ALGORITHM THE TYPESETTER HAS BEEN MISSING.** `CTLineBreak.c` implements the
  Unicode Line Breaking Algorithm — the `Line_Break` property of every code point and all thirty-one numbered rules —
  over a generated table (`CTLineBreakData.c`, `tools/gen-linebreak-data.py`, Unicode 15.1.0, **2,845 ranges** with
  the file's OWN defaults for unlisted code points baked in). **THE RULES ARE A LIST AND THE ENGINE IS A TRANSCRIPTION
  OF IT**: 31 rules, in the specification's order, with its numbers, because that is what makes the file checkable
  against the specification rather than merely plausible.
- **AND ITS MEASUREMENT IS A CONFORMANCE RUN RATHER THAN A SET OF EXAMPLES, WHICH IS THE WHOLE POINT.** The UCD
  ships `LineBreakTest.txt` — every rule's own examples with Unicode's answer for every position — and the engine was
  written against it: **10,274 of 10,274, NO FAILURES.** The file is 1.5 MB and is NOT in this repository (the same
  standing the UCD's other files have): what is committed is `tools/gen-linebreak-cases.py`, which fetches it and
  emits **one case per rule number** — the shortest case naming it, 69 rows — so the suite checks every rule the data
  exercises without the file.
- **AND EVERY HARD PART OF THE RULES WAS FOUND BY THAT RUN AND BY NOTHING ELSE.** Stated because each is a fact about
  the specification that no amount of reading the obvious parts would have produced:
  **(1)** LB9's fold — giving a combining mark its base's class — loses the BOUNDARY, so a base and its own mark looked
  like two adjacent characters that happen to share a class; 1,547 cases failed on it, all tagged `[0.3]`.
  **(2)** Unicode 15.1 **REPLACED LB15** (`QU SP* × OP`) **WITH LB15a AND LB15b**, which split a `QU` by whether
  `General_Category` is `Pi` or `Pf` — a left quote and a right quote are the SAME `Line_Break` class and opposite
  typography — which is 174 cases and a property the class cannot express.
  **(3)** §8.2's Example 7 tailors the number rules to a REGULAR EXPRESSION **and requires LB13 to be tailored beside
  it** ("if this is not done, single digits might be handled by rule LB13 before being handled in the regular
  expression"), and the tailored LB25 differs from the rewritten pair rules on cases like `,0`, which the pair rules
  make unbreakable and the tailoring breaks.
  **(4)** LB28a has **FOUR** clauses, not three, and its third is `VI` where I had written `VF` — checked against
  `tr14-51` rather than guessed, after six cases named rule numbers (`28.12`, `28.13`, `28.14`) that no clause of mine
  could reach.
  **(5)** And the last two cases of ten thousand turned on the same lesson as (1) one level up: several rules read the
  BASE of a syllable, not the character next to the position, so a look-behind has to step over a base's own marks.
  MEASURED: `100% tests passed, 0 tests failed out of 2` over `text: 66 of 66 cases passed, 0 failed`.
  **AND THE TYPESETTER STILL DOES NOT CALL IT** — that is the next slice, and until it lands no line is laid out
  differently than it was. `CTTypesetter.h` says so where a caller reads it.
- **AND THE TYPESETTER CALLS UAX #14 NOW, WHICH IS THE ALGORITHM ITS BREAKING WAS STANDING IN FOR.** Word wrapping
  backs off to the last opportunity the rules allow instead of the last SPACE — and a space is ONE of thirty-one
  rules. **MEASURED, and one character apart from the answer that was there before**: `abc-defghij` at a width that
  fits five characters broke after **4** (after the hyphen) where the space search returned **5**, a cut INSIDE a
  word. `abc,defghij` at the same width broke after **5** — and that one was a correction rather than a confirmation:
  I expected 4, and the engine was right, because **LB29 (`IS × (AL | HL)`, the rule whose own comment is "e.g.")**
  forbids a break after a comma before a letter.
- **AND THE CLUSTER DOOR DELIBERATELY DOES NOT, WHICH THE CASE MEASURES RATHER THAN ASSERTS.** Apple's sentence for
  it is "no other contextual analysis will be done", and UAX #14 is exactly that analysis — so the two doors differ,
  and the case asserts the DIFFERENCE as well as the answer, because a door rewired by accident would pass one check
  and fail the other. MEASURED on two Hangul syllables (U+1100 U+1161 twice), which LB26 will not let a break fall
  inside: at a width fitting three of the four characters the line door breaks after **2**, BETWEEN them, and the
  cluster door cuts at **3**, inside the second.
- **AND TWO THINGS THIS DOES *NOT* BUY ARE SAID WHERE A READER WOULD ASSUME THEM.** Han was already right by
  accident — the old space search fell back to the overflow, and ideographs may break between every pair, so no width
  distinguishes the two implementations. And **Thai, Lao and Khmer do not wrap at all**, because they are `SA`
  ("require morphological analysis to determine break opportunities"), LB1 resolves `SA` to `AL`, and LB28 forbids a
  break inside an alphabetic run: they need a DICTIONARY, which ICU's dictionary-based break iterator and the
  specification's own `SA` tailoring exist for. "UAX #14 is implemented" invites the opposite assumption, so the
  engine's header, the typesetter's header and the typesetter's source each say it in words; the suite asserts the
  Hangul rule instead, because a case that asserted a difference for Thai would be asserting the fallback.
- **AND HYPHENATION REACHES THE ENGINE, BEHIND AN EXPLICIT OPTION.** `kCTTypesetterOptionHyphenationFactor` is **this
  tier's OWN key** — Apple declares two typesetter options and this is not one of them — and the reasoning is at the
  declaration: the SETTING is AppKit's (`NSParagraphStyle -hyphenationFactor`; `CTParagraphStyle` has no specifier for
  it in any version), the RESOURCE is Core Foundation's (`CFStringGetHyphenationLocationBeforeIndex`, which the layout
  manager already asks), and a typesetter that can hyphenate needs a channel Core Text does not provide — the options
  dictionary is the surface built for exactly that.
- **AND THE ALTERNATIVE WAS REFUSED RATHER THAN OVERLOOKED: `SuggestLineBreak` MUST NOT ALWAYS HYPHENATE.** Its answer
  would depend on the MACHINE — a `hyph_en_US.dic` present here and absent there breaks the same string at a different
  character — and it would change the meaning of a door Apple documents, for callers that asked for nothing. A door
  whose answer changes with the machine is the one thing a layout engine must not have, so the factor is the opt-in
  and zero is the default. MEASURED, each case a PAIR because the claim is the difference: `hyphenation` at a
  four-character width breaks at the overflow **4** with no factor and at the hyphenation point **2** with one; `the
  hyphenation` at an eight-character width breaks after the space at **4** with no factor and after **6** with one,
  because the space leaves the line only half full and the factor's zone says hyphenate; and the CLUSTER door ignores
  the option entirely, answering the fit — **4** and **8** — since it reads no paragraph style at all.
- **AND TWO BUGS OF MY OWN WERE FOUND BY THE SUITE RATHER THAN BY READING, BOTH WORTH RECORDING.** The factor was
  first READ INSIDE THE PARAGRAPH-STYLE BRANCH, so a string with no style silently got no hyphenation and the new case
  measured nothing; and the read was UNGUARDED, so `CFDictionaryGetValue(NULL, …)` **SEGFAULTED two of this suite's
  own cases** the moment it ran. A caller passing no options is the ordinary caller, and it took a measurement to
  notice.
- **AND THE APPKIT STILL HYPHENATES ITSELF, WHICH IS THE LAYERING ANSWER RATHER THAN A DUPLICATION LEFT BEHIND.**
  Passing this option from `NSLayoutManager` is the obvious tidy-up and it is WRONG: **THE FACTOR IS A PARAGRAPH'S
  PROPERTY AND A TYPESETTER COVERS A WHOLE STRING.** The manager makes ONE typesetter for the store and reads
  `hyphenationFactor` from each paragraph's own style, so one option value cannot carry two paragraphs' factors, and a
  typesetter per paragraph would re-typeset the whole string per paragraph — against this type's own contract that
  "the whole string is typeset once". MEASURED, in the AppKit's suite: one store, two paragraphs, factors 0.0 and 1.0,
  and each is laid out by its OWN (the first breaks at its space, the second reaches into its word). A single factor
  could only have got one of them right.
- **AND THAT CASE FOUND A BUG OF LONG STANDING IN THIS FILE, WHICH IS THE PART WORTH RECORDING.** `ct_ts_hard_break`
  scanned forward for the first newline and `SuggestLineBreak` returned it WHATEVER THE WIDTH WAS — so A PARAGRAPH
  LONGER THAN ITS CONTAINER THAT ENDED IN A NEWLINE CAME BACK AS ONE LINE THAT OVERFLOWS. Apple's sentence is "either
  by a hard break character in the stream or by FILLING THE SPECIFIED WIDTH", and the fix is to COMPARE the two
  triggers instead of ordering them: the hard break wins only when it is REACHED (`start + hard <= fit`). MEASURED,
  the case's first paragraph: **8 fragments where it had 1**, `(0,24)` becoming `(0,3) (3,9) (12,9) (21,3)`. Every
  case in this suite that involved a newline used a line short enough to fit, which is exactly why it survived — and
  the engine's own suite now holds both sides of the rule: a newline INSIDE the width still ends the line, newline
  included, and one BEYOND it does not.
- **AND THE SHAPER ITEMIZES BY SCRIPT NOW, WHICH IS A CORRECTION RATHER THAN A FEATURE.** Its answer to "no script was
  stated" was HarfBuzz's: read ONE script off the FIRST STRONG CHARACTER of the buffer. That is right for a run that
  already is one script and wrong for every other — and **A WRONG SCRIPT STILL PRODUCES GLYPHS, SO THE DAMAGE WAS
  SILENT**: a Devanagari syllable inside a Latin sentence was shaped with `latn`, which forms no conjunct. Now the run
  is cut where the script changes (`fn_itemize`, over `hb_unicode_script`, so the tree still links ONE shaping
  library) and each piece is shaped with its own script; `Common`, `Inherited` and `Unknown` are not scripts for this
  purpose and take their neighbour's, by ICU's rule — the preceding one, and the first following one for a run that
  opens with them. **AN EXPLICIT TAG IS STILL A STATEMENT AND IS OBEYED**, which is what makes the case a measurement:
  MEASURED, `a` + `KA`+VIRAMA+`SSA` is **2 glyphs** itemized (the letter and ONE conjunct) and **4** when the run is
  stated as `Latn` — the control that shows the itemizer did the work rather than HarfBuzz getting it right anyway.
  **AND A LINE GETS IT WITHOUT ASKING** (`CTLine.c` states no script, only font and level cuts): a line over the same
  four characters is **2 glyphs**, with the run reporting the caller's OWN string indices, so nothing above can tell
  the run was cut at all.
- **AND THE OTHER HALF — ITEMIZATION BY FACE — HAS ITS DOOR ALREADY, WHICH IS WORTH SAYING BECAUSE IT LOOKS MISSING.**
  `CTFontCreateForString` answers "a font that renders this string" over the cascade list, and it is implemented and
  measured (`CTFont.c`, ten cases in this suite, including a `kCTFontCascadeListAttribute` the caller supplies). What
  is owed is the LINE CALLING IT — and that is a cut of its own rather than a call added: a line's runs are the
  ATTRIBUTED FONT's today, and a fallback face is not in the attributes, so a substituted run would report an
  attribute that does not name the face it was drawn in. That is the next slice, and the shaper's own header says so
  where a reader looks.
