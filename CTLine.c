/*
 * CTLine.c — the line: one pass over a string, its glyphs, and where they go.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE LINE IS THE FIRST THING HERE THAT IS BUILT RATHER THAN STORED, and the whole of the build is three calls to
 * doors that already existed: `CTFontGetGlyphsForCharacters` turns the string's characters into glyphs — the door
 * this library was started for — `CTFontGetAdvancesForGlyphs` says how wide each one is, and the font's own ascent,
 * descent and leading are the line's heights. Everything else in this file is arithmetic over those, or a call to
 * Core Graphics to put the ink on a page.
 *
 * SO A LINE IS AS MANY RUNS AS ITS STRING HAS FACES, AND THE HEADER SAYS WHAT THAT STILL COSTS. No line breaking —
 * and **NO SHAPING YET**, which is the next milestone rather than a missing idea: one glyph per character, each
 * mapped through the face THAT CHARACTER is set in. THE ENGINE EXISTS (`CTShaping.c`, over HarfBuzz) and is measured,
 * and it is NOT CALLED FROM HERE, because wiring it in means changing the one-glyph-per-character correspondence that
 * this file's arrays, its caret doors AND the AppKit's glyph ranges all rely on — one change, with the shaper's
 * clusters as the new source of truth. Until that change nothing here is silently wrong, which is the point of
 * saying it.
 * **AND THE GLYPHS ARE REORDERED WHEN THE TEXT CALLS FOR IT**: the line runs the WHOLE bidirectional pipeline
 * (`CTBidi_internal.h`) over its own characters and, when the visual order is not the logical one, PLACES each glyph
 * where the reordering says — a Hebrew letter in a Latin paragraph is drawn to the RIGHT of the Latin one before it
 * — and the runs report `kCTRunStatusNonMonotonic`. THE CARET DOORS SPEAK VISUAL ORDER: the hit test walks the visual
 * slots, and an offset is the character's LEADING edge — its LEFT one at an even level and its RIGHT one at an odd
 * level. What is still owed is the WIRING of the shaping engine into this file — the engine itself is built
 * (`CTShaping.c`) and the slice that connects it is named in `README.md`.
 */
#include <CoreText/CTLine.h>

/* **AND THE RUN DELEGATE, WHICH IS HOW A CHARACTER THAT IS NOT A GLYPH GETS A WIDTH.** The key and the object are
 * Core Text's; `CTRunDelegate_internal.h` is this library's own seam to the three answers it asks. */
#include <CoreText/CTRunDelegate.h>
#include <CoreText/CTRunDelegate_internal.h>
#include <CoreText/CTTextTab.h>

/* THE FONT OBJECT AND ITS DOORS, WHICH THIS FILE'S HEADER DOES NOT DECLARE AND MUST NOT: Apple's CTLine.h does not
 * include CTFont.h either — a LINE's surface mentions no font at all. The IMPLEMENTATION is a different question,
 * and this is where it answers it. */
#include <CoreText/CTFont.h>
#include <CoreText/CTFont_internal.h>

/* AND THE RUN, WHICH A LINE IS MADE OF. `CTRun_internal.h` is NOT part of Core Text's surface — Apple has no public
 * creator for a run, because a line is what makes one — and it is included here so that the two files agree about
 * how a run is built instead of one of them `extern`ing what the other defines. */
#include <CoreText/CTRun_internal.h>

/* AND THIS FILE'S OWN INTERNAL HEADER, WHICH IS NOT A CIRCLE: it declares the two things a TYPESETTER needs from a
 * line — a line made over a RANGE, and the name of the font key — and including it here is what makes the
 * declarations and the definitions below the SAME functions rather than two files agreeing by convention. */
#include <CoreText/CTLine_internal.h>
#include <CoreText/CTBidi_internal.h>
#include <CoreText/CTBidi_internal.h>

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFRuntime.h>

#include <math.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
/* **ONE FONT RUN OF A LINE: the face, the characters it is set in, and where its glyphs begin.** A line is one or
 * more of these; a line with one face has EXACTLY one, which is why every door that read `line->font` still has an
 * answer. The fonts are retained by the line's `runFonts` array and BORROWED here. */
struct ct_line_run {
	CTFontRef font;
	CFIndex start;		/* character offset within the LINE's range */
	CFIndex count;
	CFIndex glyphStart;
	CGFloat ascent, descent, leading;
};

struct __CTLine {
	CFRuntimeBase base;
	CFAttributedStringRef string;	/* RETAINED: the string this line was made from */
	CTFontRef font;			/* RETAINED: the FIRST run's face, for the doors that want one */
	CFArrayRef runFonts;		/* RETAINED: one font per run */
	struct ct_line_run *runInfo;	/* malloc'd, runCount entries */
	CFIndex runCount;
	CFRange range;			/* the characters that spawned the glyphs */
	CGGlyph *glyphs;		/* malloc'd, glyphCount entries */
	CGPoint *positions;		/* malloc'd, glyphCount entries, relative to the LINE's own origin */
	/* **AND THE ADVANCES ARE KEPT WHEN THE LINE IS REORDERED**, because a glyph's advance is no longer the distance to
	 * the next POSITION — the next position is a different character. NULL for a line in logical order. */
	CGSize *advances;		/* malloc'd, glyphCount entries, or NULL */
	/* **AND THE VISUAL ORDER, WHEN IT IS NOT THE LOGICAL ONE**: `visualOrder[k]` is the logical index in visual slot
	 * `k`, which is what the hit test walks and what makes the run report `NonMonotonic`. NULL = logical order. */
	CFIndex *visualOrder;		/* malloc'd, glyphCount entries, or NULL */
	bool nonMonotonic;
	/* **AND THE RESOLVED LEVELS, WHEN THE PIPELINE RAN.** An ODD level is what makes a character's LEADING edge its
	 * RIGHT one — the caret convention the offset door owes — so the levels are kept rather than freed. NULL when the
	 * line never ran the pipeline, which means logical order and a left leading edge. */
	uint8_t *levels;		/* malloc'd, glyphCount entries, or NULL */
	/* **AND WHICH OF THE GLYPHS IS NOT A GLYPH BUT A BOX.** A character carrying a run delegate occupies the width its
	 * delegate answers and draws no ink; this flag is the one thing that tells the two apart, and it is carried
	 * alongside the glyphs so a DRAW can skip a box and the `CTRun` built from the line can too. */
	bool *isBox;			/* malloc'd, glyphCount entries; NULL when the line has no box */
	CFIndex glyphCount;
	double width;			/* the sum of the advances */
	CGFloat ascent, descent, leading;
	double trailingWhitespace;
	/* **AND THE DIRECTION THE PARAGRAPH RUNS IN**, read once at the line's first character and RESOLVED by P2/P3 when
	 * the caller left it natural: `kCTRunStatusRightToLeft` is what a run reports from it. */
	CTWritingDirection direction;
	/* AND ITS RUNS, BUILT ON FIRST ASK RATHER THAN AT CREATION: a line that is only measured never pays for a run, and a
	 * line whose runs are asked for pays for one per FONT run. `runs` is the ARRAY the door hands out, kept so that two
	 * calls give a caller the same object rather than two arrays holding the same runs. */
	CFArrayRef runs;		/* RETAINED once built: one array holding one run per font run */
};

static void ct_line_finalize(CFTypeRef cf)
{
	struct __CTLine *line = (struct __CTLine *)cf;

	if (line->string != NULL) {
		CFRelease(line->string);
	}
	if (line->font != NULL) {
		CFRelease(line->font);
	}
	if (line->runFonts != NULL) {
		CFRelease(line->runFonts);
	}
	free(line->runInfo);
	if (line->runs != NULL) {
		CFRelease(line->runs);
	}
	free(line->glyphs);
	free(line->positions);
	free(line->advances);
	free(line->visualOrder);
	free(line->levels);
	free(line->isBox);
}

/* TWO LINES ARE EQUAL WHEN THEY WERE MADE FROM THE SAME STRING, WITH THE SAME FACES OVER THE SAME RANGES, AND COVER
 * THE SAME CHARACTERS: the glyphs and their positions are a FUNCTION of those facts, so comparing them would be
 * comparing the same thing twice. **THE FACES ARE COMPARED AS THE WHOLE SEQUENCE AND NOT AS THE FIRST ONE** —
 * `runFonts` is one entry per font run, so two lines that share a first face but differ in a later one are two lines. */
static Boolean ct_line_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTLine *a = (const struct __CTLine *)cf1;
	const struct __CTLine *b = (const struct __CTLine *)cf2;

	if ((a->string == NULL) != (b->string == NULL) || (a->runFonts == NULL) != (b->runFonts == NULL)) {
		return false;
	}
	if (a->string != NULL && !CFEqual(a->string, b->string)) {
		return false;
	}
	if (a->runFonts != NULL && !CFEqual(a->runFonts, b->runFonts)) {
		return false;
	}
	return a->range.location == b->range.location && a->range.length == b->range.length;
}

static CFHashCode ct_line_hash(CFTypeRef cf)
{
	const struct __CTLine *line = (const struct __CTLine *)cf;
	CFHashCode h = 11u;

	if (line->string != NULL) {
		h = h * 31u + CFHash(line->string);
	}
	if (line->runFonts != NULL) {
		h = h * 31u + CFHash(line->runFonts);
	}
	return h * 31u + (CFHashCode)line->range.length;
}

static const CFRuntimeClass ct_line_class = {
	0,			/* version */
	"CTLine",		/* className */
	NULL,			/* init */
	NULL,			/* copy */
	ct_line_finalize,	/* finalize */
	ct_line_equal,		/* equal */
	ct_line_hash,		/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTLineGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_line_class);
	}
	return type_id;
}

/* =================================================================================================
 * THE BUILD. One pass over the string, one glyph per character, in EACH CHARACTER'S OWN FACE.
 * =============================================================================================== */
/* WHETHER A CHARACTER IS WHITESPACE, BY CORE FOUNDATION'S OWN ANSWER rather than by a list written here: the
 * trailing-whitespace door asks a typographic question and the character set is where the answer already lives. A
 * character set the platform cannot give up answers "not whitespace", which makes the door report NOTHING rather
 * than report something wrong. */
static bool ct_line_is_whitespace(UniChar ch)
{
	CFCharacterSetRef set = CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline);

	return set != NULL && CFCharacterSetIsCharacterMember(set, ch);
}

static struct __CTLine *ct_line_alloc(void)
{
	return (struct __CTLine *)_CFRuntimeCreateInstance(kCFAllocatorDefault, CTLineGetTypeID(),
		(CFIndex)(sizeof(struct __CTLine) - sizeof(CFRuntimeBase)), NULL);
}

/* **THE WHOLE BIDI PIPELINE FOR ONE LINE'S CHARACTERS, AND WHETHER IT MOVED ANYTHING.** P2/P3, the explicit levels, the
 * weak, bracket, neutral and implicit rules, then L1's reset and L2's reorder — the seven calls below are the whole of
 * `CTBidi_internal.h`, which is what that header was built for. The answer is `visualOrder` (visual slot -> logical
 * index), and the return says whether it is NOT the identity, which is what sets `NonMonotonic`. */
static bool fn_line_bidi_order(const UniChar *chars, CFIndex count, int paragraphLevel, CFIndex *visualOrder,
			       uint8_t *levels)
{
	CTBidiClass *classes;
	bool moved = false;
	CFIndex i;

	if (chars == NULL || visualOrder == NULL || levels == NULL || count <= 0) {
		return false;
	}
	classes = (CTBidiClass *)malloc((size_t)count * sizeof(CTBidiClass));
	if (classes == NULL) {
		return false;
	}
	for (i = 0; i < count; i++) {
		classes[i] = ct_bidi_class(chars[i]);
	}
	ct_bidi_explicit_levels(chars, count, paragraphLevel, levels, classes);
	ct_bidi_resolve_weak(classes, levels, count, paragraphLevel);
	ct_bidi_resolve_brackets(chars, classes, levels, count, paragraphLevel);
	ct_bidi_resolve_neutral(classes, levels, count, paragraphLevel);
	ct_bidi_resolve_implicit(classes, levels, count);
	ct_bidi_reset_levels(chars, levels, count, paragraphLevel);
	ct_bidi_reorder(levels, count, visualOrder);
	for (i = 0; i < count; i++) {
		if (visualOrder[i] != i) {
			moved = true;
			break;
		}
	}
		free(classes);
	return moved;
}

/* **THE NEUTRAL DIRECTIVE, DEFINED ONCE.** A line made by the whole-string door has no typesetter above it and
 * therefore no options, so it takes P2/P3's own answer and runs the whole algorithm — which is what this says. */
const CTLineBidiDirective CTLineBidiDirectiveNone = { -1, false };

/* **THE TAB STOPS A LINE OBEYS, TAKEN FROM ITS PARAGRAPH STYLE.** A tab is not a glyph and not a fixed box: where it
 * lands depends on where the pen ALREADY is, so the stop is chosen while the positions are being walked. Read once at
 * the line's own start, which is right for a single-paragraph line; a line spanning paragraphs with different tab
 * vocabularies is named as owed. */
/* **AND THE DIRECTION THE PARAGRAPH RUNS IN, WHICH THE RUN REPORTS.** The paragraph style's
 * `kCTParagraphStyleSpecifierBaseWritingDirection` is read at the line's FIRST character. A STATED direction is kept;
 * a NATURAL one is RESOLVED rather than guessed — UAX #9 P2 and P3 over the line's own characters — so a paragraph of
 * Hebrew runs right to left and one that starts with a Latin letter does not. See `CTRun.h`'s status flags; the
 * REORDERING is the next slice of the bidi milestone. */
static CTWritingDirection ct_line_base_direction(CFAttributedStringRef string, CFRange range,
						  CTLineBidiDirective bidi)
{
	CFTypeRef style;
	CTWritingDirection direction = kCTWritingDirectionNatural;

	/* **A FORCED LEVEL IS THE ANSWER, ABOVE EVEN THE PARAGRAPH STYLE**, which is what "forced" means: the option
	 * exists precisely to override the direction a document states. */
	if (bidi.forcedLevel == 0) {
		return kCTWritingDirectionLeftToRight;
	}
	if (bidi.forcedLevel == 1) {
		return kCTWritingDirectionRightToLeft;
	}
	if (string == NULL) {
		return kCTWritingDirectionLeftToRight;
	}
	style = CFAttributedStringGetAttribute(string, range.location, kCTParagraphStyleAttributeName, NULL);
	if (style != NULL && CFGetTypeID(style) == CTParagraphStyleGetTypeID()) {
		(void)CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style,
			kCTParagraphStyleSpecifierBaseWritingDirection, sizeof direction, &direction);
	}
	if (direction != kCTWritingDirectionNatural) {
		return direction;
	}
	/* **WITH BIDI PROCESSING OFF, A NATURAL DIRECTION CANNOT BE RESOLVED AND TAKES P3'S OWN DEFAULT.** The paragraph
	 * style's STATED direction is kept above, because it is a PROPERTY of the paragraph rather than a computation —
	 * what the option turns off is the algorithm. A NATURAL one has nothing left to read it with, so it is
	 * left-to-right, which is the answer P3 gives for a paragraph with nothing to look at. */
	if (bidi.disabled) {
		return kCTWritingDirectionLeftToRight;
	}
	/* P2/P3 DECIDE THE NATURAL CASE, over the characters the line covers. */
	if (range.length > 0) {
		UniChar *chars = (UniChar *)malloc((size_t)range.length * sizeof(UniChar));

		if (chars != NULL) {
			int level;

			CFStringGetCharacters(CFAttributedStringGetString(string), range, chars);
			level = ct_bidi_paragraph_level(chars, range.length, 0);
			free(chars);
			return (level == 1) ? kCTWritingDirectionRightToLeft : kCTWritingDirectionLeftToRight;
		}
	}
	/* NO CHARACTERS AT ALL IS LEFT TO RIGHT, which is P3's answer with nothing to read. */
	return kCTWritingDirectionLeftToRight;
}

static CFArrayRef ct_line_tab_stops(CFAttributedStringRef string, CFIndex index, double *interval)
{
	CFTypeRef style = CFAttributedStringGetAttribute(string, index, kCTParagraphStyleAttributeName, NULL);
	CFArrayRef tabs = NULL;

	*interval = 0.0;
	if (style == NULL || CFGetTypeID(style) != CTParagraphStyleGetTypeID()) {
		return NULL;
	}
	(void)CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style,
			kCTParagraphStyleSpecifierTabStops, sizeof tabs, &tabs);
	(void)CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style,
			kCTParagraphStyleSpecifierDefaultTabInterval, sizeof *interval, interval);
	return tabs;
}

/* THE FIRST STOP PAST THE PEN, OR THE NEXT MULTIPLE OF THE DEFAULT INTERVAL when the list has run out — and when
 * neither exists the tab is zero-width rather than a guess. */
static double ct_line_next_tab_stop(CFArrayRef tabs, CFIndex count, double interval, double x)
{
	CFIndex i;

	for (i = 0; tabs != NULL && i < count; i++) {
		CTTextTabRef tab = (CTTextTabRef)CFArrayGetValueAtIndex(tabs, i);
		double location = (tab != NULL) ? CTTextTabGetLocation(tab) : 0.0;

		if (location > x) {
			return location;
		}
	}
	if (interval > 0.0) {
		return (floor(x / interval) + 1.0) * interval;
	}
	return x;
}

/* **THE FONT RUNS OF A LINE'S RANGE.** A run is a maximal span whose `kCTFontAttributeName` is the SAME font — `CFEqual`
 * compares two equal faces as one run — and a character with NO font attribute falls back to the face the caller passed,
 * which is what a line built from an attribution with no font at all is made of. `runs` must hold `range.length`
 * entries; `fonts` collects each run's face, which the runs BORROW. */
static CFIndex fn_font_runs(CFAttributedStringRef string, CFRange range, CTFontRef fallback, CFMutableArrayRef fonts,
			    struct ct_line_run *runs)
{
	CFIndex runCount = 0;
	CFIndex pos = 0;

	while (pos < range.length) {
		CFRange effective = CFRangeMake(0, 0);
		CFRange limit = CFRangeMake(range.location, range.length);
		/* **THE LONGEST RANGE WITH THE SAME FACE**, NOT the effective range of the RUN the character sits in: a run says
		 * where the ATTRIBUTE DICTIONARY changes, and a line's font runs must be where the FONT changes — a string that
		 * sets a run delegate or a tab attribute in the middle would otherwise split into runs that share one face. */
		CTFontRef font = (CTFontRef)CFAttributedStringGetAttributeAndLongestEffectiveRange(string,
				range.location + pos, kCTFontAttributeName, limit, &effective);
		CFIndex end;

		if (font == NULL || CFGetTypeID(font) != CTFontGetTypeID()) {
			font = fallback;
		}
		end = effective.location + effective.length - range.location;
		if (end > range.length) {
			end = range.length;
		}
		if (end <= pos) {
			end = pos + 1;
		}
		runs[runCount].font = font;
		runs[runCount].start = pos;
		runs[runCount].count = end - pos;
		runs[runCount].glyphStart = pos;
		runs[runCount].ascent = CTFontGetAscent(font);
		runs[runCount].descent = CTFontGetDescent(font);
		runs[runCount].leading = CTFontGetLeading(font);
		CFArrayAppendValue(fonts, font);
		runCount++;
		pos = end;
	}
	return runCount;
}

/* THE RUN A CHARACTER BELONGS TO, by binary search over runs that are CONTIGUOUS and in order. */
static CFIndex fn_run_index(const struct __CTLine *line, CFIndex charIndex)
{
	CFIndex lo = 0;
	CFIndex hi = line->runCount;

	while (lo < hi) {
		CFIndex mid = lo + (hi - lo) / 2;

		if (charIndex < line->runInfo[mid].start) {
			hi = mid;
		} else if (charIndex >= line->runInfo[mid].start + line->runInfo[mid].count) {
			lo = mid + 1;
		} else {
			return mid;
		}
	}
	return (line->runCount > 0) ? line->runCount - 1 : 0;
}

/* AND THE FACE OF THE RUN A CHARACTER BELONGS TO, for a kerning pair and for the draw. */
static CTFontRef fn_run_font(const struct __CTLine *line, CFIndex charIndex)
{
	if (line->runCount == 0) {
		return line->font;
	}
	return line->runInfo[fn_run_index(line, charIndex)].font;
}

/* THE ONE PLACE A LINE IS MADE, AND IT IS ALWAYS MADE OVER A RANGE. A whole-string line and a typeset line differ in
 * ONE number — where the glyphs begin — so they are one function with the range as a parameter rather than two
 * functions that could drift apart. `font` is BORROWED by the callers and RETAINED here. */
static CTLineRef ct_line_create_with_font(CFAttributedStringRef string, CTFontRef font, CFRange range,
					    CTLineBidiDirective bidi)
{
	struct __CTLine *line;
	CFStringRef text;
	UniChar *chars;
	CGSize *advances;
	CFIndex n, i;
	double x = 0.0;
	CFArrayRef tabs;
	CFIndex tabCount;
	double tabInterval = 0.0;
	uint8_t *levels = NULL;
	CFIndex *visualOrder = NULL;
	bool moved = false;

	text = CFAttributedStringGetString(string);
	n = range.length;
	line = ct_line_alloc();
	if (line == NULL) {
		return NULL;
	}
	line->string = (CFAttributedStringRef)CFRetain(string);
	/* THE RANGE IS THE STRING'S OWN AND STAYS THE STRING'S OWN, which is the whole point of the parameter: a line cut
	 * out of the MIDDLE of a document reports the range it covers THERE, so a caller laying out line after line never
	 * has to translate between the line's coordinates and the string's. */
	line->range = range;
	line->direction = ct_line_base_direction(string, range, bidi);
	/* **THE FONT RUNS, WHICH IS WHAT MAKES A LINE MORE THAN ONE FACE.** */
	line->runFonts = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	line->runInfo = (struct ct_line_run *)malloc((size_t)((range.length > 0) ? range.length : 1)
						     * sizeof(struct ct_line_run));
	if (line->runFonts == NULL || line->runInfo == NULL) {
		CFRelease((CFTypeRef)line);	/* the finalizer frees whichever arrived */
		return NULL;
	}
	line->runCount = fn_font_runs(string, range, font, (CFMutableArrayRef)line->runFonts, line->runInfo);
	line->font = (CTFontRef)CFRetain((line->runCount > 0) ? line->runInfo[0].font : font);
	/* THE HEIGHTS ARE THE RUNS' UNION — the tallest ascent, the deepest descent — which is what a framesetter needs:
	 * line after line placed without measuring glyph by glyph. They are read even for a line of nothing. */
	if (line->runCount == 0) {
		line->ascent = CTFontGetAscent(font);
		line->descent = CTFontGetDescent(font);
		line->leading = CTFontGetLeading(font);
	} else {
		CFIndex r;

		line->ascent = line->runInfo[0].ascent;
		line->descent = line->runInfo[0].descent;
		line->leading = line->runInfo[0].leading;
		for (r = 1; r < line->runCount; r++) {
			if (line->runInfo[r].ascent > line->ascent) line->ascent = line->runInfo[r].ascent;
			if (line->runInfo[r].descent > line->descent) line->descent = line->runInfo[r].descent;
			if (line->runInfo[r].leading > line->leading) line->leading = line->runInfo[r].leading;
		}
	}
	tabs = ct_line_tab_stops(string, range.location, &tabInterval);
	tabCount = (tabs != NULL) ? CFArrayGetCount(tabs) : 0;
	if (n <= 0) {
		return (CTLineRef)line;
	}

	chars = (UniChar *)malloc((size_t)n * sizeof(UniChar));
	line->glyphs = (CGGlyph *)malloc((size_t)n * sizeof(CGGlyph));
	line->positions = (CGPoint *)malloc((size_t)n * sizeof(CGPoint));
	advances = (CGSize *)malloc((size_t)n * sizeof(CGSize));
	line->isBox = (bool *)calloc((size_t)n, sizeof(bool));
	if (chars == NULL || line->glyphs == NULL || line->positions == NULL || advances == NULL || line->isBox == NULL) {
		free(chars);
		free(advances);
		/* THE LINE IS RELEASED RATHER THAN FIXED UP: it is a fresh Core Foundation object with one reference, so
		 * releasing it runs the finalizer, which frees whichever arrays DID arrive — one place that knows what a
		 * line owns, instead of a second list of frees here. */
		CFRelease((CFTypeRef)line);
		return NULL;
	}
	CFStringGetCharacters(text, range, chars);

	/* **AND THE BIDI PIPELINE RUNS HERE, BEFORE THE GLYPHS ARE LOOKED UP**, because L4 MIRRORS a character whose level
	 * is odd and the MIRRORED character is the one the font has to be asked for: `(` in Arabic text is `)`. The visual
	 * order is kept for the placement at the END of this function; `levels` is only needed for the mirroring. */
	if (bidi.disabled) {
		/* **NOTHING RUNS AND NOTHING MOVES, AND THE ARRAYS ARE NOT EVEN ALLOCATED.** A NULL `levels` and a NULL
		 * `visualOrder` are already this function's spelling for "logical order" — the placement at the end and the
		 * caret doors both read them that way — so bidi processing turned off needs no second path through the rest of
		 * the function. What that means concretely is measured in the suite: the glyphs keep their string order, the run
		 * does not report `kCTRunStatusNonMonotonic`, and L4's MIRRORING does not happen either, so a `(` in Hebrew text
		 * is still the font's `(`. */
	} else {
		int paragraphLevel = (line->direction == kCTWritingDirectionRightToLeft) ? 1 : 0;

		levels = (uint8_t *)malloc((size_t)n * sizeof(uint8_t));
		visualOrder = (CFIndex *)malloc((size_t)n * sizeof(CFIndex));
		if (levels != NULL && visualOrder != NULL) {
			moved = fn_line_bidi_order(chars, n, paragraphLevel, visualOrder, levels);
			ct_bidi_mirror(chars, n, levels);
		} else {
			free(levels);
			free(visualOrder);
			levels = NULL;
			visualOrder = NULL;
		}
	}

	/* **EACH RUN'S GLYPHS ARE INDEXED IN ITS OWN FONT, WHICH IS THE WHOLE POINT OF A RUN.** Apple's contract for the
	 * glyph step is a BOOLEAN and a line does not need it: a character the font cannot encode becomes glyph 0, which IS
	 * `.notdef`, and Apple DRAWS that rather than dropping it. */
	memset(advances, 0, (size_t)n * sizeof(CGSize));
	{
		CFIndex r;

		for (r = 0; r < line->runCount; r++) {
			struct ct_line_run *run = &line->runInfo[r];

			(void)CTFontGetGlyphsForCharacters(run->font, chars + run->start, line->glyphs + run->start,
							   run->count);
			(void)CTFontGetAdvancesForGlyphs(run->font, kCTFontHorizontalOrientation,
							 line->glyphs + run->start, advances + run->start, run->count);
		}
	}
	/* **AND A CHARACTER CARRYING A RUN DELEGATE IS A BOX, NOT A GLYPH.** Its advance is what the delegate answers rather
	 * than what the placeholder glyph measured, no ink is drawn for it, and the line grows to cover the delegate's
	 * ascent and descent — so a picture taller than the font's line makes the line taller. THE KERN IS SKIPPED AROUND A
	 * BOX in the position loop below, because there is no pair of glyphs to adjust. */
	for (i = 0; i < n; i++) {
		CFTypeRef value = CFAttributedStringGetAttribute(string, range.location + i,
								 kCTRunDelegateAttributeName, NULL);

		if (value != NULL && CFGetTypeID(value) == CTRunDelegateGetTypeID()) {
			CTRunDelegateRef delegate = (CTRunDelegateRef)value;
			CGFloat boxAscent = CTRunDelegateAscent(delegate);
			CGFloat boxDescent = CTRunDelegateDescent(delegate);

			line->isBox[i] = true;
			line->glyphs[i] = 0;
			advances[i] = CGSizeMake(CTRunDelegateWidth(delegate), 0.0);
			if (boxAscent > line->ascent) {
				line->ascent = boxAscent;
			}
			if (boxDescent > line->descent) {
				line->descent = boxDescent;
			}
		}
	}
	/* **AND THE PAIR'S OWN ADJUSTMENT AFTER EACH GLYPH, WHICH IS WHAT KERNING IS.** An advance is what a glyph
	 * asks for on its own; a KERN is what the font's typographer said about the two glyphs TOGETHER — that an `o`
	 * belongs closer under a `T` than their advances put it — and a line that sums advances and stops has thrown
	 * that opinion away. MEASURED, the face this desktop draws with asks for `To` to close by 348 units of 2048:
	 * an em's sixth, TWO PIXELS of daylight at 13 points, which is the width of a stem.
	 *
	 * IT IS APPLIED TO THE LINE'S OWN WIDTH AS WELL, because it is applied to `x` — so a line measures and draws
	 * the same string, and every door that reads a position (`CTLineGetOffsetForStringIndex` and the hit test
	 * above it) is reading the kerned ones because the kern is already inside the difference between two
	 * positions. */
	for (i = 0; i < n; i++) {
		line->positions[i] = CGPointMake((CGFloat)x, 0.0);
		/* **AND A TAB ADVANCES TO ITS STOP, WHICH DEPENDS ON WHERE THE PEN ALREADY IS.** It is a BOX — no ink — whose
		 * width is the distance from here to the next stop, so the two halves of a document line up in columns the way a
		 * typewriter's did. A tab with no stop ahead and no default interval is zero-width, which is the honest answer. */
		if (chars[i] == '\t') {
			double stop = ct_line_next_tab_stop(tabs, tabCount, tabInterval, (double)x);

			line->isBox[i] = true;
			line->glyphs[i] = 0;
			advances[i].width = (CGFloat)(stop - (double)x);
			if (advances[i].width < 0.0) {
				advances[i].width = 0.0;
			}
		}
		x += advances[i].width;
		/* **A KERN IS A PAIR IN ONE FACE**: two glyphs from DIFFERENT runs have no shared typographer, so the pair
		 * across a run boundary is not asked about. */
		if (i + 1 < n && !line->isBox[i] && !line->isBox[i + 1]
		    && fn_run_index(line, i) == fn_run_index(line, i + 1)) {
			x += (CGFloat)CTFontGetKerningBetweenGlyphs(fn_run_font(line, i), line->glyphs[i],
								    line->glyphs[i + 1]);
		}
	}
	line->glyphCount = n;
	line->width = x;
	/* THE TRAILING WHITESPACE IS MEASURED BACKWARDS OVER THE CHARACTERS THIS LINE HAS, because that is what the door
	 * is about: the part of the line's width that is invisible at its end. */
	for (i = n - 1; i >= 0 && ct_line_is_whitespace(chars[i]); i--) {
		line->trailingWhitespace += advances[i].width;
	}
	/* **AND THE GLYPHS ARE PLACED IN THE VISUAL ORDER THE PIPELINE IMPLIES.** The advances are KEPT — a reordered
	 * glyph's advance is not recoverable from its neighbours' positions — and when the order is not the identity the
	 * positions are REWRITTEN so that the glyphs run in visual order, which is what the draw reads: a glyph array
	 * indexed by LOGICAL character with a position that says where it GOES. Kerning is taken between VISUAL
	 * neighbours, because that is the pair the eye sees. */
	line->advances = advances;
	if (visualOrder != NULL && moved) {
		CGPoint *placed = (CGPoint *)malloc((size_t)n * sizeof(CGPoint));

		if (placed != NULL) {
			double placedX = 0.0;
			CFIndex k;

			for (k = 0; k < n; k++) {
				CFIndex logical = visualOrder[k];

				placed[logical] = CGPointMake((CGFloat)placedX, 0.0);
				placedX += advances[logical].width;
				if (k + 1 < n && !line->isBox[logical] && !line->isBox[visualOrder[k + 1]]
				    && fn_run_index(line, logical) == fn_run_index(line, visualOrder[k + 1])) {
					placedX += (CGFloat)CTFontGetKerningBetweenGlyphs(fn_run_font(line, logical),
										line->glyphs[logical],
										line->glyphs[visualOrder[k + 1]]);
				}
			}
			for (k = 0; k < n; k++) {
				line->positions[k] = placed[k];
			}
			line->width = placedX;
			line->visualOrder = visualOrder;
			line->nonMonotonic = true;
			visualOrder = NULL;
			free(placed);
		}
	}
	line->levels = levels;	/* the caret doors read it; NULL when the pipeline could not run */
	free(visualOrder);
	free(chars);
	return (CTLineRef)line;
}

/* =================================================================================================
 * CREATION.
 * =============================================================================================== */
/* THE FACE OUT OF AN ATTRIBUTED STRING, AND ONE OF THE TWO DOORS THAT NAMED A KEY BEFORE THE KEY EXISTED. This used
 * to return a literal — "NSFontAttributeName" — with the reason written down: the constant lives in
 * `CTStringAttributes.h`, which was later in the include order than this file, so the value was carried here as a
 * recorded boundary and the comment said the function would go the day that header landed. IT HAS LANDED, and what
 * the function returns is now `kCTFontAttributeName`: ONE COPY OF THE VALUE, in the header that owns it, and no
 * inference in this file to keep in step.
 *
 * AND ITS VALUE IS NO LONGER AN INFERENCE EITHER: `CTStringAttributes.c` records the measurement — Apple's
 * `NSAttributedString.h` declares `NSFontAttributeName`, and a dictionary crosses between the two frameworks — so
 * this is the PROVEN key it always should have been. */
CFStringRef CTLineFontAttributeName(void)
{
	return kCTFontAttributeName;
}

CTLineRef CTLineCreateWithAttributedString(CFAttributedStringRef string)
{
	CFTypeRef value;
	CFRange effective = CFRangeMake(0, 0);
	CFRange whole;

	if (string == NULL) {
		return NULL;
	}
	/* THE FONT IS AN ATTRIBUTE, AND A LINE WITH NO FONT IS NOT A LINE. Apple gives this door no default face to
	 * fall back to — the caller supplies one AS AN ATTRIBUTE — and substituting a face nobody asked for is the
	 * inventing this library refuses. An attribute of the wrong type is treated the same way: a `CFString` where a
	 * font belongs is not a font.
	 *
	 * AND A ZERO-LENGTH STRING ANSWERS NULL for the same reason: there is no character to carry the attribute, so
	 * there is no font to be found. Recorded because it is a case a caller WILL hit — an editor asks for the line
	 * of an empty document — and the answer is "there is nothing to lay out", which NULL says. */
	value = CFAttributedStringGetAttribute(string, 0, CTLineFontAttributeName(), &effective);
	if (value == NULL || CFGetTypeID(value) != CTFontGetTypeID()) {
		return NULL;
	}
	whole = CFRangeMake(0, CFStringGetLength(CFAttributedStringGetString(string)));
	return ct_line_create_with_font(string, (CTFontRef)value, whole, CTLineBidiDirectiveNone);
}

/* AND THE SECOND WAY IN, WHICH NO PUBLIC HEADER NAMES: a TYPESETTER makes lines out of PARTS of a string, and there is
 * no public door for that — so the two files agree about it in `CTLine_internal.h`, where its reason is written down.
 * The range is VALIDATED here rather than trusted: the callers are inside this library, but the bounds are still a
 * fact about the string, and a door that can be reached with a bad range must refuse rather than read past the end. */
CTLineRef CTLineCreateWithAttributedStringAndRange(CFAttributedStringRef string, CFRange range)
{
	return CTLineCreateWithAttributedStringAndRangeWithBidi(string, range, CTLineBidiDirectiveNone);
}

/* **AND THE SAME DOOR WITH THE TYPESETTER'S DIRECTIVES**, which is the only difference between a line made by a
 * caller and one made by a typesetter that was given `kCTTypesetterOptionDisableBidiProcessing` or
 * `kCTTypesetterOptionForcedEmbeddingLevel`. EVERYTHING ELSE IS SHARED, so the two doors cannot disagree about what a
 * line is. */
CTLineRef CTLineCreateWithAttributedStringAndRangeWithBidi(CFAttributedStringRef string, CFRange range,
							  CTLineBidiDirective bidi)
{
	CFTypeRef value;
	CFIndex length;

	if (string == NULL) {
		return NULL;
	}
	length = CFStringGetLength(CFAttributedStringGetString(string));
	if (range.location < 0 || range.length < 0 || range.location > length ||
	    range.location + range.length > length) {
		return NULL;
	}
	value = CFAttributedStringGetAttribute(string, range.location, CTLineFontAttributeName(), NULL);
	if (value == NULL || CFGetTypeID(value) != CTFontGetTypeID()) {
		return NULL;
	}
	return ct_line_create_with_font(string, (CTFontRef)value, range, bidi);
}

/* =================================================================================================
 * WHAT A LINE SAYS ABOUT ITSELF.
 * =============================================================================================== */
CFIndex CTLineGetGlyphCount(CTLineRef line)
{
	const struct __CTLine *l = (const struct __CTLine *)line;

	return (l == NULL) ? 0 : l->glyphCount;
}

CFRange CTLineGetStringRange(CTLineRef line)
{
	const struct __CTLine *l = (const struct __CTLine *)line;

	return (l == NULL) ? CFRangeMake(0, 0) : l->range;
}

/* =================================================================================================
 * MEASUREMENT.
 * =============================================================================================== */
double CTLineGetTypographicBounds(CTLineRef line, CGFloat *ascent, CGFloat *descent, CGFloat *leading)
{
	const struct __CTLine *l = (const struct __CTLine *)line;

	if (l == NULL) {
		/* APPLE'S OWN FAILURE ANSWER IS ZERO, and nothing is written to the out-parameters: a caller that reads an
		 * ascent after a failure reads its own variable, which is the same contract the paragraph style's buffer
		 * has. */
		return 0.0;
	}
	if (ascent != NULL) {
		*ascent = l->ascent;
	}
	if (descent != NULL) {
		*descent = l->descent;
	}
	if (leading != NULL) {
		*leading = l->leading;
	}
	return l->width;
}

double CTLineGetTrailingWhitespaceWidth(CTLineRef line)
{
	const struct __CTLine *l = (const struct __CTLine *)line;

	return (l == NULL) ? 0.0 : l->trailingWhitespace;
}

/* THE INK, AND IT IS THE UNION OF THE GLYPHS' OWN BOXES EACH MOVED TO WHERE ITS GLYPH IS DRAWN. The font gives a
 * box per glyph relative to the drawing origin, so the line's positions are added here — the font's door does NOT
 * do it, because a font does not know where a line puts anything. THEN THE CONTEXT'S TEXT POSITION IS ADDED TOO,
 * which Apple's own sentence requires: the rectangle is "translated by the supplied context's text position".
 *
 * A LINE WITH NO GLYPHS HAS NO INK, and `CGRectNull` is the answer — the same answer an invalid line gets, and the
 * same convention `CTFontGetBoundingBox` uses for a font it cannot measure. */
CGRect CTLineGetImageBounds(CTLineRef line, CGContextRef context)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	CGRect *boxes;
	CGRect ink = CGRectNull;
	CGPoint pen;
	CFIndex i;

	if (l == NULL || context == NULL || l->glyphCount <= 0) {
		return CGRectNull;
	}
	boxes = (CGRect *)malloc((size_t)l->glyphCount * sizeof(CGRect));
	if (boxes == NULL) {
		return CGRectNull;
	}
	/* THE FONT'S DOOR DOES NOT APPLY THE POSITIONS, so its per-glyph boxes are asked for and moved here; the union
	 * it also returns is ignored, because a union of boxes that all sit at the origin is not the ink. */
	(void)CTFontGetBoundingRectsForGlyphs(l->font, kCTFontHorizontalOrientation, l->glyphs, boxes,
					      l->glyphCount);
	pen = CGContextGetTextPosition(context);
	for (i = 0; i < l->glyphCount; i++) {
		CGRect box = boxes[i];

		box.origin.x += l->positions[i].x + pen.x;
		box.origin.y += l->positions[i].y + pen.y;
		ink = CGRectIsNull(ink) ? box : CGRectUnion(ink, box);
	}
	free(boxes);
	return ink;
}

double CTLineGetPenOffsetForFlush(CTLineRef line, CGFloat flushFactor, double flushWidth)
{
	const struct __CTLine *l = (const struct __CTLine *)line;

	if (l == NULL) {
		return 0.0;
	}
	/* APPLE'S OWN WORDS FOR THE FACTOR ARE THE CLAMP: 0 "or less" is left flush and 1.0 "or more" is right, so the
	 * factor is held between them and the offset is what is left of the width, times it. */
	if (flushFactor < 0.0) {
		flushFactor = 0.0;
	}
	if (flushFactor > 1.0) {
		flushFactor = 1.0;
	}
	return (double)flushFactor * (flushWidth - l->width);
}

/* =================================================================================================
 * DRAWING.
 * =============================================================================================== */
void CTLineDraw(CTLineRef line, CGContextRef context)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	CGPoint pen;
	CFIndex r;

	if (l == NULL || context == NULL || l->glyphCount <= 0) {
		return;
	}
	/* THE LINE'S ORIGIN IS THE CONTEXT'S TEXT POSITION, AND APPLYING IT IS THIS DOOR'S JOB BECAUSE *THIS* CORE
	 * GRAPHICS DOES NOT: `CGContextShowGlyphsAtPositions` takes positions that are "specified in user space" and says
	 * in its own header that "the CTM alone places the pen" — UNLIKE the three advance doors, whose pen "starts at
	 * the TEXT POSITION". So the pen offset is added here, and the suite MEASURES it by drawing the same line at two
	 * text positions and looking at where the ink landed: a comment cannot tell those two readings apart, and a
	 * picture can. */
	pen = CGContextGetTextPosition(context);
	/* **EACH RUN IS DRAWN IN ITS OWN FACE**, because a glyph index means nothing without the face it was indexed in.
	 * A BOX IS SKIPPED: it occupies space and draws no ink, and `CGContextShowGlyphsAtPositions` takes one contiguous
	 * run, so the boxes come out once per font run. */
	for (r = 0; r < ((l->runCount > 0) ? l->runCount : 1); r++) {
		CTFontRef runFont = (l->runCount > 0) ? l->runInfo[r].font : l->font;
		CFIndex start = (l->runCount > 0) ? l->runInfo[r].start : 0;
		CFIndex count = (l->runCount > 0) ? l->runInfo[r].count : l->glyphCount;
		CGFontRef graphics = CTFontCopyGraphicsFont(runFont, NULL);
		CGGlyph *drawGlyphs;
		CGPoint *drawPositions;
		CFIndex drawCount = 0;
		CFIndex i;

		if (graphics == NULL) {
			continue;
		}
		drawGlyphs = (CGGlyph *)malloc((size_t)((count > 0) ? count : 1) * sizeof(CGGlyph));
		drawPositions = (CGPoint *)malloc((size_t)((count > 0) ? count : 1) * sizeof(CGPoint));
		if (drawGlyphs == NULL || drawPositions == NULL) {
			free(drawGlyphs);
			free(drawPositions);
			CGFontRelease(graphics);
			continue;
		}
		for (i = 0; i < count; i++) {
			if (l->isBox != NULL && l->isBox[start + i]) {
				continue;
			}
			drawGlyphs[drawCount] = l->glyphs[start + i];
			drawPositions[drawCount] = CGPointMake(l->positions[start + i].x + pen.x,
							       l->positions[start + i].y + pen.y);
			drawCount++;
		}
		/* THE FONT AND ITS SIZE GO ON THE CONTEXT because the glyphs are drawn BY Core Graphics and not by this
		 * library: a glyph index means nothing without the face and the size it was indexed in. */
		if (drawCount > 0) {
			CGContextSetFont(context, graphics);
			CGContextSetFontSize(context, CTFontGetSize(runFont));
			CGContextShowGlyphsAtPositions(context, drawGlyphs, drawPositions, (size_t)drawCount);
		}
		free(drawGlyphs);
		free(drawPositions);
		CGFontRelease(graphics);
	}
}

/* =================================================================================================
 * CARET POSITIONING.
 * =============================================================================================== */
/* A GLYPH'S OWN WIDTH, RECOVERED FROM THE POSITIONS THEMSELVES: it is the distance to the next glyph, and the last
 * one's is what is left of the width. One function because BOTH caret doors need it, and a second copy of the rule
 * is a second chance to get it wrong. */
static CGFloat ct_line_advance_at(const struct __CTLine *line, CFIndex i)
{
	/* **A REORDERED LINE KEEPS ITS ADVANCES**, because the next POSITION is a different character; a line in LOGICAL
	 * order still derives the advance from the positions, which is what the constructors that place glyphs directly
	 * (a truncated or a justified line) rely on. */
	if (line->advances != NULL) {
		return line->advances[i].width;
	}
	if (i + 1 < line->glyphCount) {
		return line->positions[i + 1].x - line->positions[i].x;
	}
	return (CGFloat)(line->width - line->positions[i].x);
}

CFIndex CTLineGetStringIndexForPosition(CTLineRef line, CGPoint position)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	CFIndex i;

	if (l == NULL) {
		return kCFNotFound;
	}
	/* **A REORDERED LINE IS WALKED IN VISUAL ORDER**, because its glyphs are no longer in string order: the slot whose
	 * box holds the point answers with ITS logical index, and one past the last slot is the door's own upper bound. */
	if (l->visualOrder != NULL) {
		CFIndex k;

		for (k = 0; k < l->glyphCount; k++) {
			CFIndex logical = l->visualOrder[k];
			CGFloat advance = ct_line_advance_at(l, logical);
			CGFloat midpoint = l->positions[logical].x + advance / 2.0f;
			bool rtl = (l->levels != NULL) && ((l->levels[logical] % 2) != 0);

			/* **THE HALF THE POINT FELL IN DECIDES WHICH SIDE OF THE CHARACTER THE CARET IS ON**, and which logical
			 * index that is depends on the character's OWN direction: a right-to-left glyph's caret-before is on its
			 * RIGHT, so its left half belongs to the character AFTER it. */
			if (position.x < midpoint) {
				CFIndex answer = rtl ? (logical + 1) : logical;

				return (answer > l->glyphCount) ? l->glyphCount : answer;
			}
			if (position.x < l->positions[logical].x + advance) {
				CFIndex answer = rtl ? logical : (logical + 1);

				return (answer > l->glyphCount) ? l->glyphCount : answer;
			}
		}
		return l->glyphCount;
	}
	/* THE NEAREST BOUNDARY WINS: a click belongs to the character whose half of the advance it fell in. Before the
	 * first glyph is index 0 and past the last is one past the last character, which is Apple's own bound for this
	 * door ("no less than the first string index and no greater than one plus the last string index"). */
	for (i = 0; i < l->glyphCount; i++) {
		CGFloat midpoint = l->positions[i].x + ct_line_advance_at(l, i) / 2.0f;

		if (position.x < midpoint) {
			return i;
		}
	}
	return l->glyphCount;
}

CGFloat CTLineGetOffsetForStringIndex(CTLineRef line, CFIndex charIndex, CGFloat *secondaryOffset)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	CGFloat offset;

	if (l == NULL) {
		if (secondaryOffset != NULL) {
			*secondaryOffset = 0.0;
		}
		return 0.0;
	}
	if (charIndex < 0) {
		offset = 0.0;
	} else if (l->levels == NULL) {
		/* **A LINE THAT NEVER RAN THE PIPELINE IS IN LOGICAL ORDER**, so its leading edge IS its left one and the end of
		 * the line is its width — the answer this door has always given. */
		offset = (charIndex >= l->glyphCount) ? (CGFloat)l->width : l->positions[charIndex].x;
	} else if (charIndex < l->glyphCount) {
		/* **APPLE'S LEADING EDGE, WHICH IS A SIDE AND NOT AN x.** A LEFT-TO-RIGHT character's leading edge is its left
		 * one; a RIGHT-TO-LEFT character's is its RIGHT one, because that is the side the caret sits on when the
		 * character is about to be typed. */
		CGFloat advance = ct_line_advance_at(l, charIndex);

		offset = l->positions[charIndex].x + (((l->levels[charIndex] % 2) != 0) ? advance : 0.0);
	} else {
		/* AND ONE PAST THE LAST CHARACTER IS ITS TRAILING EDGE. */
		CFIndex last = l->glyphCount - 1;

		if (last >= 0 && (l->levels[last] % 2) != 0) {
			offset = l->positions[last].x;
		} else {
			offset = (CGFloat)l->width;
		}
	}
	/* A SINGLE RUN HAS ONE CARET PER BOUNDARY, so the secondary offset is the same as the primary — which is
	 * exactly the case Apple's sentence describes for it. */
	if (secondaryOffset != NULL) {
		*secondaryOffset = offset;
	}
	return offset;
}

/* =================================================================================================
 * THE RUNS, WHICH IS WHERE THIS TYPE MEETS `CTRun`.
 * =============================================================================================== */
/* ONE ARRAY, ONE RUN PER FONT RUN, BUILT ON FIRST ASK AND KEPT. APPLE'S NAME FOR THIS DOOR IS `Get` AND NOT `Copy`,
 * and its documentation says nothing about releasing — so what comes back is BORROWED, owned by the line, which is the
 * reading the name and the silence agree on. The array is built once and cached for exactly that reason: two calls
 * hand a caller the SAME object rather than two arrays holding the same runs.
 *
 * AND A RUN IS WHERE THE FACE CHANGES. A run is, in CTRun.h's own words, "the piece of a line that shares one set of
 * attributes", and the attribute this tier partitions on is the FONT: a string that sets a run delegate or a tab in
 * the middle is still ONE font run, and a string that changes face in the middle is two. Each run carries its own
 * face, its own slice of the string, and glyphs INDEXED IN THAT FACE — which is the whole difference, because a glyph
 * index means nothing without the face it came from, so a second face cannot be drawn with the first face's indices.
 *
 * THE ADVANCES COME FROM THE LINE'S OWN ARRAY WHEN IT KEEPS ONE, which a REORDERED line does because its next
 * POSITION is a different character; otherwise they are recovered from the positions, which is the same rule the caret
 * doors already use, so it is taken from that one place rather than written a second time. */
CFArrayRef CTLineGetGlyphRuns(CTLineRef line)
{
	struct __CTLine *l = (struct __CTLine *)line;
	CTRunStatus status;
	CFMutableArrayRef runs;
	CFIndex r;

	if (l == NULL) {
		return NULL;
	}
	if (l->runs != NULL) {
		return l->runs;
	}
	/* **ONE RUN PER FONT RUN.** The string indices are the run's fact and are built here: one glyph per character,
	 * RELATIVE TO THE LINE, so a run's index array is its own slice of the line's. The attributes are the string's own
	 * for the run's range — Apple's sentence allows either that or a dictionary "manufactured by the layout engine", and
	 * this tier has nothing to manufacture: the font came from these very attributes. */
	status = ((l->direction == kCTWritingDirectionRightToLeft) ? kCTRunStatusRightToLeft : kCTRunStatusNoStatus)
	       | (l->nonMonotonic ? kCTRunStatusNonMonotonic : 0);
	runs = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	if (runs == NULL) {
		return NULL;
	}
	for (r = 0; r < ((l->runCount > 0) ? l->runCount : 1); r++) {
		CFIndex start = (l->runCount > 0) ? l->runInfo[r].start : 0;
		CFIndex count = (l->runCount > 0) ? l->runInfo[r].count : l->glyphCount;
		CTFontRef font = (l->runCount > 0) ? l->runInfo[r].font : l->font;
		CFDictionaryRef attributes;
		CFIndex *indices;
		CGSize *advances;
		CTRunRef created;
		CFIndex k;

		indices = (CFIndex *)malloc((size_t)((count > 0) ? count : 1) * sizeof(CFIndex));
		advances = (CGSize *)malloc((size_t)((count > 0) ? count : 1) * sizeof(CGSize));
		if (indices == NULL || advances == NULL) {
			free(indices);
			free(advances);
			CFRelease(runs);
			return NULL;
		}
		for (k = 0; k < count; k++) {
			indices[k] = start + k;
			advances[k] = CGSizeMake(ct_line_advance_at(l, start + k), 0.0);
		}
		attributes = (l->string == NULL) ? NULL
			: CFAttributedStringGetAttributes(l->string, l->range.location + start, NULL);
		created = CTRunCreateWithAttributesAndGlyphs(attributes, font, status,
			CFRangeMake(l->range.location + start, count), indices, l->glyphs + start, l->positions + start,
			advances, (l->isBox != NULL) ? l->isBox + start : NULL, count);
		free(indices);
		free(advances);
		if (created == NULL) {
			CFRelease(runs);
			return NULL;
		}
		CFArrayAppendValue(runs, created);
		CFRelease(created);
	}
	l->runs = runs;
	return l->runs;
}

/* =================================================================================================
 * THE TRUNCATION ENGINE, AND IT IS APPLE'S PRINTED CONTRACT AND NOTHING MORE THAN IT.
 *
 * Its header states four things, and those four are what this file implements: the line "will be truncated if its
 * width is greater than the width passed" — so a line that FITS is returned whole; the token "will be added to the
 * point where truncation took place"; "if this parameter is set to NULL, then no truncation token is used, and the
 * line is simply cut off"; and "if the width of the line specified in truncationToken is greater, this function will
 * return NULL if truncation is needed". The three types are Apple's own descriptions of which end stays visible, and
 * they are the three branches below.
 *
 * ONE BOUNDARY IS RECORDED RATHER THAN GUESSED, AND IT IS THE FONT. A LINE CAN HOLD SEVERAL FACES, BUT TRUNCATION ADDS
 * NONE OF ITS OWN: a TOKEN DRAWN IN ANOTHER FACE cannot be appended to it, because the token's glyph INDICES are
 * indices IN ITS OWN FONT and drawing them with the line's would put garbage where an ellipsis belongs — a wrong
 * answer that LOOKS like a truncation, which is the kind this tree refuses. Apple's answer is a line the FRAMESETTER
 * builds with runs of its own, and this tier's is to REFUSE; the case holds the refusal. A token in the SAME face —
 * the ellipsis it usually is — works, and that is the case the door was written for.
 *
 * AND ONE NUMBER APPLE DOES NOT PRINT, WHICH IS NAMED WHERE IT IS CHOSEN: for a MIDDLE truncation the available room is
 * SPLIT BETWEEN THE TWO ENDS, and how it splits is this tier's (half for the head, then whatever is left for the
 * tail). Apple's own sentence describes the result — "leaving both the start and the end portions visible" — and says
 * nothing about the division.
 * =============================================================================================== */
/* A LINE OVER GLYPHS THAT ARE NOT A STRING RANGE, which is what a truncated line is. */
CTLineRef CTLineCreateWithGlyphs(CFAttributedStringRef string, CFRange range, CTFontRef font, const CGGlyph *glyphs,
				 CFIndex count)
{
	struct __CTLine *line;
	CGSize *advances;
	CFIndex i;
	double x = 0.0;

	if (font == NULL || count < 0 || (count > 0 && glyphs == NULL)) {
		return NULL;
	}
	line = ct_line_alloc();
	if (line == NULL) {
		return NULL;
	}
	line->string = (string == NULL) ? NULL : (CFAttributedStringRef)CFRetain(string);
	line->font = (CTFontRef)CFRetain(font);
	line->range = range;
	line->direction = ct_line_base_direction(string, range, CTLineBidiDirectiveNone);
	line->ascent = CTFontGetAscent(font);
	line->descent = CTFontGetDescent(font);
	line->leading = CTFontGetLeading(font);
	if (count == 0) {
		return (CTLineRef)line;
	}
	line->glyphs = (CGGlyph *)malloc((size_t)count * sizeof(CGGlyph));
	line->positions = (CGPoint *)malloc((size_t)count * sizeof(CGPoint));
	advances = (CGSize *)malloc((size_t)count * sizeof(CGSize));
	if (line->glyphs == NULL || line->positions == NULL || advances == NULL) {
		free(advances);
		CFRelease((CFTypeRef)line);
		return NULL;
	}
	memcpy(line->glyphs, glyphs, (size_t)count * sizeof(CGGlyph));
	memset(advances, 0, (size_t)count * sizeof(CGSize));
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, line->glyphs, advances, count);
	for (i = 0; i < count; i++) {
		line->positions[i] = CGPointMake((CGFloat)x, 0.0);
		x += advances[i].width;
	}
	line->glyphCount = count;
	line->width = x;
	free(advances);
	return (CTLineRef)line;
}

/* THE WIDTH OF A LINE'S FIRST `count` GLYPHS, from the positions the line already holds: they are cumulative, so the
 * n-th one IS that width. One function, because two of the branches below need it and a second copy of the rule is a
 * second chance to get it wrong. */
static CGFloat ct_line_prefix_width(const struct __CTLine *line, CFIndex count)
{
	if (count <= 0) {
		return 0.0;
	}
	if (count >= line->glyphCount) {
		return (CGFloat)line->width;
	}
	return line->positions[count].x;
}

/* AS MANY GLYPHS AS FIT IN A WIDTH, FROM THE FRONT OR FROM THE BACK — the one measurement all three types share, so
 * that they cannot disagree about what "as many as fit" means. */
static CFIndex ct_line_fit(const struct __CTLine *line, double width, bool from_end)
{
	double used = 0.0;
	CFIndex i, taken = 0;

	if (width <= 0.0) {
		return 0;
	}
	for (i = 0; i < line->glyphCount; i++) {
		CFIndex at = from_end ? (line->glyphCount - 1 - i) : i;
		CGFloat advance = ct_line_advance_at(line, at);

		if (used + advance > width) {
			break;
		}
		used += advance;
		taken++;
	}
	return taken;
}

CTLineRef CTLineCreateTruncatedLine(CTLineRef line, double width, CTLineTruncationType truncationType,
				    CTLineRef truncationToken)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	const struct __CTLine *token = (const struct __CTLine *)truncationToken;
	CGGlyph *glyphs;
	CTLineRef result;
	CFRange range;
	CFIndex head = 0, tail = 0, count, i, at;
	double available;

	if (l == NULL) {
		return NULL;
	}
	/* THE LINE FITS, AND APPLE'S OWN CONDITION SAYS SO: it "will be truncated if its width is GREATER than the width
	 * passed", so nothing is cut and the result is a line of its own over the same range. */
	if (l->width <= width) {
		return CTLineCreateWithAttributedStringAndRange(l->string, l->range);
	}
	if (token != NULL) {
		if (token->font == NULL || !CFEqual(token->font, l->font)) {
			return NULL;	/* the recorded refusal: one font per line (see above) */
		}
		if (token->width >= width) {
			return NULL;	/* Apple's sentence for a token too wide to fit */
		}
	}
	available = (token != NULL) ? (width - token->width) : width;
	if (available < 0.0) {
		available = 0.0;
	}
	/* WHICH GLYPHS STAY. Apple's three descriptions are the three branches, and the middle one's division is named
	 * above as this tier's. */
	switch (truncationType) {
	case kCTLineTruncationStart:
		tail = ct_line_fit(l, available, true);
		break;
	case kCTLineTruncationMiddle:
		head = ct_line_fit(l, available / 2.0, false);
		tail = ct_line_fit(l, available - (double)ct_line_prefix_width(l, head), true);
		break;
	default:
		head = ct_line_fit(l, available, false);
		break;
	}

	/* THE RESULT IS THE KEPT GLYPHS WITH THE TOKEN'S BETWEEN THEM, in the order each type leaves them. */
	count = head + ((token != NULL) ? token->glyphCount : 0) + tail;
	glyphs = (CGGlyph *)malloc((size_t)((count > 0) ? count : 1) * sizeof(CGGlyph));
	if (glyphs == NULL) {
		return NULL;
	}
	at = 0;
	for (i = 0; i < head; i++) {
		glyphs[at++] = l->glyphs[i];
	}
	if (token != NULL) {
		for (i = 0; i < token->glyphCount; i++) {
			glyphs[at++] = token->glyphs[i];
		}
	}
	for (i = l->glyphCount - tail; i < l->glyphCount; i++) {
		glyphs[at++] = l->glyphs[i];
	}

	/* AND ITS STRING RANGE IS THE SPAN IT SHOWS OF THE ORIGINAL — which for a MIDDLE truncation covers the characters
	 * it DROPPED, and that is the honest description of a line whose glyphs no longer correspond one to one with a
	 * range of the string. APPLE PRINTS NOTHING ABOUT THIS DOOR'S RANGE, and a span is what a caret needs: it says
	 * where in the string this line begins and ends. The token contributes no characters, because its own are not in
	 * this string. */
	if (truncationType == kCTLineTruncationStart) {
		range = CFRangeMake(l->range.location + (l->glyphCount - tail), tail);
	} else if (truncationType == kCTLineTruncationMiddle) {
		range = CFRangeMake(l->range.location, head + tail);
	} else {
		range = CFRangeMake(l->range.location, head);
	}
	result = CTLineCreateWithGlyphs(l->string, range, l->font, glyphs, count);
	free(glyphs);
	return result;
}

/* =================================================================================================
 * THE JUSTIFICATION ENGINE, WHOSE HEADER PRINTS LESS THAN THE TRUNCATION ONE'S — SO WHAT IS NOT PRINTED IS RECORDED
 * HERE INSTEAD OF GUESSED AT QUIETLY.
 *
 * APPLE PRINTS THREE THINGS. `justificationFactor`: "When set to 1.0 or greater indicates, full justification will be
 * performed. If less than 1.0, varying degrees of partial justification will be performed. If set to 0 or less, then
 * no justification will be performed." `justificationWidth`: "The width to which the resultant line will be
 * justified." And: "If justificationWidth is less than the actual width of the line, then negative justification will
 * be performed ("text squishing")."
 *
 * AND IT PRINTS NOTHING ABOUT WHERE THE SLACK GOES. TWO DECISIONS ARE THEREFORE THIS TIER'S, AND BOTH ARE NAMED:
 *
 *   WHERE: THE SPACES. Full justification is the typographic word for WIDENING THE GAPS BETWEEN WORDS, which is what
 *   `CTParagraphStyle`'s `kCTJustifiedTextAlignment` means by "Text is fully justified" and why that same vocabulary
 *   says the LAST line of a paragraph "is naturally aligned" — a last line has no gap to widen. So the slack is
 *   divided among the line's SPACE glyphs, and a line with none comes back unexercised. SPREADING IT BETWEEN EVERY
 *   GLYPH would look plausible and be wrong, which is the failure this file named when the door was owed.
 *
 *   HOW MUCH: `min(factor, 1.0)`, LINEARLY. Apple says a factor below 1.0 gives "varying degrees of partial
 *   justification" and does not print HOW the degrees vary, so the factor is taken as the FRACTION of the slack that
 *   is taken up — 0.5 justifies half way to the width. That is a reading, and it is here rather than in a caption.
 *
 * AND NEGATIVE JUSTIFICATION IS THE SAME ARITHMETIC with a negative slack: the spaces SHRINK by their share, which is
 * Apple's own word for it.
 * =============================================================================================== */
/* THE LINE THE ENGINE BUILDS, AND WHY IT IS A HELPER RATHER THAN THE SEAM `CTLineCreateWithGlyphs`: a JUSTIFIED line
 * has the SAME GLYPHS as the line it came from and DIFFERENT ORIGINS — the gaps moved — so it cannot be placed by the
 * font's own advances, which is what that seam does. IT IS STATIC BECAUSE THE ENGINE IS ITS ONLY CALLER, and the
 * advances a caller reads back are the distances between these positions (`CTLineGetGlyphRuns`), so a justified space
 * reports the WIDER advance it was given, which is the whole point of the exercise. */
static struct __CTLine *ct_line_with_positions(CFAttributedStringRef string, CFRange range, CTFontRef font,
					       const CGGlyph *glyphs, const CGPoint *positions, CFIndex count)
{
	struct __CTLine *line;
	CGSize last = CGSizeMake(0.0, 0.0);

	if (font == NULL || count < 0 || (count > 0 && (glyphs == NULL || positions == NULL))) {
		return NULL;
	}
	line = ct_line_alloc();
	if (line == NULL) {
		return NULL;
	}
	line->string = (string == NULL) ? NULL : (CFAttributedStringRef)CFRetain(string);
	line->font = (CTFontRef)CFRetain(font);
	line->range = range;
	line->direction = ct_line_base_direction(string, range, CTLineBidiDirectiveNone);
	line->ascent = CTFontGetAscent(font);
	line->descent = CTFontGetDescent(font);
	line->leading = CTFontGetLeading(font);
	if (count == 0) {
		return line;
	}
	line->glyphs = (CGGlyph *)malloc((size_t)count * sizeof(CGGlyph));
	line->positions = (CGPoint *)malloc((size_t)count * sizeof(CGPoint));
	if (line->glyphs == NULL || line->positions == NULL) {
		CFRelease((CFTypeRef)line);
		return NULL;
	}
	memcpy(line->glyphs, glyphs, (size_t)count * sizeof(CGGlyph));
	memcpy(line->positions, positions, (size_t)count * sizeof(CGPoint));
	line->glyphCount = count;
	/* THE WIDTH IS THE LAST GLYPH'S ORIGIN PLUS THAT GLYPH'S OWN ADVANCE — and the last advance comes from the FONT,
	 * because only the SPACES were given new room: the final glyph is exactly as wide as it always was. Position
	 * differences cannot supply it, since there is no position after the last one. */
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, &glyphs[count - 1], &last, 1);
	line->width = (double)positions[count - 1].x + last.width;
	return line;
}

/* THE SPACE GLYPH OF A FACE, or 0 when it has none — the one thing this engine needs to know about a line's glyphs
 * that the line itself does not store, since it keeps GLYPHS and not the characters they came from. */
static CGGlyph ct_line_space_glyph(CTFontRef font)
{
	UniChar space = (UniChar)' ';
	CGGlyph glyph = 0;

	if (font == NULL) {
		return 0;
	}
	(void)CTFontGetGlyphsForCharacters(font, &space, &glyph, 1);
	return glyph;
}

CTLineRef CTLineCreateJustifiedLine(CTLineRef line, CGFloat justificationFactor, double justificationWidth)
{
	const struct __CTLine *l = (const struct __CTLine *)line;
	CGPoint *positions;
	CTLineRef result;
	CGGlyph space;
	CFIndex i, spaces = 0;
	double slack, share, shift = 0.0;

	if (l == NULL) {
		return NULL;
	}
	/* "IF SET TO 0 OR LESS, THEN NO JUSTIFICATION WILL BE PERFORMED", which is Apple's sentence and this door's first
	 * branch: the result is a line of its own over the same range. */
	if (justificationFactor <= 0.0 || l->glyphCount == 0) {
		return CTLineCreateWithAttributedStringAndRange(l->string, l->range);
	}
	space = ct_line_space_glyph(l->font);
	for (i = 0; i < l->glyphCount; i++) {
		if (space != 0 && l->glyphs[i] == space) {
			spaces++;
		}
	}
	/* A LINE WITH NO SPACE HAS NO GAP TO WIDEN — a single long word, or a face with no space glyph — so it comes back
	 * over its own range rather than being stretched between its glyphs (see this section's head). */
	if (spaces == 0) {
		return CTLineCreateWithAttributedStringAndRange(l->string, l->range);
	}
	/* THE SLACK, TAKEN UP TO THE FACTOR: positive when the width asked for is WIDER than the line, negative when it
	 * is narrower — Apple's own "text squishing". */
	slack = ((double)justificationWidth - l->width) *
		(double)((justificationFactor > 1.0) ? 1.0 : justificationFactor);
	share = slack / (double)spaces;

	positions = (CGPoint *)malloc((size_t)l->glyphCount * sizeof(CGPoint));
	if (positions == NULL) {
		return NULL;
	}
	/* THE SHIFT IS APPLIED AFTER EACH SPACE, so it is the SPACE that grows and the word that follows it that moves —
	 * which is what "widening the gap between words" means, and it leaves the first word where it was. */
	for (i = 0; i < l->glyphCount; i++) {
		positions[i] = CGPointMake((CGFloat)(l->positions[i].x + shift), l->positions[i].y);
		if (l->glyphs[i] == space) {
			shift += share;
		}
	}
	result = (CTLineRef)ct_line_with_positions(l->string, l->range, l->font, l->glyphs, positions, l->glyphCount);
	free(positions);
	return result;
}

/* =================================================================================================
 * OWED DOORS, EACH WITH THE REASON IT CANNOT BE ANSWERED HERE. They are DECLARED in CTLine.h and NOT DEFINED, so a
 * caller who reaches for one gets a link error rather than an invented answer.
 *
 * THIS FILE OWES NOTHING NOW, AND THAT IS WORTH SAYING RATHER THAN LEAVING AS AN EMPTY SECTION. The three doors that
 * were owed here left in the order their reasons became false, and EACH OF THOSE REASONS WAS A CONDITION THAT COULD BE
 * CHECKED: `CTLineGetGlyphRuns` waited for `CTRun` to exist; `CTLineCreateTruncatedLine` waited for "the typesetter,
 * whose job the breaking is"; and `CTLineCreateJustifiedLine` waited for a decision about where justification slack
 * goes, which the section above makes and names. A REASON THAT CANNOT BE CHECKED IS A SENTENCE, NOT A REASON.
 * =============================================================================================== */
