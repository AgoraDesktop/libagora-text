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

**`CTLine` — the first thing here that is BUILT rather than stored, and the first that DRAWS (2026-10-11).** The type
after `CTParagraphStyle` in Apple's include order, declared and implemented. `CTLineCreateWithAttributedString` makes
ONE RUN: the string's characters become glyphs through `CTFontGetGlyphsForCharacters` — the door this library was
started for — each glyph is placed by `CTFontGetAdvancesForGlyphs`, and the font's own ascent, descent and leading are
the line's heights. Then every measuring door is arithmetic over those (glyph count, string range, typographic bounds,
flush offset, trailing whitespace, image bounds), the caret pair inverts, and `CTLineDraw` puts the ink on a page
through `CGContextShowGlyphsAtPositions`.

**THE SINGLE-RUN BOUNDARY IS WRITTEN WHERE A CALLER READS IT, NOT LEFT TO BE DISCOVERED FROM A WRONG LAYOUT.** Apple's
own sentence for the creator is that "without a typesetter object, the line cannot be properly broken", so this is the
simple case: NO LINE BREAKING, NO TWO FONTS IN ONE LINE, NO BIDIRECTIONAL REORDERING AND NO SHAPING — a character's
glyph is drawn where its own advance puts it, in the order the characters appeared. Multi-run, bidi and complex scripts
are the HarfBuzz milestone this project's plan already names. AND THREE DOORS ARE OWED WITH THEIR REASONS:
`CTLineGetGlyphRuns`, because a line's runs are `CTRunRef`s and `CTRun` does not exist yet — Apple's own include order
puts its header AFTER `CTLine.h`, which is why that header never mentions the type, and this is the first owed door
whose reason is another HEADER rather than another engine; and `CTLineCreateTruncatedLine` with
`CTLineCreateJustifiedLine`, which need the truncation and justification engines — where an ellipsis may fall, which
clusters may not be split, and where justification slack goes are rules Apple states NOWHERE in the header, and
spreading the slack evenly would look plausible and be wrong. The `owed` test now names SEVEN doors.

**AND `CTLineDraw`'S ONE CROSS-FRAMEWORK SEMANTIC IS MEASURED RATHER THAN ASSUMED.** This Core Graphics' own header
says `CGContextShowGlyphsAtPositions` takes positions "specified in user space" and that "the CTM alone places the
pen" — UNLIKE the three advance doors, whose pen "starts at the TEXT POSITION" — so the line's origin is applied by
`CTLineDraw` itself. THE CASE DRAWS THE SAME LINE AT TWO TEXT POSITIONS INTO A REAL BITMAP AND CHECKS THE INK MOVED BY
THE SAME 30 POINTS (to within five), because a comment cannot tell those two readings apart and a picture can. AND THE
BITMAP ITSELF TAUGHT THE CASE SOMETHING: this library's bitmap contexts SUPPORT EXACTLY ONE PIXEL FORMAT
(`kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little`) and REFUSE the others rather than reinterpret them, so
the case asks for that one — a case asking for another would have been testing the refusal.

**AND THE FIRST RUN OF THAT CASE FOUND TWO THINGS, NEITHER OF THEM THE LINE'S FAULT, AND BOTH WORTH KEEPING.** ONE IS
A FINDING RATHER THAN A BUG: **THIS TIER'S `CTFontGetDescent` RETURNS A NEGATIVE NUMBER**, because the seam answers
`CGFontGetDescent` with the engine's own `face->descender` — negative in FreeType's convention — and nothing on the way
up changes it. APPLE'S OWN HEADER DOES NOT SETTLE THE SIGN: it says only "the scaled font descent metric", scaled by
the size and matrix. So the case asserts THAT the line's heights are the font's, and no more, and the sign is recorded
here as an OPEN QUESTION for the tier that can answer it — the AppKit's own `NSFont`, or a measurement on the era —
rather than invented inside a case. THE OTHER WAS THE CASE'S OWN MISTAKE, which is the kind worth writing down too: it
clicked EXACTLY halfway between two glyphs, where the answer is a TIE — this implementation gives ties to the glyph on
the right — so its clicks moved INSIDE an advance, where the answer is a contract rather than a coin toss.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists the eleven defined `CTLine` doors and NONE of the three owed ones; and
a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over
`text: 17 of 17 cases passed, 0 failed`.

**`CTRun` — the type a line is made of, and the one `CTLineGetGlyphRuns` was owed against (2026-10-11).** Declared and
implemented: the four arrays (glyphs, positions, advances, string indices) with a **pointer** door and a **copying**
door each, the attributes, the string range, the status, the text matrix, the typographic and image bounds, and a draw
over a RANGE. **AND `CTLineGetGlyphRuns` IS DEFINED IN THE SAME COMMIT** — it was owed for exactly as long as this type
did not exist, which is what Apple's own include order records by putting `CTRun.h` AFTER `CTLine.h`. THE `owed` TEST
LOST THAT NAME AT THE SAME MOMENT, and that is not tidiness: it fails when an owed door ACQUIRES a definition, so a
line that answered runs while the test still named it would have failed the suite. The two moved together because
neither could move alone.

**THREE DECISIONS IN THIS FILE ARE WORTH THE SPACE THEY TAKE, AND ALL THREE ARE THINGS A CALLER CAN OBSERVE.** ONE:
**A RUN OWNS ITS ARRAYS.** It is independently retainable, so it cannot be a view over its line's storage — the four
arrays are copied at creation, which costs a second copy of the glyph data per line and is paid rather than answering a
retained run with a dangling pointer. TWO: **THE POINTER DOORS NEVER ANSWER NULL**, because the run always keeps its
data contiguous — Apple's contract for each of them is "the caller should be prepared for this function to return NULL
… [and] allocate their own buffer", and this implementation's answer is stronger, so the case asserts that the pointer
door and the copying door AGREE rather than checking one of them. THREE: **THE RANGE RULE IS APPLIED IN ONE PLACE.**
Apple's own sentence — "If the length of the range is set to 0, then the copy operation will continue from the range's
start index to the end of the run" — lives in `ct_run_clamp`, which the eight array doors and the two range-taking
measurement doors all share, because eight copies of a rule would be eight chances to disagree.

**AND TWO ANSWERS ARE THIS TIER'S RATHER THAN DERIVED, ASSERTED SO THAT CHANGING THEM IS DELIBERATE.
`CTRunGetStatus` IS `NoStatus` AND `CTRunGetTextMatrix` IS THE IDENTITY**, because this library does no bidirectional
reordering and sets no run matrix; the three status flags (`RightToLeft`, `NonMonotonic`, `HasNonIdentityMatrix`) exist
for the tier that does. AND THE SEAM IS A HEADER OF ITS OWN: `CTRun_internal.h` is NOT part of Core Text's surface —
Apple has no public creator for a run, because a line is what produces one — and it exists so that the run and the line
agree about how one is built instead of one file `extern`ing what the other defines.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists the seventeen `CTRun` doors and, of the two CTLine doors still owed,
neither; and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over
`text: 18 of 18 cases passed, 0 failed`. The new case checks the run's glyphs against the FONT'S OWN MAPPING of the
same string, its positions against its own advances, its width against the LINE's, its attributes against the string's,
its range against the line's, and its draw against PIXELS — half the run marking ink strictly inside the whole.

**`CTTypesetter` — the first thing in this library that DECIDES rather than measures (2026-10-11).** Declared and
implemented: its two creators (and the two option constants, inferred as the other unpublished keys are),
`CTTypesetterCreateLine`, and the two breaking doors. THE WHOLE ALGORITHM IS APPLE'S OWN SENTENCE — "The line break can
be triggered either by a hard break character in the stream or by filling the specified width with characters" — so a
newline ends the line WHEREVER IT IS and the line includes it, and otherwise characters are taken until the width is
full. The whole string is typeset ONCE (glyphs and advances for every character, in the creator), which is what makes
every later question a sum over an array rather than a second pass over the text.

**AND THE PARAGRAPH STYLE REACHES IT — THE FIRST TIME IN THIS LIBRARY THAT A RECORDED SETTING CHANGES WHAT THE
LIBRARY DOES.** The typesetter reads the string's paragraph style through a SECOND attribute key carried as a recorded
value (`kCTParagraphStyleAttributeName`, whose header is later in the include order, exactly like the font's) and
honours the line-break mode: WORD WRAPPING backs off to the last space that fits and INCLUDES IT — a line's trailing
whitespace is part of its range, which is why `CTLineGetTrailingWhitespaceWidth` exists at all — CHAR WRAPPING breaks
at the width, and CLIPPING and TRUNCATING return the whole remainder, because what those modes do is a matter of
DRAWING (a clipped line is cut off; a truncated one is given an ellipsis by the framesetter) rather than of which
characters belong to the line. THE CASE PROVES THE SETTING REACHES THE BREAKER BY DIFFERENCE rather than by
restating it: `"one two three"` at seven advances breaks after **4** characters under the default and after **7**
under a char-wrapping style.

**AND THE TWO BREAKING DOORS DIFFER IN EXACTLY THE WAY APPLE NAMES**, which is the whole of the difference: the cluster
door "is similar to a character break, except that it will not break apart linguistic clusters. NO OTHER CONTEXTUAL
ANALYSIS WILL BE DONE" — so it never backs off to a space and reads no paragraph style. Seven advances of
`"one two three"` gives 4 from the line door and 7 from the cluster door, and the case asserts BOTH NUMBERS, because
"they differ" would pass on an implementation that got both wrong.

**WHAT IS NOT ATTEMPTED IS STATED WHERE A CALLER READS IT**: nothing of UAX #14, no hyphenation, and NO BIDIRECTIONAL
REORDERING AT ALL — so `kCTTypesetterOptionDisableBidiProcessing` is RECORDED and cannot change an answer, which is
the difference between a door that cannot do something and one that pretends it did.

**AND THE SEAM IS A NEW INTERNAL HEADER.** A typeset line is a line over a RANGE of the string, and Apple has no
public door for one — in Apple's implementation the typesetter owns the runs and the line is a view of them; here they
are separate objects, so `CTLine_internal.h` is where they agree. IT ALSO CARRIES THE FONT-KEY ACCESSOR, so that value
is written ONCE: the line and the typesetter both read the same key through the same function, which is also why
`CTLine.c`'s private `ct_line_font_attribute` became `CTLineFontAttributeName`. The case holds the property that makes
the seam necessary: a line cut from characters 4..6 reports the range **(4, 3)** — the STRING's coordinates, not the
line's own.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists `CTTypesetterGetTypeID`, both creators, `CreateLine`, both breaking
doors and both option constants; and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2`
over `text: 20 of 20 cases passed, 0 failed`. AND THE REFACTOR WAS CHECKED BY MORE THAN THE NEW CASES: changing
`CTLine.c` to build over a range left all eighteen earlier cases passing, which is the regression evidence for the one
touch this unit made outside its own files.

**`CTFrame` and `CTFramesetter` — the text-layout half of Apple's include order, finished (2026-10-11).** Declared and
implemented: a framesetter's five doors (one creator, `CreateFrame`, `GetTypesetter`,
`SuggestFrameSizeWithConstraints`, `GetTypeID` — and no options door, because Apple adds `…AndOptions` LATER in the
era, which is a fact about the 10.6 header rather than a decision here) and a frame's ten (two ranges, path, frame
attributes, lines, line origins, draw, `GetTypeID`, and the progression vocabulary with its attribute key as an
inference). APPLE'S SENTENCE IS THE ALGORITHM ONCE MORE: filling continues "until it either runs out of text or it
finds that text no longer fits" — so a frame may show LESS than it was asked for, and `CTFrameGetVisibleStringRange`
is the door that says how much.

**THE LAYOUT IS SIX LINES AND EVERY DECISION IN IT BELONGS TO SOMEBODY ELSE.** The typesetter says where each line ends
(asked with a width), the line says how tall the text is (asked for the font's metrics), and the path says how much
room there is; the framesetter adds the ARITHMETIC BETWEEN THEM. The first baseline is one ascent below the path's
top and each line after it is one line height lower, which is what makes a frame's origin its BOTTOM-LEFT. THE CASE
HOLDS THE PROPERTY THAT MATTERS RATHER THAN ANY SINGLE NUMBER: the lines TILE the visible range — the first starts
where the range does, each starts where the last ended, and together they cover every character the frame claims to
have shown — and the origins DESCEND and lie inside the box. A gap would drop text and an overlap would draw it twice,
and no accessor-by-accessor assertion could tell either.

**AND TWO BOUNDARIES ARE RECORDED WHERE A CALLER READS THEM.** ONE: THE PATH IS READ AS ITS BOUNDING BOX. Apple fills
the path's SHAPE, reflowing a line around a notch or inside a circle; this tier fills the path's BOX, which is exact for
every rectangle and wrong for everything else, because the rules for filling an arbitrary outline belong to the text
system and not to a signature — and a rectangle is what a page of text actually is. TWO: THE PROGRESSION IS READ AND NOT
HONOURED. `CTFrameProgression` exists for VERTICAL text, where lines stack in columns; this tier lays out horizontally
only, so a frame attribute asking for right-to-left or left-to-right progression is KEPT (the dictionary is retained
whole) and changes no origin.

**AND A FRAME HAS NO PUBLIC CREATOR EITHER, WHICH MAKES A THIRD INTERNAL SEAM.** `CTFrame_internal.h` is where the
framesetter and the frame agree about how one is built. THE LINES AND THEIR ORIGINS ARRIVE TOGETHER — a frame with one
and not the other is not a frame, and a seam that allowed it would be a seam that lets a frame be built wrong — and
both are COPIED, so a frame can never be left pointing at a framesetter's finished stack.

**AND TWO MISTAKES OF MY OWN WERE FOUND BY THE FIRST RUN, BOTH WORTH KEEPING.** ONE WAS A LATENT BUG IN THE IMPLEMENTATION
THAT READING THE CASE'S FAILURES SURFACED: the fit test compared the line's baseline against **ZERO** rather than against
the PATH's origin, so a frame drawn away from the origin would have mis-measured every line; it is now a RECTANGLE test
in the path's own coordinates. THE OTHER WAS THE CASE'S: it expected **49** characters of a string that has **48**, and
it drew a frame whose top line sits at y ≈ 391 into a bitmap **200** pixels tall, so the ink landed outside the buffer —
a draw case that had asserted only "no crash" would have passed while measuring nothing.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists a frame's nine doors plus the seam and a framesetter's five; and a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 21 of 21 cases passed, 0 failed`.

**`CTStringAttributes` — the attribute vocabulary, and TWO RECORDED BOUNDARIES RETIRED (2026-10-11).** Fourteen keys and
the two underline vocabularies (`CTUnderlineStyle`, whose members are 0x00, 0x01, 0x02 and **0x09** — a gap a counting
implementation would fall into — and `CTUnderlineStyleModifiers`, whose patterns live in the HIGH BYTE so that a style
and a pattern combine into one number). NO DOORS AT ALL: a string attribute is a key and the value belongs to somebody
else, so everything here is a constant or an enum.

**AND THE PAYOFF IS THE TWO VALUES THAT STOPPED BEING DUPLICATED.** `CTLine.c` and `CTTypesetter.c` each carried the
VALUE of an attribute key as a local literal — `"NSFontAttributeName"` and `"NSParagraphStyleAttributeName"` — with the
reason written down: the constants live in `CTStringAttributes.h`, which was later in the documented include order, so
each file read the key by a value of its own and the comment said the function would go the day that header landed. IT
HAS LANDED: both now return `kCTFontAttributeName` and `kCTParagraphStyleAttributeName`, the values exist in EXACTLY ONE
PLACE, and the retirement was checked by every one of the twenty-one earlier cases still passing — which is the evidence
that a key read by a copy and a key read by the constant are the same key.

**AND TWELVE OF THE FOURTEEN KEYS MOVED FROM INFERENCE TO PROOF, BY MEASUREMENT.** The rule is the font descriptor's —
a dictionary CROSSES between Core Text and the AppKit, so the string has to be the one the other framework reads it by,
which makes the AppKit's spelling of the same name evidence for Core Text's value. THE MEASUREMENT: Apple's
`NSAttributedString.h` at 10.6 declares TWENTY-SIX `NS…AttributeName` constants, and TWELVE of the fourteen keys below
pair with one exactly. THE TWO THAT DO NOT are `kCTForegroundColorFromContextAttributeName` and
`kCTVerticalFormsAttributeName`, and each has a reason rather than an accident: the first pairs with nothing because the
AppKit's counterpart carries an `NSColor` where Core Text carries a `CGColorRef`, and the second exists for the
VERTICAL-text support Core Text has and the AppKit's attribute vocabulary does not name — the same support `CTFrame`'s
progression records as not honoured here. The case names all fourteen, asserts the count of provable ones, and holds the
underline gap and the pattern byte.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists the fourteen keys; `grep` over the library's sources finds the two
retired VALUES in exactly one file; and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2`
over `text: 22 of 22 cases passed, 0 failed`.

**`CTTextTab` — the tab type, and the third recorded deviation PAID (2026-10-11).** Five doors: a type id, a creator
taking an alignment, a location and an options dictionary, and three getters. THE OPTIONS ARE KEPT AND NOT READ — Apple
names no key for a per-tab options dictionary anywhere in the era's headers, so there is nothing to obey, and the door
hands back the very dictionary it was given.

**AND IT PAYS THE DEBT ITS OWN HEADER RECORDED.** `CTParagraphStyle`'s `kCTParagraphStyleSpecifierTabStops` default is
printed in Apple's header as "12 left-aligned tabs, spaced by 28.0 points" — and a tab is a `CTTextTabRef` whose header
was LATER in the documented include order, so that row could not be built and answered an EMPTY ARRAY instead, with a
comment naming its own landing point: "the tabs arrive with CTTextTab, and this function is where they will be built".
THEY HAVE ARRIVED: `ct_para_tab_fallback` builds twelve tabs — `kCTLeftTextAlignment` at 28.0, 56.0 … 336.0, the
multiples of 28.0 Apple's second number names — and the paragraph style's case, WHICH HAD BEEN ASSERTING THE EMPTY
ARRAY, now asserts the count, each tab's alignment and each location. THAT IS THE EVIDENCE THE RETIREMENT IS REAL: a
change that altered behaviour would have FAILED the case that recorded the deviation rather than passing a new one.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; a `grep` over the library's sources finds each of the two retired attribute-key values in exactly one file;
and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 23 of 23 cases passed, 0 failed`.
**`CTGlyphInfo` — the glyph substitution, and the last of the vocabulary (2026-10-11).** Three creators, three getters
and a type id, over a glyph named three ways: BY NAME (resolved through the face with `CTFontGetGlyphWithName`, the name
KEPT so the getter answers it), BY INDEX, and BY CID IN A COLLECTION. It is the value of `kCTGlyphInfoAttributeName`,
which `CTStringAttributes` already declared, so this closes the loop between the two vocabulary headers.

**AND THE THREE WAYS ARE NOT THE SAME ANSWER THREE TIMES.** An INDEX-made glyph info stores no name, and its getter
RESOLVES ONE LAZILY through the engine's glyph-name door — the seam door that exists for exactly this, cached because a
`Get` reference belongs to the object. A NAME-made one keeps what it was given. And A CID-MADE ONE KEEPS ITS COLLECTION
AND HAS NO GLYPH AT ALL: a CID is a key into a COLLECTION's CMap, and a collection is a mapping table this tier does not
have — the same standing the character set, the platform font and the vertical metrics carry, and it is ASSERTED AS A
BOUNDARY by the case rather than left for a caller to discover.

**AND ITS COLLECTION VOCABULARY HAS NO GAPS**, the opposite shape to the underline one this repository's attribute case
holds: the identity mapping is 0 and the five Adobe collections are 1 through 5 in Apple's order. The case asserts both
the values and the absence of a gap.

**AND THIS PARAGRAPH IS A CORRECTION, WHICH IS WHY IT IS HERE.** The commit that implemented this type
(`9c9beab`) CLAIMED that "README.md's status and PROVENANCE.md's Agora-changes section are changed in this same
commit" — AND IT WAS NOT TRUE WHEN IT WAS WRITTEN. The script that writes these two documents asserts on its ANCHORS
before writing anything, and the README's anchor did not match, so it aborted; the shell commands that committed were
separated by newlines rather than gated on that script, so the commit proceeded and the documents were left behind.
THE FAULT IS THE ONE THIS TREE HAS NOW RECORDED FOUR TIMES — a claim written from what the run was EXPECTED to do —
and the remedy is the same one: the two documents are written HERE, by a script that APPENDS rather than matching an
anchor, and the numbers quoted are the ones CAPTURED from the run below.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists all seven glyph-info doors; and a bare `ctest --test-dir build` is
`100% tests passed, 0 tests failed out of 2` over `text: 24 of 24 cases passed, 0 failed`.
**`CoreText.h` and `CoreText.c` — the umbrella, and the end of Apple's include graph (2026-10-11).** Fourteen headers in
the order Apple includes them, plus the ONE DOOR NO OTHER HEADER COULD DECLARE (`CTGetCoreTextVersion`, because a
version is a fact about the framework) and the six `kCTVersionNumber…` constants, which are MACROS in Apple's header and
therefore macros here — a caller compares them without linking anything.

**AND THE VERSION IS A CLAIM ABOUT THE SURFACE, NOT ABOUT THE PLATFORM, WHICH IS WORTH STATING WHERE THE NUMBER IS.**
`CoreText.c` answers `kCTVersionNumber10_6`: every header in this library was written from Apple's 10.6 headers, every
value is the era's, and every door is either defined or owed with its reason written down — so a caller written against
the 10.6 API gets the number it expects and FINDS THE SURFACE IT EXPECTS, because that surface is what is here. It is
NOT "this is macOS 10.6", and the comment says so.

**AND ONE HEADER IS ABSENT FROM THE UMBRELLA, WHICH IS APPLE'S OMISSION REPRODUCED RATHER THAN REPAIRED.**
`CTFontManagerErrors.h` is not in Apple's include list and not in this one; a caller who includes the umbrella still HAS
it, because `CTFontManager.h` includes it — which is why that header was written first among its neighbours. Adding it
here would be a kindness that made this file disagree with the framework it duplicates.

**AND THE UMBRELLA IS VERIFIED AS AN UMBRELLA.** The suite includes it FIRST — before the individual headers — so that
every door below is reachable THROUGH it; and the case asks each of the ELEVEN headers that declare a type for its type
id THROUGH THE UMBRELLA, calls the version door, and checks Apple's own recommended use of it (the pointer is callable
and the number is at least 10.5). AN INCLUDE LIST THAT LOST A HEADER WOULD FAIL THAT CASE, rather than a caller months
later.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists `CTGetCoreTextVersion`; and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2`
over `text: 25 of 25 cases passed, 0 failed`.

**AND WITH THIS THE ORDER IS DONE**: `CTFontTraits`, `CTFontDescriptor`, `CTFont`, `CTFontCollection`,
`CTFontManagerErrors`, `CTFontManager`, `CTParagraphStyle`, `CTLine`, `CTRun`, `CTTypesetter`, `CTFrame`,
`CTFramesetter`, `CTStringAttributes`, `CTTextTab`, `CTGlyphInfo` and now the umbrella — seventeen headers written from
Apple's own, with every owed door named and every recorded deviation either paid or still carrying its reason.