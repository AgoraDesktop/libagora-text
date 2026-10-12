# Provenance

`libagora-text` is Core Text for Agora.

**THE REPOSITORY WAS STARTED OVER ON 2026-10-11.** Its first content was the whole CoreText tree from Fiwix
(`../../Fiwix/userland/CoreText`, revision `6f5d90d97763e042c3653cce376e4c1b00ef4e93`), imported verbatim as 41
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
tooling measures. `../../Fiwix/tools/coretext-sweep.py` says it outright:

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
| `../../Fiwix/docs/reference/coretext-apple-surface.txt` | Apple's documented surface as **2032 rows** with a type and a status each, generated from Apple's documentation index. **THE ROW LIST THIS LIBRARY WORKS FROM.** Its per-kind rows sum to 1038 shipped, 534 open, 460 struck — the same numbers read at the restart, which is the point of saying so — and 515 of the open rows are the AAT/SFNT structure vocabulary |
| `../../Fiwix/tools/coretext-sweep.py --check` | verifies that ledger **offline** ("the only mode that touches the network" is `--refresh`). IT IS **NOT** THE PROGRESS METRIC FOR THIS WORK, AND THAT CLAIM IS CORRECTED HERE: it computes its verdict over `HEADERS = <Fiwix>/userland/CoreText/*.h` — the Objective-C import THIS repository deleted, and which that tree still holds — so every row for a type this library has not written yet still reads `shipped` there |
| `../../Fiwix/docs/reference/coretext-sdk106.txt` | a presence oracle for the era: 1836 names, each with the header it was found in |
| `../../Fiwix/docs/design/coretext-plan.md` | the plan this work is a milestone of |

**AND WHAT MEASURES THIS LIBRARY IS ITS OWN SUITE** — `ctest --test-dir build`, plus `nm -D` on its own
library — because both live in this repository and neither can be satisfied by another tree's declarations.
**EVERY PATH ABOVE WAS ALSO WRITTEN `../Fiwix/…` AND RESOLVED TO NOTHING**, since the Fiwix tree is a sibling of
`Agora` rather than of this repository: the commands could not have run at all.

## What Apple's headers settled that no local file could

- **The dependency set is two, not three** — and the leading one of the sixteen headers states it: no
  `#import`, no `@interface`, no `@class` anywhere in them.
- **`ATSFontRef` IS A `UInt32`**, so `CTFont.h`'s include of `ATS/ATSTypes.h` needs no ATS header: one typedef
  of an integer, and the doors that take it are owed doors anyway.
- **The include graph**, which fixes the order the headers have to be written in (see `README.md`).
- **The character→glyph step is Core Text's own**, and it is the one thing a signature cannot answer: Apple
  deprecated Core Graphics' glyph doors, this tree struck them, and reaching a font's `cmap` is the first
  architectural question of the restart.

## What a type identity is worth, measured

**A TYPE ID THAT COMES FROM A CONSTANT RATHER THAN FROM THE RUNTIME MAKES CORE FOUNDATION HALT AT CREATION.**
The deleted library answered all eleven `CT…GetTypeID` doors from a table of constants kept in a file beside
the objects, and this library's first object takes its identity from `_CFRuntimeRegisterClass` instead. The
difference was measured by forcing the old arrangement back into `CTFontDescriptorGetTypeID`:

    FAIL: The descriptor is an object, and its identity is the runtime's: killed by signal 4
    text: 3 of 4 cases passed, 1 failed

SIGILL, AND NOTHING PRINTED BY THE CHILD AT ALL — `_CFRuntimeCreateInstance` halts before a single assertion
runs, because Core Foundation will not build an object whose type id it does not recognise. The failure is
therefore earlier and louder than a wrong answer: an identity the runtime does not own cannot be given to an
object in the first place. Restored, the suite is `text: 4 of 4 cases passed, 0 failed`.

AND ONE THING ABOUT THAT RECORD IS WORTH KEEPING TOO: the commit that first reported this falsification
(`0150d16`) printed a tidy `FAIL: CFGetTypeID(descriptor) is 256 and CTFontDescriptorGetTypeID() is 1` line
that NEVER APPEARED. It was written from what the failure was expected to look like rather than read off the
run, which is the exact error this tree keeps having to correct — a document, or in this case a commit
message, describing a state that was assumed rather than measured. The corrective commit is `87ba9f6`, and this paragraph is why it exists.

## How a claim about a command is recorded here

**A COMMAND'S RESULT GOES INTO A COMMIT MESSAGE ONLY AFTER THE COMMAND RAN AND ITS OUTPUT WAS CAPTURED.** Three
times in this library's first five commits a message described something other than what the run printed:

- `0150d16` INVENTED a `FAIL` line that never appeared — the run's whole transcript was `killed by signal 4`.
  Corrected in `87ba9f6`.
- `e6b31a0` OMITTED two of four `FAIL` lines, describing a weaker test than the one that ran. Corrected in
  `9104b24`, which is in the sibling `libagora-drawing` repository, where the run happened.
- `0099210` CLAIMED THAT `a .c file including this header compiles cleanly`, AND THE PROBE HAD FAILED. The
  probe was run with `-std=c99`, where Core Foundation's own headers cannot see `lstat` and `syscall`, so the
  compiler stopped inside Core Foundation before ever reaching the claim's subject. The claim was written into
  the message before the probe ran, and the shell's `;` let the commit proceed past the failure.

The three faults are one fault: **writing the run from what it was expected to do.** The remedy is a habit, not
a sentence: CAPTURE THE OUTPUT TO A FILE, READ IT, AND QUOTE THE FILE. `-std=c99` was also simply the wrong
flag — the library builds under clang's default standard, and a probe that does not use the library's own flags
is measuring a different program.

## Agora changes

**The restart itself is this section's first entry**: the import deleted, the build rewritten for a C-only
library with two dependencies, and the package config's `find_dependency(Foundation)` removed.

**`CTFontCreatePathForGlyph` — the glyph as geometry (2026-10-11).** The door the restart's own owed list said
"lands next, with the `CGPath` it builds" is defined, over the seam `cg_font_glyph_outline` that Core
Graphics already carries: the outline is decomposed into moves, lines, quadratics and cubics and hung on a
mutable `CGPath`. What was settled by Apple's header rather than by this repository is the composition order,
and it is written into both `CTFont.h` and `CTFont.c` as the quotation it is: *"The path will reflect the
font point size, matrix, and transform parameter, in that order."* The middle ingredient — **the font's own
matrix** — is what the seam's `matrix` parameter receives, concatenated with the caller's `transform`, and
`cg_font_glyph_outline` composes the size in front of both, so the three land in Apple's order once. The
`transform` parameter is `CGAffineTransformIdentity` when NULL, which the header states; the path is
**RETAINED**, which its result line states; and NULL is kept for the case that line names, "an error occurred"
— a glyph the engine will not load. A glyph with no outline is an EMPTY PATH, not NULL, because a space is not
an error.

MEASURED: `cmake --build build` exits 0, `nm -D build/libagora-text.so` lists `CTFontCreatePathForGlyph`, and
`text: 6 of 6 cases passed, 0 failed`. AND FALSIFIED, because a case that cannot fail is worth nothing: with
the font's matrix forced out of the composition the new case fails on exactly that claim —

    FAIL: the matrixed font's outline starts at x 0.193359 and the plain font's at 0.193359, want a shift of 30 -- the font's own matrix is NOT in the path
    text: 5 of 6 cases passed, 1 failed

— and restored it is `6 of 6` again.

This section is changed IN THE SAME COMMIT as the change it describes, and `README.md`'s status with it. The
previous repository got that wrong — its `PROVENANCE.md` said "not yet made" long after the work was finished
and its `README.md` went on claiming the NS types were unconverted — and the only defence is the rule.

**The build recipe, and the suite's runtime path (2026-10-11).** `tests/CMakeLists.txt` no longer assumes the
caller's shell carries an `LD_LIBRARY_PATH`: the test's search path is derived from the imported
`CoreFoundation::CoreFoundation` and `AgoraDrawing::Drawing` targets, so a **bare** `ctest` works on a clean
build and cannot disagree with the prefix the library was actually built against. Two things made that
necessary rather than tidy. The loader is not consulted for a dependency of a dependency — `libagora-core.so`
NEEDs `libobjc.so.4.6` and records no runtime path of its own, so `DT_RUNPATH` on the test executable could
never have covered it — and the prefix the build trees happened to point at was `/tmp/agora-prefix`, which a
`/tmp` cleanup had already deleted once. **THE PREFIX IS NOW `../build-prefix`, WHICH IS NOT A NEW DECISION:
it is the one `libagora-core`'s README has documented all along (`--prefix "$PWD/../build-prefix"`), and the
`/tmp` path was a casualty of where the trees were configured rather than a convention.** The same commit adds
the `## Build` section this `README.md` never had, and corrects the one false comment in it: the suite's "Core
Graphics is the library's own dependency and not the suite's" had been untrue since the font case began
loading a face from a file.

MEASURED, and this is the recipe the `## Build` section prints: configure with
`-DCMAKE_PREFIX_PATH=<Agora>/build-prefix -DCMAKE_C_COMPILER=/usr/bin/clang-19` from a **deleted** `build/`,
`cmake --build build` exits 0 with no warnings and no errors in its log, and `ctest --test-dir build` with no
`LD_LIBRARY_PATH` in the environment is `100% tests passed, 0 tests failed out of 1` over
`text: 6 of 6 cases passed, 0 failed`.

**`CTFontCollection` — the first object here that is a QUESTION rather than a value (2026-10-11).** The type
after `CTFont` in Apple's own include order is declared and implemented. `CTFontCollection.h` carries Apple's
whole surface for it — the opaque `CTFontCollectionRef`, `GetTypeID`, `CTFontCollectionSortDescriptorsCallback`,
the one option key and Apple's five further doors — and `CTFontCollection.c` defines the object and the four
creators. WHAT A COLLECTION HOLDS IS ITS QUESTION: the descriptors it was asked about, the options it was handed,
and which of Apple's two questions it is, and its `equal`/`hash` are over exactly those. The copy door therefore
keeps the original's question and APPENDS the new descriptors rather than guessing what "merged with the existing
descriptors" means against an available-fonts collection, which holds none to merge with; nothing the caller
passed is dropped and nothing is invented.

**ITS TWO MATCHING DOORS ARE OWED, WITH THE BOUNDARY STATED RATHER THAN THE MACHINERY BLAMED.** Apple's result is
"an array of NORMALIZED font descriptors matching the collection definition": the SET is reachable —
`cg_font_enumerate` walks the configured directories and the process's registrations — and the NORMALIZATION and
the ORDER are Apple's, so this tier answers neither. **AND "DECLARED AND NOT DEFINED" IS NOW HELD BY MEASUREMENT
RATHER THAN BY THE COMMENT THAT SAYS SO**: `tests/owed_symbols.cmake` reads `nm -D --defined-only`, and the
second test, `owed`, fails if either door acquires a definition. FALSIFIED by pointing it at a symbol that IS
defined, which it names and rejects.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with 0 warnings and 0 errors in
their logs, and a bare `ctest --test-dir build` prints

    Start 1: text
    1/2 Test #1: text .............................   Passed    0.02 sec
    Start 2: owed
    2/2 Test #2: owed .............................   Passed    0.05 sec

    100% tests passed, 0 tests failed out of 2

with `text` itself at `text: 8 of 8 cases passed, 0 failed`.

**AND THE ONE OPTION KEY'S REASON IS A NEGATIVE MEASUREMENT.** At 10.6 the AppKit ships no `NSFontCollection.h`
at all — its font headers are `NSFont.h`, `NSFontDescriptor.h`, `NSFontManager.h` and `NSFontPanel.h` — so
`kCTFontCollectionRemoveDuplicatesOption` has no counterpart in the era to pair with, and its value is recorded
as the same inference the other unprovable keys carry.

**`CTFontManagerErrors` — the font manager's error vocabulary, including a mistake of Apple's own (2026-10-11).**
Apple ships this as a header of its own and `CTFontManager.h` INCLUDES it, so it is written first — Apple's own
include order rather than a preference. The codes are Apple's published values copied exactly, AND THAT INCLUDES A
COLLISION APPLE'S HEADER CARRIES: **`kCTFontManagerErrorInUse` and `kCTFontManagerErrorSystemRequired` are BOTH
`202`**, so the two refusals are indistinguishable to a caller. THIS LIBRARY REPRODUCES THAT RATHER THAN
REPAIRING IT: renumbering one would answer a question the era's framework cannot, and a caller comparing against
the era's constants would be wrong in a way that stays invisible until it matters. The suite asserts both the
values and the collision.

**AND THE TWO STRINGS ARE INFERENCES, WITH THE REASON MEASURED RATHER THAN ASSUMED.** Apple publishes no value for
`kCTFontManagerErrorDomain` or `kCTFontManagerErrorFontURLsKey`, and the rule that PROVES the other Core Text
keys — the AppKit declaring the same name with CT spelled NS — has nothing to pair with here: at 10.6
`NSFontManager.h` **DECLARES NO `extern const …StringRef` AT ALL**, and `FoundationErrors.h` names its domain
only in a comment. Both values are therefore the constants' own names with the leading `k` dropped, recorded as
the first thing to check when something can answer for them.

MEASURED: `cmake --build build` exits 0 with 0 warnings and 0 errors in its log, `nm -D build/libagora-text.so`
lists `kCTFontManagerErrorDomain` and `kCTFontManagerErrorFontURLsKey` as defined, and the suite is
`text: 9 of 9 cases passed, 0 failed` — the new case naming every code in both ranges and asserting the
collision.

**`CTFontManager` — the system-facing half: what exists, and making one available (2026-10-11).** The type after
`CTFontManagerErrors` in Apple's include order, declared and implemented: the three `Copy` doors, the family-name
comparator, `CreateFontDescriptorsFromURL`, `IsSupportedFont`, `GetScopeForURL`, the four registration doors and the
auto-activation setting pair, plus both constants. **ITS READ-ONLY HALF IS ONE WALK SEEN THREE WAYS** — the same
observation the seam records one framework over: `cg_font_enumerate` covers the configured font directories AND the
process's registrations, so the PostScript names, the family names and the URLs are ONE list projected three ways,
and a registration is visible to all three because there is no second list to fall out of step.

**THE CODE CARRIES TWO BOUNDARIES, BOTH DECIDED RATHER THAN DISCOVERED.** ONE: **THERE IS ONE SCOPE, AND IT IS THE
PROCESS'S.** `kCTFontManagerScopeUser` promises availability "to ALL processes … and in subsequent sessions" and
`kCTFontManagerScopeSession` covers the session; both need a PERSISTENT, CROSS-PROCESS font store this tier does not
have, and its registry is `cg_font_register`, which is the process's. So `GetScopeForURL` answers `Process` or
`None` — Apple's own two answers — and a request for another scope is **REFUSED: `false`, with the error
out-parameter LEFT UNSET**, because Apple's era has no code meaning "unsupported scope" and inventing one is the one
thing this library will not do. A caller tells the two failures apart by the error: set means the file (one of
Apple's own codes), unset means the scope. TWO: **THE FAMILY-NAME ORDER IS A RECORDED DEVIATION, IN BOTH DOORS THAT
HAVE ONE.** Apple's order — "accounting for foundry prefix … recognized prefixes are sorted after the un-prefixed
names" — turns on a list Apple does not publish, so it cannot be reproduced from the era's headers.
`CompareFontFamilyNames` is therefore an ordinary localized, case-insensitive, numeric-aware comparison, and
`CopyAvailableFontFamilyNames` returns the families in the order that comparator gives them; the two doors AGREE,
which is what a caller re-sorting the array relies on. The deviation is ASSERTED in the suite — `"Adobe Garamond"`
before `"Garamond"`, where Apple's rule puts the un-prefixed name first — so that removing it fails the suite
instead of passing it silently.

**AND TWO DOORS ARE OWED WITH THEIR REASONS, WHICH THE `owed` TEST NOW NAMES BESIDE THE COLLECTION'S TWO.**
`CTFontManagerEnableFontDescriptors` toggles descriptors "for font descriptor matching" and so waits for the tier
that owns matching, exactly as `CTFontDescriptorCreateMatchingFontDescriptors` does;
`CTFontManagerCreateFontRequestRunLoopSource` needs a font server on the other end of a port.
`kCTFontManagerErrorInvalidFontData` (104) is recorded as **UNREACHABLE HERE** rather than guessed at: separating
"not a font file" from "a font file whose data is invalid" needs a parse the engine's refusal does not expose.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with 0 warnings and 0 errors in their
logs, and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over
`text: 13 of 13 cases passed, 0 failed` — the four new cases holding the three-way listing, the comparator and its
asserted deviation, the registration round trip INCLUDING the scope refusal, and the preference round trip.
**AND ONE THING THE SUITE DOES THAT A READER DESERVES TO BE TOLD**: the preference case WRITES a preferences file,
under an identifier of its own (`com.example.agora-text-suite`) so that no real one is touched.

**`CTParagraphStyle` — the first type here that is purely a table (2026-10-11).** Apple's header prints a TYPE and a
DEFAULT VALUE for each of the fourteen specifiers, and those two facts are the object: `Create` copies what it is
given BY SIZE, the accessor writes the stored copy or the default when nothing was stored, and nothing in the
library has to know what a setting MEANS to carry it across. That last part is why the file is short: NOTHING HERE
LAYS ANYTHING OUT — the framesetter and the typesetter do, and both are later in Apple's include order.

**AND THE SUITE CAUGHT A REAL BUG IN IT, WHICH IS WHAT THE CASES ARE FOR.** The store holds a SCALAR as the ADDRESS
of its bytes and a CFTYPE as THE OBJECT ITSELF, and the accessor treated both as the first: it read *from* the array
instead of handing the array back, so a style's `TabStops` came out as the array's own first eight bytes — ZERO, so
the caller got NULL where its tab stops were. It is invisible for every one of the thirteen scalar specifiers and
wrong for the one that is not, which is exactly the shape of bug a case written against the DOCUMENTED contract
finds and a case written against the implementation does not. The failure was

    FAIL: the TabStops value did not come back
    FAIL: the style answered a DIFFERENT array than the one it was given
    FAIL: the TabStops value came back changed
    text: 14 of 15 cases passed, 1 failed

and the case that found it is the one that holds a style's tab stops THROUGH A RELEASE OF THE CALLER'S OWN
REFERENCE — so it also proves the retain, which a weaker case would not. Fixed, `15 of 15`.

**AND ONE ROW IS A RECORDED DEVIATION WITH ITS LANDING POINT NAMED.** Apple's default for
`kCTParagraphStyleSpecifierTabStops` is "12 left-aligned tabs, spaced by 28.0 points", and a tab is a
`CTTextTabRef` — whose header is LATER in Apple's include order, so neither it nor the door that makes one exists
yet. The default is answered as an EMPTY ARRAY of the right type; the case asserts both halves of that (the type
AND the emptiness) so that the day the tabs arrive the case fails and says so, and `ct_para_tab_fallback` is where
they will be built.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with 0 warnings and 0 errors in their
logs, and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over
`text: 15 of 15 cases passed, 0 failed`.
