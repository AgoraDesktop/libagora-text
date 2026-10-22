/*
 * CTTypesetter.c — the typesetter: the first thing in this library that DECIDES rather than measures.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 *
 * APPLE'S OWN SENTENCE IS THE ALGORITHM, and it is two clauses long: "The line break can be triggered either by a
 * HARD BREAK CHARACTER in the stream or by FILLING THE SPECIFIED WIDTH with characters." So this file walks the
 * string's characters once for a newline and, failing that, adds widths until the next one would not fit — and every
 * decision below is the paragraph style's or Apple's, never a rule invented here.
 *
 * THE WHOLE STRING IS TYPESET ONCE, WHICH IS WHAT MAKES THE DOORS CHEAP. The glyphs and advances for the entire
 * attributed string are computed in the creator — the same two calls a line makes for its own range — and every
 * later question ("do these forty characters fit in 250 points") is then a sum over an array rather than a second
 * pass over the text. THE CHARACTERS ARE KEPT TOO, because the two questions the breaker asks of them are not
 * questions a glyph can answer: whether a character is a NEWLINE (a hard break, in Core Foundation's own vocabulary
 * of newlines rather than in a list written here) and whether it is a SPACE (where word wrapping backs off to).
 *
 * AND WHAT IS ABSENT IS NO LONGER ABSENT: **BOTH OPTIONS ARE OBEYED NOW.** `kCTTypesetterOptionDisableBidiProcessing`
 * and `kCTTypesetterOptionForcedEmbeddingLevel` are read into a `CTLineBidiDirective` and handed to the LINE, which is
 * where the algorithm runs — so the typesetter still consults neither while BREAKING (bidi changes no advance) and both
 * change the line this typesetter makes. **AND THE LINE-BREAKING RULES ARE UAX #14'S NOW**: the break a word-wrapped
 * line backs off to is the last opportunity the Unicode Line Breaking Algorithm allows (`CTLineBreak.c`), not the last
 * space — which is why a break can now fall after a hyphen or a slash and why a Hangul syllable is not cut in half.
 * **AND HYPHENATION IS HERE NOW, BEHIND AN OPTION.** `kCTTypesetterOptionHyphenationFactor` — this tier's own key,
 * because the setting is AppKit's, and the declaration carries the reasoning — makes the breaker consider hyphenation
 * points where UAX #14 has none, or where the legal break would leave the line too empty. **THE DEFAULT IS ZERO AND
 * THE DEFAULT IS WHAT EVERY EXISTING CALLER ALREADY GOT**, which is what makes the option an option.
 * **AND THE SCRIPTS UAX #14 SENDS TO A DICTIONARY HAVE THEIR ANALYSIS NOW AND NOT THEIR WIRING** (`SA`: Thai, Lao,
 * Khmer): `CTLineBreakThai.c` reads libthai's trie, measured against `th_brk()`, and this typesetter does not ask it
 * yet. They are one
 * unbreakable run here — said in words by `ct_lb_break_before_utf16`'s caller and by the header.
 *
 * **AND ONE BUG OF LONG STANDING WAS FOUND BY A CASE ABOUT SOMETHING ELSE.** The hard break was returned
 * UNCONDITIONALLY — see `ct_ts_hard_break_reached` — so a paragraph longer than its container that ended in a newline
 * came back as ONE LINE THAT OVERFLOWS. Every case in this suite that involved a newline used lines short enough to
 * fit, which is exactly why it survived: the case that found it was written to measure PER-PARAGRAPH HYPHENATION in
 * the AppKit, and its first paragraph (`go internationalization\n`, 135.9 points in a 74.7-point container) came back
 * as a single fragment. Seven characters' worth of wrapping appeared the moment the rule was stated correctly.
 */
#include <CoreText/CTTypesetter.h>

#include <CoreText/CTFont.h>
#include <CoreText/CTLine_internal.h>
#include <CoreText/CTFont_internal.h>
#include <CoreText/CTParagraphStyle.h>
#include <CoreText/CTRunDelegate.h>
#include <CoreText/CTRunDelegate_internal.h>
#include <CoreText/CTStringAttributes.h>
#include <CoreText/CTTextTab.h>
#include <CoreText/CTLineBreak_internal.h>

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFRuntime.h>

#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* THE TWO OPTIONS, WHOSE VALUES ARE INFERENCES: Apple declares both and publishes no value for either, so each takes
 * its own constant's name with the leading `k` dropped — the same standing the other unpublished keys in this library
 * carry, and for the same reason. */
const CFStringRef kCTTypesetterOptionDisableBidiProcessing = CFSTR("CTTypesetterOptionDisableBidiProcessing");
const CFStringRef kCTTypesetterOptionForcedEmbeddingLevel = CFSTR("CTTypesetterOptionForcedEmbeddingLevel");

/* **AND A THIRD KEY, WHICH IS THIS TIER'S OWN AND IS RECORDED AS SUCH.** Apple declares the two above and this one
 * does not exist in any version of `CTTypesetter.h` — because the SETTING it carries lives in AppKit:
 * `NSParagraphStyle -hyphenationFactor` is a paragraph property, `CTParagraphStyle` has no specifier for it, and on
 * the platform this library follows the layout manager is what reads the factor and asks
 * `CFStringGetHyphenationLocationBeforeIndex`. So a TYPESETTER that can hyphenate needs a channel for that setting,
 * Apple does not provide one in Core Text, and the options dictionary is the surface designed for exactly this — it
 * is a dictionary, Apple's own two keys are precedent, and a caller that passes nothing gets the behaviour it always
 * got.
 *
 * **THE ALTERNATIVE WAS REFUSED RATHER THAN OVERLOOKED: `SuggestLineBreak` COULD ALWAYS HYPHENATE, AND IT MUST NOT.**
 * Two reasons, and both are properties of a published door rather than matters of taste. Its answer would depend on
 * the MACHINE — a `hyph_en_US.dic` that is present on one system and absent on another would break the same string
 * at a different character — and it would change the meaning of a door Apple documents, for every caller, including
 * the ones that asked for nothing. **A DOOR THAT CHANGES ITS ANSWER WITH THE MACHINE IS THE ONE THING A LAYOUT
 * ENGINE MUST NOT DO**, which is why hyphenation is opted into here and the factor is the opt-in.
 *
 * ITS VALUE IS A `CFNumber` and its reading is the AppKit's own: **0.0 means no hyphenation** and the number is the
 * share of the width a line must be able to fill before hyphenation is worth it — the hyphenation zone, which
 * `NSLayoutManager` records at the door it reads. A missing key, a value of the wrong type and a value of zero are
 * all "do not hyphenate", which is what every existing caller already got. */
const CFStringRef kCTTypesetterOptionHyphenationFactor = CFSTR("CTTypesetterOptionHyphenationFactor");

/* AND THE PARAGRAPH STYLE'S KEY, WHICH IS NO LONGER WRITTEN HERE. This function used to return the literal
 * "NSParagraphStyleAttributeName", because `kCTParagraphStyleAttributeName` lives in a header this repository had not
 * reached; THAT HEADER HAS LANDED, so the value is Apple's constant and this file no longer knows it. THE FUNCTION
 * STAYS, and the reason is not ceremony: the line-break mode under this key is the ONE attribute this tier OBEYS, so
 * a reader asking what a typesetter reads should find one place that says so rather than a symbol buried in a call
 * two hundred lines down. */
static CFStringRef ct_ts_paragraph_style_attribute(void)
{
	return kCTParagraphStyleAttributeName;
}

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTTypesetter {
	CFRuntimeBase base;
	CFAttributedStringRef string;	/* RETAINED: the text being typeset */
	CTFontRef font;			/* RETAINED: the face the whole string is set in */
	CFDictionaryRef options;	/* RETAINED: read into the LINE's bidi directive by `ct_ts_bidi_directive` */
	UniChar *chars;			/* malloc'd, count: the characters, for newlines and spaces */
	CGGlyph *glyphs;		/* malloc'd, count */
	CGSize *advances;		/* malloc'd, count */
	/* **AND WHICH CHARACTERS ARE BOXES**, so the fit does not kern a box against its neighbour — a box has no glyph to
	 * adjust. Copied from the delegate attribute each character carries. */
	bool *isBox;			/* malloc'd, count, or NULL */
	CFIndex count;
	CTLineBreakMode mode;		/* the paragraph style's mode, or the default when it has none */
	/* **AND WHETHER THIS TYPESETTER MAY HYPHENATE, from `kCTTypesetterOptionHyphenationFactor`.** Zero — the default
	 * — is no hyphenation, which is what every caller that passes no options gets. */
	double hyphenationFactor;
	/* **AND THE TAB STOPS THE FIT OBEYS**, borrowed from the paragraph style the string carries (which the typesetter's
	 * own retained string keeps alive) — the same list `CTLine` reads, so a line breaks where it will actually end. */
	CFArrayRef tabs;		/* BORROWED, or NULL */
	double tabInterval;
};

static void ct_ts_finalize(CFTypeRef cf)
{
	struct __CTTypesetter *ts = (struct __CTTypesetter *)cf;

	if (ts->string != NULL) {
		CFRelease(ts->string);
	}
	if (ts->font != NULL) {
		CFRelease(ts->font);
	}
	if (ts->options != NULL) {
		CFRelease(ts->options);
	}
	free(ts->chars);
	free(ts->glyphs);
	free(ts->advances);
	free(ts->isBox);
}

/* TWO TYPESETTERS ARE EQUAL WHEN THEY TYPESET THE SAME STRING IN THE SAME FACE **AND WOULD MAKE THE SAME LINE**,
 * which is what the BIDI DIRECTIVE decides. It used to say the options changed nothing, and that was true then; the
 * honest version is not "the dictionaries are equal" either, because an options dictionary carrying a key this tier
 * ignores — or a value of the wrong type — changes no line, and two typesetters that lay text out identically ARE the
 * same typesetter. So the DIRECTIVES are compared, and the hash below stays what it was, which is what makes the two
 * agree: equal objects hash alike. */
static CTLineBidiDirective ct_ts_bidi_directive(const struct __CTTypesetter *ts);

static Boolean ct_ts_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTTypesetter *a = (const struct __CTTypesetter *)cf1;
	const struct __CTTypesetter *b = (const struct __CTTypesetter *)cf2;
	CTLineBidiDirective da, db;

	if ((a->string == NULL) != (b->string == NULL) || (a->font == NULL) != (b->font == NULL)) {
		return false;
	}
	if (a->string != NULL && !CFEqual(a->string, b->string)) {
		return false;
	}
	if (a->font != NULL && !CFEqual(a->font, b->font)) {
		return false;
	}
	da = ct_ts_bidi_directive(a);
	db = ct_ts_bidi_directive(b);
	/* **AND THE HYPHENATION FACTOR IS PART OF WHAT MAKES TWO TYPESETTERS THE SAME ONE**, for the same reason the
	 * directive is: it changes the lines they make. */
	return da.forcedLevel == db.forcedLevel && da.disabled == db.disabled
	       && a->hyphenationFactor == b->hyphenationFactor;
}

static CFHashCode ct_ts_hash(CFTypeRef cf)
{
	const struct __CTTypesetter *ts = (const struct __CTTypesetter *)cf;
	CFHashCode h = 13u;

	if (ts->string != NULL) {
		h = h * 31u + CFHash(ts->string);
	}
	if (ts->font != NULL) {
		h = h * 31u + CFHash(ts->font);
	}
	return h;
}

static const CFRuntimeClass ct_ts_class = {
	0,		/* version */
	"CTTypesetter",	/* className */
	NULL,		/* init */
	NULL,		/* copy */
	ct_ts_finalize,	/* finalize */
	ct_ts_equal,	/* equal */
	ct_ts_hash,	/* hash */
	NULL, NULL, NULL, NULL,
	0
};

CFTypeID CTTypesetterGetTypeID(void)
{
	static CFTypeID type_id = 0;

	if (type_id == 0) {
		type_id = _CFRuntimeRegisterClass(&ct_ts_class);
	}
	return type_id;
}

/* =================================================================================================
 * THE BREAKER'S TWO VOCABULARIES, BOTH TAKEN FROM CORE FOUNDATION RATHER THAN WRITTEN HERE: a newline is a member of
 * CF's own newline set, and a space is a member of its whitespace-and-newline set. A set the platform cannot give up
 * answers "no", which makes the breaker fall back to the width instead of guessing.
 * =============================================================================================== */
/* THE NEWLINE SET IS Core Foundation's, AND ITS NAME IS SINGULAR — `kCFCharacterSetNewline`, which is the spelling
 * the compiler corrected this file to when the plural was written: the SET is a set of newline characters, and Apple's
 * constant for it is not the plural of its contents. */
static bool ct_ts_is_newline(UniChar ch)
{
	CFCharacterSetRef set = CFCharacterSetGetPredefined(kCFCharacterSetNewline);

	return set != NULL && CFCharacterSetIsCharacterMember(set, ch);
}

static bool ct_ts_is_space(UniChar ch)
{
	CFCharacterSetRef set = CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline);

	return set != NULL && CFCharacterSetIsCharacterMember(set, ch);
}

/* =================================================================================================
 * CREATION. One pass over the string for everything the doors below will ever ask.
 * =============================================================================================== */
static struct __CTTypesetter *ct_ts_alloc(void)
{
	return (struct __CTTypesetter *)_CFRuntimeCreateInstance(kCFAllocatorDefault,
		CTTypesetterGetTypeID(),
		(CFIndex)(sizeof(struct __CTTypesetter) - sizeof(CFRuntimeBase)), NULL);
}

CTTypesetterRef CTTypesetterCreateWithAttributedStringAndOptions(CFAttributedStringRef string,
								 CFDictionaryRef options)
{
	struct __CTTypesetter *ts;
	CFStringRef text;
	CFTypeRef font_value;
	CFTypeRef style_value;
	CFRange whole;
	CFIndex n;

	if (string == NULL) {
		return NULL;
	}
	/* THE FACE IS READ BY THE SAME KEY THE LINE READS IT BY, and through the same single copy of that value — the
	 * key belongs to a header this tree has not reached, and it is written down in exactly one place (`CTLine.c`) so
	 * that two files cannot disagree about it. */
	font_value = CFAttributedStringGetAttribute(string, 0, CTLineFontAttributeName(), NULL);
	if (font_value == NULL || CFGetTypeID(font_value) != CTFontGetTypeID()) {
		return NULL;
	}
	text = CFAttributedStringGetString(string);
	n = CFStringGetLength(text);
	ts = ct_ts_alloc();
	if (ts == NULL) {
		return NULL;
	}
	ts->string = (CFAttributedStringRef)CFRetain(string);
	ts->font = (CTFontRef)CFRetain(font_value);
	ts->options = (options == NULL) ? NULL : (CFDictionaryRef)CFRetain(options);
	ts->count = n;
	ts->mode = kCTLineBreakByWordWrapping;	/* the default, and Apple prints it beside the specifier */
	ts->hyphenationFactor = 0.0;		/* and no hyphenation is the default the same way */
	/* **AND THE HYPHENATION FACTOR IS READ HERE RATHER THAN INSIDE THE PARAGRAPH-STYLE BRANCH BELOW** — it is an
	 * OPTION and the paragraph style has no specifier for it, so a string with no style at all must still be able to
	 * ask for hyphenation. MEASURED, and this is the bug: the read was written inside that branch, so every case
	 * with no paragraph style silently got no hyphenation and the case below measured nothing. **AND IT IS GUARDED,
	 * because a caller with no options is the ordinary caller and `CFDictionaryGetValue` is not NULL-safe** — the
	 * unguarded version SEGFAULTED two of this suite's own cases the moment it ran. */
	if (options != NULL) {
		CFTypeRef factor = CFDictionaryGetValue(options, kCTTypesetterOptionHyphenationFactor);

		if (factor != NULL && CFGetTypeID(factor) == CFNumberGetTypeID()) {
			double value = 0.0;

			if (CFNumberGetValue((CFNumberRef)factor, kCFNumberDoubleType, &value) && value > 0.0) {
				ts->hyphenationFactor = value;
			}
		}
	}

	/* THE PARAGRAPH STYLE'S MODE, IF THE STRING CARRIES ONE. This is the one place the settings a paragraph style
	 * holds actually CHANGE what the library does — everything else so far has recorded them — and it is read here
	 * rather than in the breaker because it cannot change while the typesetter lives. */
	style_value = CFAttributedStringGetAttribute(string, 0, ct_ts_paragraph_style_attribute(), NULL);
	if (style_value != NULL && CFGetTypeID(style_value) == CTParagraphStyleGetTypeID()) {
		CTLineBreakMode mode = kCTLineBreakByWordWrapping;

		if (CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style_value,
							 kCTParagraphStyleSpecifierLineBreakMode, sizeof mode, &mode)) {
			ts->mode = mode;
		}
		{
			CFArrayRef tabs = NULL;
			CGFloat interval = 0.0;

			(void)CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style_value,
					kCTParagraphStyleSpecifierTabStops, sizeof tabs, &tabs);
			(void)CTParagraphStyleGetValueForSpecifier((CTParagraphStyleRef)style_value,
					kCTParagraphStyleSpecifierDefaultTabInterval, sizeof interval, &interval);
			ts->tabs = tabs;
			ts->tabInterval = (double)interval;
		}
	}
	if (n <= 0) {
		return (CTTypesetterRef)ts;
	}

	whole = CFRangeMake(0, n);
	ts->chars = (UniChar *)malloc((size_t)n * sizeof(UniChar));
	ts->glyphs = (CGGlyph *)malloc((size_t)n * sizeof(CGGlyph));
	ts->advances = (CGSize *)malloc((size_t)n * sizeof(CGSize));
	ts->isBox = (bool *)calloc((size_t)n, sizeof(bool));
	if (ts->chars == NULL || ts->glyphs == NULL || ts->advances == NULL || ts->isBox == NULL) {
		CFRelease((CFTypeRef)ts);	/* the finalizer frees whichever arrays did arrive */
		return NULL;
	}
	CFStringGetCharacters(text, whole, ts->chars);
	(void)CTFontGetGlyphsForCharacters(ts->font, ts->chars, ts->glyphs, n);
	memset(ts->advances, 0, (size_t)n * sizeof(CGSize));
	(void)CTFontGetAdvancesForGlyphs(ts->font, kCTFontHorizontalOrientation, ts->glyphs, ts->advances, n);
	/* **AND EACH BOX MEASURES WHAT ITS DELEGATE SAYS, NOT WHAT THE PLACEHOLDER GLYPH DID**, so the fit and the line
	 * agree about the width — the same rule `CTLine` applies when it builds the glyphs. */
	{
		CFIndex i;

		for (i = 0; i < n; i++) {
			CFTypeRef value = CFAttributedStringGetAttribute(string, i, kCTRunDelegateAttributeName, NULL);

			if (value != NULL && CFGetTypeID(value) == CTRunDelegateGetTypeID()) {
				ts->isBox[i] = true;
				ts->glyphs[i] = 0;
				ts->advances[i] = CGSizeMake(CTRunDelegateWidth((CTRunDelegateRef)value), 0.0);
			}
		}
	}
	return (CTTypesetterRef)ts;
}

CTTypesetterRef CTTypesetterCreateWithAttributedString(CFAttributedStringRef string)
{
	return CTTypesetterCreateWithAttributedStringAndOptions(string, NULL);
}

/* =================================================================================================
 * A LINE FROM IT, OVER A RANGE.
 * =============================================================================================== */
/* **THE TYPESETTER'S TWO OPTIONS, READ INTO THE DIRECTIVE THE LINE UNDERSTANDS.** Apple's meaning for each is
 * published in the option's own name; what is NOT published is either key's VALUE, which is why the constants above
 * are inferences while these two readings are not.
 *
 * **AND A VALUE OF THE WRONG TYPE, OR A LEVEL THAT IS NOT 0 OR 1, IS IGNORED RATHER THAN REFUSED.** Apple makes both
 * a precondition (a boolean, and a level of 0 or 1); refusing the whole typesetter for one bad key would fail a
 * layout over a value that has an obvious "none" reading. Ignoring it is the READING this tier takes, and it is
 * named here so that a caller can tell which of the two happened: `CTTypesetterCreateLine` never fails for an option.
 *
 * **AND `DisableBidiProcessing` WINS OVER A FORCED LEVEL**, because a forced level is an INPUT to an algorithm that is
 * not going to run. Apple does not say which wins when both are given; that is the choice, and it is the only one that
 * does not leave the caller with a line that is half-processed. */
static CTLineBidiDirective ct_ts_bidi_directive(const struct __CTTypesetter *ts)
{
	CTLineBidiDirective directive = CTLineBidiDirectiveNone;
	CFTypeRef value;

	if (ts->options == NULL) {
		return directive;
	}
	value = CFDictionaryGetValue(ts->options, kCTTypesetterOptionDisableBidiProcessing);
	if (value != NULL && CFGetTypeID(value) == CFBooleanGetTypeID()) {
		directive.disabled = CFBooleanGetValue((CFBooleanRef)value) ? true : false;
	}
	value = CFDictionaryGetValue(ts->options, kCTTypesetterOptionForcedEmbeddingLevel);
	if (value != NULL && CFGetTypeID(value) == CFNumberGetTypeID()) {
		CFIndex level = -1;

		if (CFNumberGetValue((CFNumberRef)value, kCFNumberCFIndexType, &level) && (level == 0 || level == 1)) {
			directive.forcedLevel = (int)level;
		}
	}
	return directive;
}

CTLineRef CTTypesetterCreateLine(CTTypesetterRef typesetter, CFRange stringRange)
{
	const struct __CTTypesetter *ts = (const struct __CTTypesetter *)typesetter;
	CFRange r = stringRange;

	if (ts == NULL) {
		return NULL;
	}
	/* A ZERO LENGTH MEANS THE REST OF THE STRING, in Apple's own words for this parameter. */
	if (r.length == 0) {
		r.length = ts->count - r.location;
	}
	/* AND A RANGE OUTSIDE THE STRING FAILS, which Apple states as a requirement of the CALLER and this door answers
	 * by refusing rather than by clipping to fit. */
	if (r.location < 0 || r.length < 0 || r.location > ts->count || r.location + r.length > ts->count) {
		return NULL;
	}
	return CTLineCreateWithAttributedStringAndRangeWithBidi(ts->string, r, ct_ts_bidi_directive(ts));
}

/* =================================================================================================
 * WHERE THE NEXT LINE STARTS.
 * =============================================================================================== */
/* HOW FAR THE WIDTH REACHES, as an index one past the last character that fits. A GPU-less sum over the advances,
 * which is why this tier can answer it at all: measuring needs nothing but the numbers the creator already computed.
 * A character whose advance is ZERO always fits, so this cannot spin on a combining mark. */
/* **HOW WIDE ONE CHARACTER IS AT A GIVEN PEN POSITION, IN ONE PLACE.** A step is the glyph's advance PLUS the pair's
 * adjustment, or — for a tab — the distance to the next stop, which depends on where the pen already is. TWO WALKS
 * NEED IT NOW (`ct_ts_fit` forward, and the hyphenation test measuring a candidate break), and a second copy of these
 * rules would be a second chance to disagree with `CTLine`'s own positions.
 *
 * **AND A TAB TAKES THE FIT TO ITS STOP.** A tab's advance is not in `ts->advances` — it depends on where the pen
 * already is — so this computes it the same way `CTLine` does, from the same stops, and the two agree about where the
 * line ends. **AND THE PAIR'S ADJUSTMENT IS INCLUDED, BECAUSE THE LINE'S OWN WIDTH HAS IT AND THE TWO MUST AGREE**:
 * an advance is what a glyph asks for on its own, a KERN is what the font says about the two glyphs TOGETHER, and
 * `CTLine` applies it. A box has no glyph to adjust, so the kern is skipped. */
static double ct_ts_step(const struct __CTTypesetter *ts, CFIndex start, CFIndex i, double x)
{
	if (ts->chars[i] == '\t') {
		double stop = x;
		CFIndex t;

		for (t = 0; ts->tabs != NULL && t < CFArrayGetCount(ts->tabs); t++) {
			CTTextTabRef tab = (CTTextTabRef)CFArrayGetValueAtIndex(ts->tabs, t);
			double location = (tab != NULL) ? CTTextTabGetLocation(tab) : 0.0;

			if (location > x) {
				stop = location;
				break;
			}
		}
		if (stop == x && ts->tabInterval > 0.0) {
			stop = (floor(x / ts->tabInterval) + 1.0) * ts->tabInterval;
		}
		return stop - x;
	}
	{
		double step = ts->advances[i].width;

		if (i > start && !(ts->isBox != NULL && (ts->isBox[i - 1] || ts->isBox[i]))) {
			step += (double)CTFontGetKerningBetweenGlyphs(ts->font, ts->glyphs[i - 1], ts->glyphs[i]);
		}
		return step;
	}
}

static CFIndex ct_ts_fit(const struct __CTTypesetter *ts, CFIndex start, double width)
{
	double x = 0.0;
	CFIndex i = start;

	while (i < ts->count) {
		double step = ct_ts_step(ts, start, i, x);

		if (x + step > width) {
			break;
		}
		x += step;
		i++;
	}
	return i;
}

/* **THE WIDTH OF THE TEXT FROM `start` UP TO `end`**, by the same steps the fit walks — the measurement the
 * hyphenation test needs, which is "how full would this line be if it ended here". */
static double ct_ts_width_to(const struct __CTTypesetter *ts, CFIndex start, CFIndex end)
{
	double x = 0.0;
	CFIndex i = start;

	while (i < end && i < ts->count) {
		x += ct_ts_step(ts, start, i, x);
		i++;
	}
	return x;
}

/* **THE WIDTH OF THE HYPHEN A HYPHENATED LINE DRAWS, WHICH IS PART OF WHETHER IT FITS.** A break inside a word is
 * only a break if the mark that says so fits with it — `U+2010 HYPHEN`, the character `CFHyphenation.c` suggests and
 * the one the AppKit's layout manager draws — and a typesetter that ignored it would hand back a line that overflows
 * by exactly one glyph. A face with no glyph for it answers zero, which is the honest answer: nothing is drawn. */
static double ct_ts_hyphen_width(const struct __CTTypesetter *ts)
{
	UniChar hyphen = 0x2010;
	CGGlyph glyph = 0;
	CGSize advance = CGSizeMake(0.0, 0.0);

	if (!CTFontGetGlyphsForCharacters(ts->font, &hyphen, &glyph, 1)) {
		return 0.0;
	}
	(void)CTFontGetAdvancesForGlyphs(ts->font, kCTFontHorizontalOrientation, &glyph, &advance, 1);
	return (double)advance.width;
}

/* **THE HARD BREAK IS ONE OF TWO TRIGGERS, AND THE OTHER IS THE WIDTH — SO IT WINS ONLY IF IT IS REACHED.** Apple's
 * sentence puts them side by side: "The line break can be triggered either by a hard break character in the stream or
 * by FILLING THE SPECIFIED WIDTH with characters." A newline does not stop being one because the width had room to
 * spare, which is the reading this door has always had — **BUT A NEWLINE BEYOND THE WIDTH DOES NOT MAKE THE LINE
 * FIT**, and returning it unconditionally did exactly that: `ct_ts_hard_break` scanned forward for the first newline
 * and this door returned it whatever the width was, so A PARAGRAPH LONGER THAN ITS CONTAINER THAT ENDS IN A NEWLINE
 * came back as ONE LINE THAT OVERFLOWS. MEASURED, and it is what a two-paragraph case in the AppKit's suite found: a
 * 135.9-point paragraph in a 74.7-point container was laid out as a single fragment, and the only reason no test had
 * seen it is that every case with a newline in it used lines short enough to fit.
 *
 * THE BREAK CHARACTER IS STILL PART OF THE LINE, which is what makes a line built from this answer carry its newline
 * — and what lets the caller's loop advance past it. */
static CFIndex ct_ts_hard_break_reached(const struct __CTTypesetter *ts, CFIndex start, CFIndex hard, CFIndex fit)
{
	/* The newline is the LAST character of the hard break's span, so the break is reached when that character fits. */
	return (hard > 0 && start + hard <= fit) ? hard : 0;
}
static CFIndex ct_ts_hard_break(const struct __CTTypesetter *ts, CFIndex start)
{
	CFIndex i;

	for (i = start; i < ts->count; i++) {
		if (ct_ts_is_newline(ts->chars[i])) {
			return i - start + 1;
		}
	}
	return 0;
}

/* **THE LAST HYPHENATION POINT BEFORE `limit` THAT STILL FITS, OR -1.** The resource is Core Foundation's — the same
 * `CFStringGetHyphenationLocationBeforeIndex` the AppKit's layout manager asks, over the system's dictionary — so
 * there is ONE implementation of Liang's algorithm and one set of patterns behind every hyphen in this tree.
 *
 * **AND "FITS" INCLUDES THE HYPHEN.** The line takes the text up to the point AND the mark that says the word
 * continues, so a point whose text fits but whose hyphen does not is not a point — a typesetter that ignored the
 * hyphen would hand back a line that overflows by exactly one glyph. The locale is the process's, which is what the
 * Core Foundation door defaults to and what the AppKit's own door records as its reading. */
static CFIndex ct_ts_hyphenation_point(const struct __CTTypesetter *ts, CFIndex start, CFIndex limit, double width)
{
	CFStringRef text;
	CFIndex point;

	if (ts->hyphenationFactor <= 0.0 || ts->string == NULL || limit <= start) {
		return -1;
	}
	text = CFAttributedStringGetString(ts->string);
	if (text == NULL) {
		return -1;
	}
	point = CFStringGetHyphenationLocationBeforeIndex(text, limit, CFRangeMake(start, limit - start), 0, NULL, NULL);
	if (point == kCFNotFound || point <= start) {
		return -1;
	}
	if (ct_ts_width_to(ts, start, point) + ct_ts_hyphen_width(ts) > width) {
		return -1;
	}
	return point;
}

CFIndex CTTypesetterSuggestLineBreak(CTTypesetterRef typesetter, CFIndex startIndex, double width)
{
	const struct __CTTypesetter *ts = (const struct __CTTypesetter *)typesetter;
	CFIndex hard, fit, opportunity;

	if (ts == NULL || startIndex < 0 || startIndex >= ts->count) {
		return 0;
	}
	/* **THE FIT IS COMPUTED BEFORE THE HARD BREAK IS BELIEVED**, because the two triggers are compared rather than
	 * ordered — see `ct_ts_hard_break_reached`. */
	fit = ct_ts_fit(ts, startIndex, width);
	hard = ct_ts_hard_break_reached(ts, startIndex, ct_ts_hard_break(ts, startIndex), fit);
	if (hard > 0) {
		return hard;
	}
	/* TWO OF THE PARAGRAPH STYLE'S SIX MODES DO NOT WRAP AT ALL, and this is a reading of them worth stating: a line
	 * that CLIPS is drawn cut off at the width rather than broken into two, and a line that TRUNCATES is given an
	 * ellipsis by the framesetter — in both cases the text that "fills the width" is not the unit the mode is
	 * measured in, so the whole remainder is returned and the DRAWN result is the caller's. */
	if (ts->mode == kCTLineBreakByClipping || ts->mode == kCTLineBreakByTruncatingHead ||
	    ts->mode == kCTLineBreakByTruncatingTail || ts->mode == kCTLineBreakByTruncatingMiddle) {
		return ts->count - startIndex;
	}
	if (fit >= ts->count) {
		return ts->count - startIndex;
	}
	/* A CHARACTER BREAK IS WHERE THE FIRST CHARACTER THAT DOES NOT FIT BEGINS — Apple's sentence for that mode. */
	if (ts->mode == kCTLineBreakByCharWrapping) {
		return fit - startIndex;
	}
	/* **AND WORD WRAPPING BACKS OFF TO THE LAST OPPORTUNITY UAX #14 ALLOWS — WHICH IS WHAT A WORD BOUNDARY IS.**
	 * It used to back off to the last SPACE, and that is one rule of thirty-one: UAX #14 also allows a break after a
	 * hyphen, after a comma, after a closing quote, and BETWEEN IDEOGRAPHS, and it forbids one before a closing
	 * bracket or inside a word.
	 *
	 * **AND `SA` IS THE ONE CLASS THIS DOES NOT HELP, WHICH IS THE SPECIFICATION'S OWN ANSWER RATHER THAN THIS
	 * TIER'S GAP.** Thai, Lao, Khmer and friends are `SA` — "South-East Asian:... require morphological analysis to
	 * determine break opportunities" — and LB1 sends `SA` to `AL`, whose rule (LB28) forbids a break between two
	 * alphabetic characters. So a run of Thai is one unbreakable unit under the standard rules and NEEDS A
	 * DICTIONARY, which is what ICU's dictionary-based break iterator and the spec's own `SA` tailoring exist for.
	 * This tier has the first half (`CTLineBreakThai.c`: the dictionary) and not the second, so Thai still does not
	 * wrap THROUGH HERE — and saying so is the point, because the opposite claim is the
	 * one a reader would assume from "UAX #14 is implemented now". A script UAX #14 DOES handle without a
	 * dictionary — Han, Hiragana, Katakana, Hangul, and the scripts whose breaks are punctuation — wraps here.
	 *
	 * THE TRAILING SPACE STILL STAYS WITH THE LINE IT ENDED, and that is a property of the rules rather than a
	 * coincidence: LB18 is `SP ÷`, a break AFTER the spaces, so the run of spaces that ends a line is inside it —
	 * which is why `CTLineGetTrailingWhitespaceWidth` exists at all.
	 *
	 * **AND THE CLUSTER DOOR BELOW DOES NOT GET THIS**, which is also a property: Apple's own sentence for it is
	 * "no other contextual analysis will be done", and UAX #14 is exactly that analysis. The two doors therefore
	 * differ in the way Apple names, and the case that measures them still holds — a line door that breaks at a word
	 * boundary and a cluster door that does not. */
	opportunity = ct_lb_break_before_utf16(ts->chars, ts->count, startIndex, fit);
	/* **AND WHERE UAX #14 HAS NO OPINION, A HYPHENATION POINT IS THE ONLY BREAK THAT IS NOT A CUT INSIDE A WORD.**
	 * The two cases are different questions and the answer is taken for a different reason in each:
	 *
	 *   - NO OPPORTUNITY AT ALL: the fallback below would cut inside a word, which is where a line must never break.
	 *     If the caller asked for hyphenation, the hyphenation point IS the honest answer, and the zone test does not
	 *     apply — the line is nearly FULL here, so "too empty to be worth it" cannot be what rules it out.
	 *   - AN OPPORTUNITY THAT LEAVES THE LINE TOO EMPTY: the plain answer is legal and the line is badly filled, so
	 *     the factor decides. That is the AppKit's own reading of the number — `lineWidth < factor * width` — and it
	 *     is applied here in the same terms: the width the break would give up to, against the factor's share.
	 *
	 * MEASURED, both: a word too long for the width comes back at its hyphenation point, and the same word with the
	 * factor at zero comes back at the overflow, which is what this door answered before any of this existed. */
	if (ts->hyphenationFactor > 0.0) {
		CFIndex point = ct_ts_hyphenation_point(ts, startIndex, fit, width);

		if (opportunity <= startIndex) {
			if (point > startIndex) {
				return point - startIndex;
			}
		} else if (point > opportunity
			   && ct_ts_width_to(ts, startIndex, opportunity) < ts->hyphenationFactor * width) {
			return point - startIndex;
		}
	}
	if (opportunity > startIndex) {
		return opportunity - startIndex;
	}
	/* "UNLESS THE WORD ITSELF DOESN'T FIT ON A SINGLE LINE", which is Apple's own exception for this mode: with no
	 * opportunity to back off to, the line takes as much of the word as fits. */
	return (fit > startIndex) ? (fit - startIndex) : 1;
}

/* AND THE CLUSTER DOOR IS THE CHARACTER DOOR, WHICH IS WHAT APPLE SAYS IT IS: "similar to a character break, except
 * that it will not break apart linguistic clusters. NO OTHER CONTEXTUAL ANALYSIS WILL BE DONE." So it reads NO
 * paragraph style — the mode above is exactly the contextual analysis this door disclaims — and it never backs off to
 * a space. Its one difference from `SuggestLineBreak` in THIS tier is therefore the one Apple names. */
CFIndex CTTypesetterSuggestClusterBreak(CTTypesetterRef typesetter, CFIndex startIndex, double width)
{
	const struct __CTTypesetter *ts = (const struct __CTTypesetter *)typesetter;
	CFIndex hard, fit;

	if (ts == NULL || startIndex < 0 || startIndex >= ts->count) {
		return 0;
	}
	fit = ct_ts_fit(ts, startIndex, width);
	hard = ct_ts_hard_break_reached(ts, startIndex, ct_ts_hard_break(ts, startIndex), fit);
	if (hard > 0) {
		return hard;
	}
	return (fit > startIndex) ? (fit - startIndex) : 1;
}
