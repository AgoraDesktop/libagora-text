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
**`CTLineCreateTruncatedLine` — the truncation engine, and the first owed door built from another door's arrival
(2026-10-11).** Its recorded reason for waiting was "it waits for the typesetter, whose job the breaking is" — A
CONDITION RATHER THAN A SENTENCE, which is what an owed reason is FOR — and THE TYPESETTER LANDED, so the door did. The
`owed` test lost its name in the same commit, because that test fails when an owed door ACQUIRES a definition.

**WHAT IS IMPLEMENTED IS APPLE'S PRINTED CONTRACT AND NOTHING MORE THAN IT.** Four sentences from its header are the
whole of it: the line "will be truncated if its width is greater than the width passed" (so a line that FITS comes back
whole); the token "will be added to the point where truncation took place"; "if this parameter is set to NULL, then no
truncation token is used, and the line is simply cut off"; and "if the width of the line specified in truncationToken
is greater, this function will return NULL if truncation is needed". The three types are Apple's own descriptions of
which end stays visible, and THE CASE MEASURES EACH ONE rather than restating it.

**AND ONE BOUNDARY IS HELD RATHER THAN GUESSED, AND IT IS THE FONT.** A LINE HERE HOLDS ONE FONT, so a TOKEN DRAWN IN
ANOTHER FACE cannot be added to it — the token's glyph INDICES are indices in ITS font, and inserting them into a line
drawn with the other would put garbage where an ellipsis belongs, which is a wrong answer that LOOKS like a truncation.
Apple's answer is a line with several runs; this tier's is to REFUSE, and the case holds the refusal (reached the
cheapest way there is: the same face at a different SIZE, which this library's font equality calls a different font).

**AND THE RESULT NEEDED ONE THING THE LIBRARY DID NOT HAVE: A LINE OVER GLYPHS RATHER THAN OVER A STRING RANGE.**
`CTLineCreateWithGlyphs` is a third declaration in `CTLine_internal.h`, and it exists because a MIDDLE truncation keeps
a prefix AND a suffix — so the result cannot be made from any one range of the string. It took one fix to land: the
seam's header had to include `CTFont.h`, because `CTLine.h` names no font (Apple's does not either), and an INTERNAL
header may bring a type a public one must not.

**AND ONE NUMBER APPLE DOES NOT PRINT IS NAMED WHERE IT IS CHOSEN**: for a MIDDLE truncation the available room is split
between the two ends, and the division is this tier's (half for the head, then what is left for the tail). Apple's
sentence describes the RESULT and says nothing about the division, so the division is recorded in `CTLine.c` rather than
presented as Apple's. THE RANGE OF A TRUNCATED LINE IS RECORDED FOR THE SAME REASON: it is the SPAN of the original the
result shows, which for a MIDDLE truncation covers characters the line does not draw — Apple prints nothing about that
door's range, and a span is what a caret needs.

**AND THE CASE'S FIRST RUN FAILED, WHICH IS WORTH KEEPING.** It asserted that a token in another font was refused EVEN
WHEN THE LINE FIT — which would have made a door that refuses lines it never had to touch. Apple's condition is about
the LINE's width, and the token is only reached when there is cutting to do; the case now asserts that round the other
way as well.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists `CTLineCreateTruncatedLine` and `CTLineCreateWithGlyphs`; and a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 26 of 26 cases passed, 0 failed`, with the `owed` test down to 5 names.
**`CTLineCreateJustifiedLine` — the justification engine, and `CTLine` owes nothing (2026-10-11).** Its header prints
LESS than the truncation one's, so THE TWO THINGS IT DOES NOT PRINT ARE DECIDED HERE AND NAMED AS THIS TIER'S RATHER
THAN PRESENTED AS APPLE'S.

**WHAT APPLE PRINTS IS THREE SENTENCES**, and all three are implemented: "When set to 1.0 or greater indicates, full
justification will be performed. If less than 1.0, varying degrees of partial justification will be performed. If set to
0 or less, then no justification will be performed"; the width is "the width to which the resultant line will be
justified"; and "If justificationWidth is less than the actual width of the line, then negative justification will be
performed ("text squishing")".

**WHAT IT DOES NOT PRINT IS WHERE THE SLACK GOES, AND THAT IS THE DECISION.** Full justification is the typographic word
for WIDENING THE GAPS BETWEEN WORDS — which is what `CTParagraphStyle`'s `kCTJustifiedTextAlignment` means by "Text is
fully justified", and why that same vocabulary says a paragraph's LAST line "is naturally aligned": a last line has no
gap to widen. So the slack is divided among the line's SPACE glyphs, applied AFTER each one, which leaves the first word
where it was and moves every word after a space. A line with NO SPACE — a single long word, or a face with no space
glyph — comes back over its own range, recorded as the case rather than stretched between its glyphs. SPREADING THE
SLACK BETWEEN EVERY GLYPH would have been the plausible wrong answer, and it is the one this file named when the door
was owed.

**AND THE FACTOR IS THE FRACTION OF THE SLACK, LINEARLY** — Apple says a factor below 1.0 gives "varying degrees of
partial justification" and prints no curve for how the degrees vary, so 0.5 justifies half way to the width. That is a
reading, and it is in `CTLine.c` where a reader can disagree with it.

**AND IT NEEDED NO NEW SEAM, WHICH IS WORTH NOTING BECAUSE THE TRUNCATION ENGINE DID.** A justified line has the SAME
glyphs and DIFFERENT ORIGINS, so it cannot be placed by the font's advances — but the helper that places it is STATIC
in `CTLine.c`, because the engine is its only caller. A seam is for two files that must agree; a helper is for one.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists `CTLineCreateJustifiedLine`; and a bare `ctest --test-dir build` is
`100% tests passed, 0 tests failed out of 2` over `text: 27 of 27 cases passed, 0 failed`, with the `owed` test down to 4 names — none of them in `CTLine`. THE CASE HOLDS APPLE'S THREE
SENTENCES AND THIS TIER'S TWO DECISIONS, reading the glyph POSITIONS through the run, because those are what
justification changes: the glyphs are the same ones and their origins have moved.
**The matching tier: `CTFontDescriptorCreateMatchingFontDescriptors` (and its singular form) and
`CTFontCollectionCreateMatchingFontDescriptors` (and its sorted form) — four doors, and the largest remaining piece
(2026-10-11).** They all ask ONE question — which installed faces does this query describe — so the answer has one home
(`CTFontMatching.c`, behind `CTFontMatching_internal.h`) and the doors are its four fronts. THE DESCRIPTOR'S TWO ARE
DEFINED THERE rather than in `CTFontDescriptor.c`, because the tier that owns matching owns the doors that ask a
matching question.

**THE SET IS APPLE'S RULE AS FAR AS ITS HEADER PRINTS IT.** A candidate matches a query when EVERY attribute the query
names is present in the candidate with an equal value, and A QUERY WITH NO ATTRIBUTES MATCHES EVERYTHING — which is what
makes the collection's "all available" question and an empty descriptor list the same question. The subtler half of
Apple's matching (traits, family-name matching, substitution) is NOT attempted, and the boundary is stated where the
rule is: half a matcher that returned plausible wrong faces would be worse than one whose rule a caller can read.

**AND THE ORDER IS THE USER'S DECISION, RECORDED WITH ITS REASON.** Apple's documentation defers the order to "system
precedence" and never says what precedence IS, which is why these doors were owed for the whole life of the library
before this commit. THE DECISION (user, 2026-10-11): sort BY FAMILY NAME with `CTFontManagerCompareFontFamilyNames` —
the comparator the manager already offers, itself a recorded deviation from Apple's UI order — BECAUSE STABILITY IS THE
PROPERTY THAT MATTERS: two runs of the same program see the same array, which is what a caller's own sorting actually
needs. AND THE COLLECTION'S SORTED DOOR IS APPLE'S OWN WAY OUT OF THE PROBLEM: its callback "defines the sort order", so
a caller who dislikes this tier's order supplies their own — and a caller who passes NONE gets this tier's, recorded as
an answer rather than a refusal.

**AND ONE BOUNDARY IS CARRIED BY THE DESCRIPTORS THEMSELVES**: they hold the three attributes this tier can state as
FACTS about a face — its URL, its PostScript name and its family, the family read from the `name` table through the
engine's own door. Apple's answer is "an array of NORMALIZED font descriptors" whose normalization is not printed, and
that normalization is not attempted.

**AND TWO CORRECTIONS CAME OUT OF WRITING IT, BOTH WORTH KEEPING.** ONE: THE DESCRIPTOR DOOR'S SECOND PARAMETER IS
`CFSetRef mandatoryAttributes`, NOT AN OPTIONS DICTIONARY — the first draft assumed options, as most Core Text doors with
a second parameter take them, and Apple's header said otherwise. It FILTERS THE RESULT (every returned descriptor must
carry those attributes), so demanding an attribute this tier does not build produces an EMPTY ARRAY, which is the honest
answer to a requirement that cannot be met. TWO: A CALLBACK'S TYPE HAS TO BE THE CALLBACK'S TYPE — the test's ordering
function was written with the generic comparator's `const void *` pair, and clang REJECTED it as an incompatible
function pointer, which is right: a callback whose parameter types differ is a different callback.

**AND THE UNIT CLOSED A HOLE IN THE `owed` TEST.** The descriptor's two matching doors had been DECLARED AND UNDEFINED
SINCE THE DESCRIPTOR WAS WRITTEN — the README recorded them as owed with their reason — and the test that holds that
property was built LATER, when `CTFontCollection` landed, so it named the COLLECTION's two and never picked up the
descriptor's. TWO DOORS ESCAPED THE CHECK THE TEST EXISTS TO MAKE, and this is the second time this tree has found a
document claiming something a measurement did not support. They are defined now, and the test names what remains.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with no compiler diagnostic at all in
their logs; `nm -D build/libagora-text.so` lists all four doors; and a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over
`text: 28 of 28 cases passed, 0 failed`, with the `owed` test down to 2 names.
**And the matching tier FALSIFIED an owed reason, which is recorded where the reason lives (2026-10-11).** The reason
`CTFontManagerEnableFontDescriptors` carried was a CONDITION: "matching is the thing this tier does not have ...
It lands with the tier that owns matching, beside them." THE TIER LANDED, so the condition is met and the reason is no
longer true. It is replaced in `CTFontManager.c` by what is now the case, and NOT by quietly implementing the door.

**WHAT THE DOOR NOW WAITS FOR IS A DECISION, AND IT IS WORTH STATING PRECISELY BECAUSE IT IS AN ARCHITECTURAL FIRST.**
`CTFontManagerEnableFontDescriptors` enables and disables fonts "based on the provided array of font descriptors" —
PROCESS-GLOBAL MUTABLE STATE, WHICH THIS LIBRARY WOULD HAVE FOR THE FIRST TIME: every object in it is immutable by
construction, which is why its registration doors, its collections and its lines all answer without a lock. Apple's own
header promises that all of its functions "are thread safe unless otherwise specified", so A LOCK OR A RECORDED CAVEAT
IS THE DECISION — and choosing one on the way past would be exactly the failure this tree refuses.

**AND THIS IS THE SECOND TIME AN OWED REASON HAS BEEN FALSIFIED BY A LATER COMMIT AND SAID SO RATHER THAN QUIETLY
REPAIRED**: `CTLineCreateTruncatedLine`'s reason named the typesetter, and the commit that brought the typesetter is the
one that noticed. AN OWED REASON IS A CONDITION THAT CAN BE CHECKED — that is what makes it different from a sentence —
and a condition that becomes true is a door that has to move.

MEASURED, from the run in the same command: a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 28 of 28 cases passed, 0 failed`, with the `owed` test at
2 names.
**The visibility door, the library's first mutable global, and the bug its case found (2026-10-11).** The owed reason for
`CTFontManagerEnableFontDescriptors` was a CONDITION — "It lands with the tier that owns matching, beside them" — AND THE
TIER LANDED, so the door did. It is DEFINED IN `CTFontMatching.c` RATHER THAN IN `CTFontManager.c`, because that is where
its state lives, and that is the same arrangement its four sibling doors already have with the descriptor and the
collection: the tier that owns something owns the doors that reach it.

**AND THE DECISION THE PREVIOUS COMMIT SURFACED WAS MADE, IN FAVOUR OF HONOURING APPLE'S OWN SENTENCE.** The door
enables and disables fonts "based on the provided array of font descriptors", which is PROCESS-GLOBAL MUTABLE STATE —
THE FIRST THIS LIBRARY HAS, since every object in it is immutable by construction and that is why its registration
doors, its collections, its lines and its matcher all answer without a lock. Apple's header promises that all of its
functions "are thread safe unless otherwise specified", SO THE STATE IS GUARDED BY A STATICALLY INITIALISED MUTEX RATHER
THAN CAVEATED: a caveat would have been a silent deviation from a documented contract, where the lock is a few lines and
keeps it. THE LIST IS NEVER HANDED OUT — a caller gets a boolean — so the lock is held for a handful of instructions and
no caller can hold it at all. A caller's descriptors are RETAINED by the list, so they cannot be freed underneath it.

**AND THE CASE FOUND A REAL BUG, WHICH IS WHAT IT WAS WRITTEN FOR: CFEqual WAS THE WRONG QUESTION.** The first draft
asked whether the disabled descriptor and a candidate were EQUAL, and the case failed at once — THE DESCRIPTORS THIS TIER
BUILDS FOR A FACE CARRY THREE ATTRIBUTES (its URL, its name and its family, each read from the face itself) WHILE THE
DESCRIPTOR A CALLER HANDS THE DOOR CARRIES ONLY WHAT THE CALLER WROTE DOWN, so the two are never equal, the disable had
no effect, and a "disabled" face stayed matchable. THE DISABLED LIST IS THEREFORE A LIST OF QUERIES, EXACTLY AS A FONT
COLLECTION IS: a face is disabled when some descriptor in the list DESCRIBES it, which is the same `ct_match_satisfies`
test a matching query gets, and ADD AND REMOVE ASK THE SAME QUESTION — so a caller disables a font by NAMING it and
re-enables it by naming it again. The bug was caught because the case held the STATE-CHANGE and not the door's return
value, which is void and could not have shown anything.

MEASURED, from a DELETED `build/` in the same command: configure and `cmake --build build` exit 0 with NO compiler
diagnostic at all in their logs; `nm -D build/libagora-text.so` lists the door; and a bare `ctest --test-dir build` is
`100% tests passed, 0 tests failed out of 2` over `text: 29 of 29 cases passed, 0 failed` — with **1** names in the `owed` test, which is now ONE DOOR: `CTFontManagerCreateFontRequestRunLoopSource`,
which needs a FONT SERVER on the other end of the port rather than anything this library can build. THAT IS THE WHOLE OWED
LIST.
**And the last owed door was given a DECISION rather than a sentence: the Agora font service, designed and not built
(2026-10-11).** `CTFontManagerCreateFontRequestRunLoopSource` had been owed with the reason "a FONT SERVER on the other
end of the port". That reason is still true, and the maintainer chose to define the missing side rather than leave the
door pointing at nothing — SO `FONT-SERVICE.md` RECORDS ITS SHAPE AND SAYS, IN ITS OWN FIRST LINE, THAT NOTHING IN IT IS
IMPLEMENTED. The door is still declared and not defined; the `owed` test still names it; no behaviour changed in this
commit and the measurement is unchanged.

**AND THE SURVEY IS THE VALUABLE PART, BECAUSE IT TURNED ASSUMPTIONS INTO MEASUREMENTS.** Read with `nm -D` on
`libagora-core` and `grep`: THE PORT NAMESPACE ALREADY EXISTS (`CFSocketRegisterSocketSignature`,
`CFSocketCopyRegisteredSocketSignature`, `CFSocketGetDefaultNameRegistryPortNumber` — which is exactly what Apple's own
error case, "a duplicate requestPortName", assumes); the RUN-LOOP INTEGRATION EXISTS (`CFSocketCreateRunLoopSource` and
the whole `CFRunLoop` family); a CF-NATIVE CODEC EXISTS (`CFPropertyListCreateData` / `CFPropertyListCreateWithData`),
which matters because THIS LIBRARY IS CORE FOUNDATION AND CORE GRAPHICS ONLY, so Foundation's property-list codec is not
available to it at any price; and a NAME TO DERIVE FROM EXISTS (`kCTFontManagerBundleIdentifier`, an Apple-era key).
`CFMessagePort` IS ABSENT, so there is no named-message primitive to lean on and the transport is `CFSocket`. And
`window-server/` is AN EMPTY SKELETON — one commit, a `.gitignore`, no README — so this would be AGORA'S FIRST SERVER
rather than a second one.

**AND ONE THING IS EXPLICITLY UNVERIFIED, WHICH IS THE HONEST FORM OF THE CLAIM**: those `CFSocket` registry symbols are
exported, but whether a RUNTIME RENDEZVOUS stands behind them is in `libagora-core`, a different repository, and was not
read. The design says so where it depends on the answer, and names checking it as the first task.

**AND THE DAEMON'S JUSTIFICATION IS STATED RATHER THAN ASSUMED**: a responder registers a port under a name derived from
its process (Apple's error case is a DUPLICATE name, so the name is derived rather than supplied), and ONLY A SYSTEM
COMPONENT CAN ENUMERATE PROCESSES to find another process's port — a random application cannot discover a random
application. That is the daemon's entire job, and it is why the system's half and this library's half are separate.

**The font service's WIRE, and the four faults its case found (2026-10-11).** `CTFontService_internal.h` and
`CTFontService.c` are THE FRAMEWORK'S HALF OF THE CONVERSATION `FONT-SERVICE.md` describes: a requester's encode, a
responder's answer (produced by calling the MATCHING TIER, which is why this lives in the library that owns matching),
and no socket, no port, no run loop and no daemon anywhere — so both halves are tested IN-PROCESS and neither can drift
from the other without the case failing.

**AND THE CASE FAILED FOUR TIMES BEFORE IT PASSED, WHICH IS THE POINT OF IT.** FIRST: the format was asked for as
BINARY, and BINARY SUPPORT BY THIS TREE'S PROPERTY-LIST LAYER WAS NEVER MEASURED — the first fix was to ask for XML
explicitly, because the form a reader can also write is the one to ask for. SECOND: the version was written as the
constant `CFSTR("1")` AND THEN RELEASED, which is a release of something not owned — a "CFSTR is a constant" bug that
the first draft carried happily, and the wire now passes the constant itself with no copy to own. THIRD: the decode
path RETURNED THE REPLY AND LEAKED THE PROPERTY LIST IT HAD DECODED; found by reading the code again rather than by any
test, and fixed. FOURTH, AND THE REAL ONE: **A MATCHED FACE'S ATTRIBUTES CARRY `kCTFontURLAttribute`, WHICH IS A `CFURL`
OBJECT, AND A PROPERTY LIST CANNOT HOLD ONE** — so the reply never encoded, and the case's very first assertion failed
for a reason no part of the design mentioned.

**AND THE FOURTH WAS FOUND BY ELIMINATION, WITH FIVE THROWAWAY PROBES OUTSIDE THE REPOSITORY**, which is the part worth
recording: a plain dictionary round-tripped; a dictionary OF dictionaries round-tripped; a dictionary OF ARRAYS (a
reply with one match and a reply with none) round-tripped; the version as a string was present after decoding; and the
DECODED VALUES' type ids and `CFEqual` results were all correct — including `CFStringGetTypeID` and
`CFDictionaryGetTypeID` matching, which is exactly the sort of thing a from-scratch Core Foundation gets wrong. Every
link in the chain behaved, which LEFT THE ONE VALUE TYPE NOTHING HAD TRIED TO SERIALIZE: the URL. THE WIRE NOW CARRIES
PROPERTY-LIST TYPES AND NOTHING ELSE, and a URL crosses it AS ITS STRING — which is a real asymmetry and is recorded as
one: a client that rebuilds a descriptor from the reply converts the string back, and the encoding is lossless for
every attribute EXCEPT that conversion. THE PROBES WERE SCRATCH AND ARE NOT IN THIS COMMIT; the code they produced is.

**AND THE DOOR'S REASON IS NOW NARROWER THAN IT WAS.** `CTFontManagerCreateFontRequestRunLoopSource` is still declared
and not defined, but what it waits for is no longer "a font server": the conversation exists. It waits on its TRANSPORT
(the port, the name, the daemon — FONT-SERVICE.md, still a plan) and on one build fact, the door's BLOCK SIGNATURE and
this target's lack of `-fblocks`. Two checkable things, neither of them a tier.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with NO compiler diagnostic at all; a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 30 of 30 cases passed, 0 failed`; and the `owed` test still names 1 door. Two documents say which half of the
service is built, and this commit changes both.
**The `owed` guard was blind to twenty-two declared doors, and the check that found it is now part of the record
(2026-10-11).** The library had an ACCOUNTING and a GUARD that disagreed, and only the accounting was right. `CTFont.c`
has carried an owed block naming its twenty-two declared-and-undefined doors with a checkable reason each — the
font-table readers (`fvar`, `avar`, `feat`, `post`, a raw `CTFontCopyTable`), the doors where a SYSTEM CHOOSES A FACE
(`CTFontCreateUIFontForLanguage`, `CTFontCreateForString`, the copy-with-traits and copy-with-family doors), and the
ATS pair that speaks in ids this tree has no system for. THE `owed` TEST NAMED ONE OF THEM. It was written when the
`CTFontManager` doors were owed and the older `CTFont` tier's were never added, so TWENTY-TWO DOORS WOULD HAVE PASSED
THAT TEST HAD THEY QUIETLY ACQUIRED A DEFINITION — which is precisely the property the test exists to hold, and this is
the THIRD time in this library that a guard has been found narrower than the truth it was guarding (the descriptor's
matching doors escaped the same list, and `CTLineCreateTruncatedLine`'s reason was falsified by a later commit).

**AND THE CHECK THAT FOUND IT IS THE ONE THE LIBRARY DID NOT HAVE: COMPARING THE DECLARED SURFACE WITH APPLE'S.**
`nm -D` on the built library against the fifteen `CT*.h` in the era's headers gives an arithmetic that CLOSES — 151
names Apple declares, 127 this library defines, 24 unaccounted for — and the 24 are 23 declared here and undefined plus
**ONE APPLE DECLARES AND THIS LIBRARY NEVER MENTIONS: `CTFontCreateWithQuickdrawInstance`**. Apple's 10.6 header
declares it plainly, with no guard; this library does not declare it at all, so a caller gets no symbol AND no
declaration — a louder absence than an owed door's link error, and until now an UNRECORDED one. It is recorded in
`CTFont.c` beside the other owed doors, with its reason: QUICKDRAW IS A DRAWING SYSTEM AGORA DOES NOT HAVE, and an
omission with no note is the one kind of absence nobody can check later.

**AND WHAT WAS FIXED IS THE GUARD, NOT THE ACCOUNTING**: the `owed` test's list now names all twenty-three doors whose
headers declare them and whose library does not define them, so it fails if any one of them silently acquires a
definition. Nothing was implemented by this commit and no behaviour changed.

MEASURED, from the run in the same command: a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 30 of 30 cases passed, 0 failed`, and the `owed` test now
names **23** doors where it named one.
**`CTFontCopyCharacterSet` is defined, by the inverse of the glyph step this library already had (2026-10-11).** The door
was owed with the reason "needs every character the face covers: the inverse of the glyph step, over the whole cmap, plus
the OS/2 and name-table data that languages come from" — AND THE FIRST HALF OF THAT SENTENCE NAMED ITS OWN ANSWER. The
seam door `cg_font_glyphs_for_unichars` maps a BATCH of UTF-16 code units per call, so the inverse is the forward step
asked exhaustively: the whole basic multilingual plane is SIXTEEN calls, the cost of a door that reads no table at all.
THE OS/2 AND NAME-TABLE HALF IS STILL OWED, in `CTFontCopySupportedLanguages` and `CTFontGetStringEncoding`.

**AND THE RULE IS STATED WHERE THE CODE IS, BECAUSE IT DECIDES THE ANSWER**: a code unit is in the set when the face maps
it to a NON-ZERO glyph, and glyph 0 is `.notdef` — "this face has nothing here" — which is the same rule the mapping
door reports as failure. Surrogates therefore fall out of the answer by themselves rather than needing a case of their
own, and a face that maps `U+0000` keeps it. AND THE CASE HOLDS THE ONLY THING THAT MATTERS: MEMBERSHIP AND THE MAPPING
DOOR AGREE, code unit by code unit, over a sample chosen to include characters a monospace face has, one it may or may
not have, and one that is not a character at all. IT NEVER ASSERTS WHICH — only that two doors say the same thing, which
is a fact whichever way the face answers.

**AND ONE BOUNDARY IS RECORDED RATHER THAN LEFT TO BE DISCOVERED: THE SCAN COVERS THE BASIC MULTILINGUAL PLANE AND NOT
THE PLANES ABOVE IT.** The seam's mapping takes UTF-16 CODE UNITS, so a surrogate pair is two code units and the second
of them alone maps to nothing; Apple reads a cmap, which would report that coverage. THE CONDITION THAT WOULD CLOSE IT
IS CHECKABLE AND SMALL — a seam door taking SCALAR VALUES instead of code units — and nothing else about this door would
change. It is recorded in `CTFont.c` beside the door.

**AND THE REST OF THE OWED FAMILY NOW HAS A NAMED KEYSTONE, FOUND BY LOOKING RATHER THAN GUESSING**: the font seam is
implemented in `libagora-drawing/CGFont.c` and sits on **FreeType**, and that file ALREADY INCLUDES
`FT_TRUETYPE_TABLES_H` — so `FT_Load_Sfnt_Table` is ONE FUNCTION AWAY, and with it the whole table family that is owed
here (`fvar`/`avar` for variation, `feat` for features, `post` for the underline metrics, and `OS/2`+`head` for traits —
whose reason for being owed is that deriving them from a subfamily STRING "would be a guess wearing a fact's clothes",
where a table read is the fact). **THAT SEAM DOOR BELONGS TO ANOTHER REPOSITORY**, which is what makes the family a
CROSS-REPO unit rather than a gate in this one. NAMED HERE SO THE NEXT UNIT STARTS FROM THE KEYSTONE RATHER THAN FROM
THE DOOR LIST.

MEASURED, from a DELETED `build/` in the same command: configure and `cmake --build build` exit 0 with NO compiler
diagnostic at all; a bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 31 of 31 cases passed, 0 failed`; and the `owed` guard — which now
names every declared-but-undefined door — is down to **22** names, having dropped `CTFontCopyCharacterSet` IN THE
SAME COMMIT THAT DEFINED IT, which is the discipline the previous commit established.
**The raw table doors, and the first owed reason falsified by work in ANOTHER repository (2026-10-11).**
`CTFontCopyTable` and `CTFontCopyAvailableTables` are DEFINED. They are the pair of the table family that needed no
parsing at all, because Apple's answer for a table IS the file's bytes: the door asks the seam for the size, allocates,
asks again, and wraps the result in a `CFDataRef` (the directory door builds an array of `CFNumber` tags, which is what
Apple's door answers and the reason is that a tag IS four bytes rather than a name anybody chose).

**AND THE REASON THEY WERE OWED HAD TO BE CORRECTED IN THE SAME COMMIT, WHICH IS THE POINT WORTH RECORDING.** The owed
block said the table family waited because "no seam door reads a table". `libagora-drawing` grew `cg_font_copy_table`
and `cg_font_table_tags` in its own commit, so THAT CONDITION BECAME FALSE — and an owed reason is a MEASURABLE
CONDITION rather than a sentence, which is what makes a false one findable. THE BLOCK WAS REWRITTEN RATHER THAN QUIETLY
REPAIRED, and what it says now is what is actually true: the family's reason was never the absence of a TABLE, it was
THE ABSENCE OF A DOOR TO ONE, and THAT DOOR EXISTS. What remains for each is the FORMAT PARSING on this side, named one
table at a time — `fvar` and `avar` with their axis records and normalised coordinates, `feat` with its versioned
feature records, and `post` with `underlinePosition`, `underlineThickness` and the sign convention Apple's numbers
follow. THIS IS THE FIRST TIME AN OWED REASON HAS BECOME FALSE BECAUSE OF WORK IN A DIFFERENT REPOSITORY, and the
correction landed beside the code that made it false.

**AND THE OPTION IS OBEYED TRIVIALLY, WHICH IS THE HONEST FORM OF "NOT OBEYED"**: `kCTFontTableOptionExcludeSynthetic`
asks for tables that are not synthesised, and NOTHING THIS DOOR RETURNS IS SYNTHESISED — every table comes out of the
file the face was opened from, so the flag has nothing to exclude and there is nothing to record as missing. A door
that answered `NULL` for the flag, or dropped tables for it, would be inventing a distinction the data does not have.

**AND THE CASE HOLDS THE PART THAT IS NOT NOTHING: THE DIRECTORY AND THE TABLE DOOR MUST AGREE.** Every tag the face
lists must be one the table door can produce with a non-empty answer, a tag no face carries must answer `NULL` rather
than invent a table, a NULL font must answer `NULL` from both doors, and the bytes must be THAT TABLE'S — `head`
BEGINS WITH ITS OWN VERSION, 0x00010000 for TrueType, which is a fact about the FORMAT and not about this library. And
the directory's elements must be NUMBERS, because a caller who cannot read a tag from the array cannot ask for the
table it names.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with NO compiler diagnostic at all; a
bare `ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 32 of 32 cases passed, 0 failed`; and the `owed` guard — which names every declared-but-undefined door —
is down to **20** names, having dropped BOTH table doors in the commit that defined them.
**The `post` metrics, the first doors here to read a table and interpret it (2026-10-11).**
`CTFontGetUnderlinePosition` and `CTFontGetUnderlineThickness` are DEFINED, and their reason — "the `post` table; the
seam reports no underline metrics" — was already false when this commit began, because the seam reads tables now. THE
OMITTED HALF OF THE REASON WAS THE INTERPRETATION, and that is what this commit supplies.

**AND THE INTERPRETATION IS SMALL ON PURPOSE, WHICH IS WHY THIS PAIR WENT FIRST.** `underlinePosition` and
`underlineThickness` sit at FIXED OFFSETS 8 AND 10 IN EVERY VERSION OF THE `post` TABLE — the versions differ in what
FOLLOWS them — so THERE IS NO VERSION CHECK TO GET WRONG, unlike `OS/2`, whose version decides whether a field exists at
all and whose readers in this tree already have to say so. The parse is two values, and the whole door is a table read, a
big-endian signed 16-bit read, and the scale every other metric in the library uses.

**AND THE BYTE ORDER IS THE PART THAT WOULD HAVE BEEN INVISIBLE IF IT WERE WRONG**: sfnt is BIG-ENDIAN, so a
little-endian read of these fields gives a PLAUSIBLE NUMBER rather than an error, and nothing downstream would complain
about a metric that is merely wrong. `fn_table_s16` does the read and says why in its own comment; it also SIGN EXTENDS
BY HAND rather than through a typedef, because the two bytes are big-endian but their SIGN is the format's.

**AND THE SIGN IS THE TABLE'S AND IS NOT INVERTED, RECORDED WHERE THE DOORS ARE**: a negative `underlinePosition` places
the line BELOW the baseline, which is the `post` table's own convention and the way the value is written down. Apple's
header says the door returns "the scaled underline position" and says NOTHING about a sign, so inverting it would be a
guess about a convention neither document states — and a caller who wants the other sign has an answer one subtraction
away. "SCALED" MEANS THE POINT SIZE, which is this library's rule for every metric: the matrix moves GLYPHS and not
metrics, which the font object's own fields say and which every metric door here already does.

**AND THE CASE DOES THE INTERPRETING ITSELF, WHICH IS THE ONLY WAY TO HOLD A PARSER TO ANYTHING**: it reads the `post`
table through the table door, takes the two values at their fixed offsets in BIG-ENDIAN, SCALES THEM BY THE POINT SIZE
ITSELF, and requires both metric doors to agree with that arithmetic — A PARSER CHECKED AGAINST ITS OWN OUTPUT CHECKS
NOTHING. It also holds that the answers are SCALED AND NOT RAW (a font unit is a fraction of a point, so a metric in the
tens would mean the scale was never applied), that the thickness is positive, and that a NULL font answers 0 and not a
metric.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with NO compiler diagnostic at all; a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 33 of 33 cases passed, 0 failed`; and the `owed` guard is down to **18** names, having dropped both `post`
doors in the commit that defined them.
**The symbolic traits, read from three tables rather than guessed from a string (2026-10-11).**
`CTFontGetSymbolicTraits` is DEFINED. Its owed reason was the sharpest in the library because it was A REFUSAL RATHER
THAN A SHORTAGE: "the seam reports no style flags, and deriving traits from a subfamily STRING would be a guess wearing a
fact's clothes". THE REFUSAL STANDS AND ALWAYS DID — what changed is that the FACT was in three tables the whole time,
and the seam has been able to hand them over since it grew a table door. `OS/2.fsSelection` (offset 62), `head.macStyle`
(offset 44) and `post.isFixedPitch` (offset 12) each sit at a fixed offset and are present in every version of their own
table for the fields read here, so there is no version check to get wrong.

**AND THE RULE FOR A FORMAT THAT STATES THINGS TWICE IS THAT EITHER STATEMENT COUNTS**: `fsSelection` and `macStyle`
both mark bold and italic, and a face that says so in either is saying so. The two width sources work the same way, and
`usWidthClass` IS CONSULTED HERE FOR THE FIRST TIME — 1 to 4 condensed, 6 to 9 expanded, 5 THE MIDDLE AND NEITHER, which
is the field's OWN meaning rather than a threshold anybody picked, while 0 means the table says nothing rather than
saying "medium".

**AND FOUR TRAITS ARE DELIBERATELY NOT SET, RECORDED IN THE DOOR RATHER THAN LEFT TO BE ASSUMED.** The CLASS MASK is a
mapping from `sFamilyClass` that no header in the era prints — the header itself says the classes "correspond CLOSELY to
the font class values in the OpenType 'OS/2' table", and "closely" is a correspondence rather than a table. VERTICAL
needs the font's own direction, which no door here reports. `UIOptimized` has no source in these tables, and neither do
the traits the MODERN headers add for colour and composite glyphs, which this era does not have at all. A TRAIT THIS DOOR
DOES NOT KNOW IS LEFT OUT RATHER THAN GUESSED: a caller who checks one bit can survive silence where a plausible wrong
bit would be believed.

**AND THE ERA'S SPELLING COST ONE ATTEMPT, WHICH IS WHY IT IS RECORDED**: this header declares `kCTFontBoldTrait`,
`kCTFontItalicTrait` and their siblings — the MODERN headers RENAMED them to `kCTFontTraitBold` and `kCTFontTraitItalic`
— and the first draft wrote the modern names. AN ASSERT AGAINST `CTFontTraits.h` STOPPED THE EDIT BEFORE A LINE WAS
WRITTEN, which is the argument for checking a name rather than remembering it; the door now uses the era's names,
because a library that declared the era's surface should answer in it.

**AND THE CASE HOLDS THREE THINGS, THE THIRD OF WHICH KEEPS THE OTHER TWO HONEST**: the face this suite loads is
`DejaVuSansMono-Bold`, so BOLD MUST be set — and MONOSPACE, since it is fixed-pitch; THE DOOR MUST AGREE WITH THE TABLES
IT READS, which the case computes for itself out of the raw `OS/2` and `head` bytes rather than asking the door twice;
AND THE TABLES MUST ACTUALLY STATE IT, because a check that passes with both sides silent proves nothing at all. A NULL
font answers 0.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with NO compiler diagnostic at all; a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 34 of 34 cases passed, 0 failed`; and the `owed` guard is down to **17** names, having dropped
`CTFontGetSymbolicTraits` in the commit that defined it — and NOT BEFORE, which this unit had to learn: an earlier
attempt in the same session removed the guard entry while the door was still undefined, and put it back before anything
was built. THE DOOR AND THE GUARD MOVE IN ONE COMMIT, DOOR FIRST.
**The variation instance, and the font-creation bug it exposed (2026-10-11).** `CTFontCopyVariation` is DEFINED, and it
needed no table at all, because ITS OWED REASON WAS WRONG ABOUT IT. The reason said the variation doors were "the `fvar`
and `avar` tables"; APPLE'S OWN WORDS SAY OTHERWISE — this door returns "the CURRENT VARIATION INSTANCE as a dictionary"
while the axes door returns "an ARRAY of variation AXES" — AND AN INSTANCE IS A CHOICE WHERE AN AXIS IS A CAPABILITY. The
choice is the descriptor's, and this tier has held a descriptor since it began. `fvar` is still owed, for the axes door,
and the owed block now says so with a note that no variable font is installed on this machine, so a case there could
only exercise the no-`fvar` path.

**AND ONE BOUNDARY IS RECORDED RATHER THAN IMPLIED BY THE DOOR'S PRESENCE: THIS TIER CAN REPORT THE INSTANCE IT WAS ASKED
FOR AND CANNOT APPLY IT.** Nothing in the seam sets a variation coordinate on a face, so the glyphs a caller draws from
such a font are the DEFAULT instance's. The door answers what the font REPRESENTS, which is honest, and that is not the
same as what it RENDERS, which is a capability the engine here does not have — a caller who needs the applied instance
needs a seam door that sets coordinates, and there is none. AND A FONT WITH NO INSTANCE ANSWERS NULL RATHER THAN AN EMPTY
DICTIONARY: Apple's header describes the answer for a font that has one and says nothing about one that has not, so this
is a CHOICE, recorded as one, on the grounds that "there is none" is said more plainly by NULL.

**AND THE DOOR EXPOSED A REAL BUG IN FONT CREATION, WHICH IS THE PART WORTH CARRYING FORWARD.**
`CTFontCreateWithFontDescriptor` read the NAME out of the caller's descriptor and called `CTFontCreateWithName`, LETTING
EVERY OTHER ATTRIBUTE FALL — against this file's own struct, whose field says "RETAINED: what this font was asked for"
while the code did the opposite. WHAT IT COST WAS EVERY ATTRIBUTE THAT IS NOT A NAME: a variation instance, a feature
setting, a trait request, all discarded on the way in, and `CTFontCopyVariation` is simply THE FIRST DOOR THAT COULD SHOW
IT. The font now keeps THE CALLER'S ATTRIBUTES MERGED WITH THE SIZE IT ACTUALLY RESOLVED — from the font rather than from
the parameter, because the parameter is allowed to be 0.0 and the size the font resolved is the truth then. THE FIELD IS
ASSIGNED AFTER CREATION, which is safe HERE and nowhere else: the font has not been returned to anybody, and fonts are
immutable once visible, which is why nothing else in the file assigns that field.

**AND ONE PROCESS CORRECTION IS RECORDED, because it happened twice in two units**: the `owed` guard entry for a door
must move IN THE SAME SCRIPT that defines it, sequenced after the write — an earlier attempt in this session dropped a
guard entry while its door was still undefined (twice), leaving a tree whose guard and truth disagreed until the next
step noticed. THE RULE IS NOW MECHANICAL RATHER THAN REMEMBERED: door first, guard second, one script, asserted anchors.

MEASURED, from a DELETED `build/`: configure and `cmake --build build` exit 0 with NO compiler diagnostic at all; a bare
`ctest --test-dir build` is `100% tests passed, 0 tests failed out of 2` over `text: 35 of 35 cases passed, 0 failed`; and the `owed` guard is down to **16** names.