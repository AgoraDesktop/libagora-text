/*
 * CTTypesetter.c — the typesetter: the first thing in this library that DECIDES rather than measures.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
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
 * change the line this typesetter makes. NO LINE-BREAKING RULES BEYOND SPACES (nothing of UAX #14, no hyphenation), so a script whose words are not space-separated breaks at the
 * width and no sooner.
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
	return da.forcedLevel == db.forcedLevel && da.disabled == db.disabled;
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
static CFIndex ct_ts_fit(const struct __CTTypesetter *ts, CFIndex start, double width)
{
	double x = 0.0;
	CFIndex i = start;

	while (i < ts->count) {
		double step;

		/* **AND A TAB TAKES THE FIT TO ITS STOP.** A tab's advance is not in `ts->advances` — it depends on where the
		 * pen already is — so the fit computes it the same way `CTLine` does, from the same stops, and the two agree
		 * about where the line ends. */
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
			step = stop - x;
		} else {
			step = ts->advances[i].width;
			/* **AND THE PAIR'S ADJUSTMENT, BECAUSE THE LINE's OWN WIDTH HAS IT AND THE TWO MUST AGREE.** An advance
			 * is what a glyph asks for on its own; a KERN is what the font says about the two glyphs TOGETHER, and
			 * `CTLine` applies it (see `CTLine.c`). A typesetter that measured without it would break a line at a
			 * DIFFERENT place than the line then draws it. A box has no glyph to adjust, so the kern is skipped. */
			if (i > start && !(ts->isBox != NULL && (ts->isBox[i - 1] || ts->isBox[i]))) {
				step += (double)CTFontGetKerningBetweenGlyphs(ts->font, ts->glyphs[i - 1], ts->glyphs[i]);
			}
		}
		if (x + step > width) {
			break;
		}
		x += step;
		i++;
	}
	return i;
}

/* THE HARD BREAK, WHICH COMES FIRST AND WINS WHEREVER IT IS. Apple's sentence puts the two triggers side by side
 * ("either by a hard break character in the stream or by filling the specified width"), and a newline does not stop
 * being one because the width had room to spare. THE BREAK CHARACTER IS PART OF THE LINE, which is what makes a line
 * built from this answer carry its newline — and what lets the caller's loop advance past it. */
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

CFIndex CTTypesetterSuggestLineBreak(CTTypesetterRef typesetter, CFIndex startIndex, double width)
{
	const struct __CTTypesetter *ts = (const struct __CTTypesetter *)typesetter;
	CFIndex hard, fit, i;

	if (ts == NULL || startIndex < 0 || startIndex >= ts->count) {
		return 0;
	}
	hard = ct_ts_hard_break(ts, startIndex);
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
	fit = ct_ts_fit(ts, startIndex, width);
	if (fit >= ts->count) {
		return ts->count - startIndex;
	}
	/* A CHARACTER BREAK IS WHERE THE FIRST CHARACTER THAT DOES NOT FIT BEGINS — Apple's sentence for that mode. */
	if (ts->mode == kCTLineBreakByCharWrapping) {
		return fit - startIndex;
	}
	/* AND WORD WRAPPING BACKS OFF TO THE LAST SPACE THAT FITS, WHICH IT THEN INCLUDES: a line's trailing whitespace
	 * is part of its range — that is why `CTLineGetTrailingWhitespaceWidth` exists at all — so the space stays with
	 * the line it ended. */
	for (i = fit; i > startIndex; i--) {
		if (ct_ts_is_space(ts->chars[i - 1])) {
			return i - startIndex;
		}
	}
	/* "UNLESS THE WORD ITSELF DOESN'T FIT ON A SINGLE LINE", which is Apple's own exception for this mode: with no
	 * space to back off to, the line takes as much of the word as fits. */
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
	hard = ct_ts_hard_break(ts, startIndex);
	if (hard > 0) {
		return hard;
	}
	fit = ct_ts_fit(ts, startIndex, width);
	return (fit > startIndex) ? (fit - startIndex) : 1;
}
