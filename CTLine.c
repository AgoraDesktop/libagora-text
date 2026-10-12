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
 * SO IT IS ONE RUN, AND THE HEADER SAYS WHAT THAT COSTS. No line breaking, no two fonts in one line, no
 * bidirectional reordering and no shaping: a character's glyph is drawn where its own advance puts it, in the order
 * the characters appeared. That is stated in CTLine.h as the boundary it is, rather than left for a caller to
 * discover from a wrong layout.
 */
#include <CoreText/CTLine.h>

/* THE FONT OBJECT AND ITS DOORS, WHICH THIS FILE'S HEADER DOES NOT DECLARE AND MUST NOT: Apple's CTLine.h does not
 * include CTFont.h either — a LINE's surface mentions no font at all. The IMPLEMENTATION is a different question,
 * and this is where it answers it. */
#include <CoreText/CTFont.h>

/* AND THE RUN, WHICH A LINE IS MADE OF. `CTRun_internal.h` is NOT part of Core Text's surface — Apple has no public
 * creator for a run, because a line is what makes one — and it is included here so that the two files agree about
 * how a run is built instead of one of them `extern`ing what the other defines. */
#include <CoreText/CTRun_internal.h>

/* AND THIS FILE'S OWN INTERNAL HEADER, WHICH IS NOT A CIRCLE: it declares the two things a TYPESETTER needs from a
 * line — a line made over a RANGE, and the name of the font key — and including it here is what makes the
 * declarations and the definitions below the SAME functions rather than two files agreeing by convention. */
#include <CoreText/CTLine_internal.h>

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFRuntime.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 * THE OBJECT.
 * =============================================================================================== */
struct __CTLine {
	CFRuntimeBase base;
	CFAttributedStringRef string;	/* RETAINED: the string this line was made from */
	CTFontRef font;			/* RETAINED: the face the whole line is set in */
	CFRange range;			/* the characters that spawned the glyphs */
	CGGlyph *glyphs;		/* malloc'd, glyphCount entries */
	CGPoint *positions;		/* malloc'd, glyphCount entries, relative to the LINE's own origin */
	CFIndex glyphCount;
	double width;			/* the sum of the advances */
	CGFloat ascent, descent, leading;
	double trailingWhitespace;
	/* AND ITS ONE RUN, BUILT ON FIRST ASK RATHER THAN AT CREATION: a line that is only measured never pays for a
	 * run, and a line whose runs are asked for pays for exactly one. `runs` is the ARRAY the door hands out, kept
	 * so that two calls give a caller the same object rather than two arrays holding the same run. */
	CTRunRef run;			/* RETAINED once built */
	CFArrayRef runs;		/* RETAINED once built: one array holding that one run */
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
	if (line->run != NULL) {
		CFRelease(line->run);
	}
	if (line->runs != NULL) {
		CFRelease(line->runs);
	}
	free(line->glyphs);
	free(line->positions);
}

/* TWO LINES ARE EQUAL WHEN THEY WERE MADE FROM THE SAME STRING IN THE SAME FONT: the glyphs and their positions are
 * a FUNCTION of those two, so comparing them would be comparing the same fact twice. */
static Boolean ct_line_equal(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CTLine *a = (const struct __CTLine *)cf1;
	const struct __CTLine *b = (const struct __CTLine *)cf2;

	if ((a->string == NULL) != (b->string == NULL) || (a->font == NULL) != (b->font == NULL)) {
		return false;
	}
	if (a->string != NULL && !CFEqual(a->string, b->string)) {
		return false;
	}
	if (a->font != NULL && !CFEqual(a->font, b->font)) {
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
	if (line->font != NULL) {
		h = h * 31u + CFHash(line->font);
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
 * THE BUILD. One pass over the string, in one font.
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

/* THE ONE PLACE A LINE IS MADE, AND IT IS ALWAYS MADE OVER A RANGE. A whole-string line and a typeset line differ in
 * ONE number — where the glyphs begin — so they are one function with the range as a parameter rather than two
 * functions that could drift apart. `font` is BORROWED by the callers and RETAINED here. */
static CTLineRef ct_line_create_with_font(CFAttributedStringRef string, CTFontRef font, CFRange range)
{
	struct __CTLine *line;
	CFStringRef text;
	UniChar *chars;
	CGSize *advances;
	CFIndex n, i;
	double x = 0.0;

	text = CFAttributedStringGetString(string);
	n = range.length;
	line = ct_line_alloc();
	if (line == NULL) {
		return NULL;
	}
	line->string = (CFAttributedStringRef)CFRetain(string);
	line->font = (CTFontRef)CFRetain(font);
	/* THE RANGE IS THE STRING'S OWN AND STAYS THE STRING'S OWN, which is the whole point of the parameter: a line cut
	 * out of the MIDDLE of a document reports the range it covers THERE, so a caller laying out line after line never
	 * has to translate between the line's coordinates and the string's. */
	line->range = range;
	/* THE HEIGHTS ARE THE FONT'S AND NOT THE GLYPHS', which is what a framesetter needs: line after line placed
	 * without measuring glyph by glyph. They are read even for a line of nothing, so that a line still has a
	 * baseline to sit on. */
	line->ascent = CTFontGetAscent(font);
	line->descent = CTFontGetDescent(font);
	line->leading = CTFontGetLeading(font);
	if (n <= 0) {
		return (CTLineRef)line;
	}

	chars = (UniChar *)malloc((size_t)n * sizeof(UniChar));
	line->glyphs = (CGGlyph *)malloc((size_t)n * sizeof(CGGlyph));
	line->positions = (CGPoint *)malloc((size_t)n * sizeof(CGPoint));
	advances = (CGSize *)malloc((size_t)n * sizeof(CGSize));
	if (chars == NULL || line->glyphs == NULL || line->positions == NULL || advances == NULL) {
		free(chars);
		free(advances);
		/* THE LINE IS RELEASED RATHER THAN FIXED UP: it is a fresh Core Foundation object with one reference, so
		 * releasing it runs the finalizer, which frees whichever arrays DID arrive — one place that knows what a
		 * line owns, instead of a second list of frees here. */
		CFRelease((CFTypeRef)line);
		return NULL;
	}
	CFStringGetCharacters(text, range, chars);

	/* APPLE'S CONTRACT FOR THE GLYPH STEP IS A BOOLEAN, AND A LINE DOES NOT NEED IT: a character the font cannot
	 * encode becomes glyph 0, which IS `.notdef`, and Apple DRAWS `.notdef` for such a character rather than
	 * dropping it. So the answer is deliberately not checked, and the slot is drawn like any other. */
	(void)CTFontGetGlyphsForCharacters(font, chars, line->glyphs, n);

	memset(advances, 0, (size_t)n * sizeof(CGSize));
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, line->glyphs, advances, n);
	for (i = 0; i < n; i++) {
		line->positions[i] = CGPointMake((CGFloat)x, 0.0);
		x += advances[i].width;
	}
	line->glyphCount = n;
	line->width = x;
	/* THE TRAILING WHITESPACE IS MEASURED BACKWARDS OVER THE CHARACTERS THIS LINE HAS, because that is what the door
	 * is about: the part of the line's width that is invisible at its end. */
	for (i = n - 1; i >= 0 && ct_line_is_whitespace(chars[i]); i--) {
		line->trailingWhitespace += advances[i].width;
	}
	free(chars);
	free(advances);
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
	return ct_line_create_with_font(string, (CTFontRef)value, whole);
}

/* AND THE SECOND WAY IN, WHICH NO PUBLIC HEADER NAMES: a TYPESETTER makes lines out of PARTS of a string, and there is
 * no public door for that — so the two files agree about it in `CTLine_internal.h`, where its reason is written down.
 * The range is VALIDATED here rather than trusted: the callers are inside this library, but the bounds are still a
 * fact about the string, and a door that can be reached with a bad range must refuse rather than read past the end. */
CTLineRef CTLineCreateWithAttributedStringAndRange(CFAttributedStringRef string, CFRange range)
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
	return ct_line_create_with_font(string, (CTFontRef)value, range);
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
	CGFontRef graphics;
	CGPoint *absolute;
	CGPoint pen;
	CFIndex i;

	if (l == NULL || context == NULL || l->glyphCount <= 0) {
		return;
	}
	graphics = CTFontCopyGraphicsFont(l->font, NULL);
	if (graphics == NULL) {
		return;
	}
	/* THE LINE'S ORIGIN IS THE CONTEXT'S TEXT POSITION, AND APPLYING IT IS THIS DOOR'S JOB BECAUSE *THIS* CORE
	 * GRAPHICS DOES NOT: `CGContextShowGlyphsAtPositions` takes positions that are "specified in user space" and
	 * says in its own header that "the CTM alone places the pen" — UNLIKE the three advance doors, whose pen
	 * "starts at the TEXT POSITION". So the pen offset is added here, and the suite MEASURES it by drawing the same
	 * line at two text positions and looking at where the ink landed: a comment cannot tell those two readings
	 * apart, and a picture can. */
	pen = CGContextGetTextPosition(context);
	absolute = (CGPoint *)malloc((size_t)l->glyphCount * sizeof(CGPoint));
	if (absolute == NULL) {
		CGFontRelease(graphics);
		return;
	}
	for (i = 0; i < l->glyphCount; i++) {
		absolute[i] = CGPointMake(l->positions[i].x + pen.x, l->positions[i].y + pen.y);
	}
	/* THE FONT AND ITS SIZE GO ON THE CONTEXT because the glyphs are drawn BY Core Graphics and not by this
	 * library: a glyph index means nothing without the face and the size it was indexed in. */
	CGContextSetFont(context, graphics);
	CGContextSetFontSize(context, CTFontGetSize(l->font));
	CGContextShowGlyphsAtPositions(context, l->glyphs, absolute, (size_t)l->glyphCount);
	free(absolute);
	CGFontRelease(graphics);
}

/* =================================================================================================
 * CARET POSITIONING.
 * =============================================================================================== */
/* A GLYPH'S OWN WIDTH, RECOVERED FROM THE POSITIONS THEMSELVES: it is the distance to the next glyph, and the last
 * one's is what is left of the width. One function because BOTH caret doors need it, and a second copy of the rule
 * is a second chance to get it wrong. */
static CGFloat ct_line_advance_at(const struct __CTLine *line, CFIndex i)
{
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
	if (charIndex <= 0) {
		offset = 0.0;
	} else if (charIndex >= l->glyphCount) {
		/* ONE PAST THE LAST CHARACTER IS THE END OF THE LINE, which is where a caret at the end belongs. */
		offset = (CGFloat)l->width;
	} else {
		offset = l->positions[charIndex].x;
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
/* ONE ARRAY, ONE RUN, BUILT ON FIRST ASK AND KEPT. APPLE'S NAME FOR THIS DOOR IS `Get` AND NOT `Copy`, and its
 * documentation says nothing about releasing — so what comes back is BORROWED, owned by the line, which is the
 * reading the name and the silence agree on. The array is built once and cached for exactly that reason: two calls
 * hand a caller the SAME object rather than two arrays holding the same run.
 *
 * AND THERE IS ONE RUN BECAUSE THERE IS ONE FONT. A run is, in CTRun.h's own words, "the piece of a line that shares
 * one set of attributes", and this tier lays a line out in one pass with one face — so the single run covers every
 * character the line does and its string range IS the line's. What a caller must NOT conclude from that is that a
 * line's runs are always one: the day a line can hold two fonts, THIS function is where that changes and nothing
 * else in the library has to.
 *
 * THE ADVANCES ARE RECOVERED FROM THE POSITIONS, because the line does not store them — it stores where each glyph
 * goes and how wide the whole thing is. For this tier's one-pass layout an advance IS the distance to the next
 * glyph's origin, which is the same rule the caret doors already use, so it is taken from that one place rather
 * than written a second time. */
CFArrayRef CTLineGetGlyphRuns(CTLineRef line)
{
	struct __CTLine *l = (struct __CTLine *)line;
	CFDictionaryRef attributes;
	CFIndex *indices;
	CGSize *advances;
	CFIndex i;

	if (l == NULL) {
		return NULL;
	}
	if (l->runs != NULL) {
		return l->runs;
	}
	/* THE STRING INDICES ARE THE RUN'S FACT AND ARE BUILT HERE: one glyph per character, in order, so the index
	 * array is 0..glyphCount-1. A line that shaped its text would read them from the shaper instead, and this is
	 * the place that would change. */
	indices = (CFIndex *)malloc((size_t)((l->glyphCount > 0) ? l->glyphCount : 1) * sizeof(CFIndex));
	advances = (CGSize *)malloc((size_t)((l->glyphCount > 0) ? l->glyphCount : 1) * sizeof(CGSize));
	if (indices == NULL || advances == NULL) {
		free(indices);
		free(advances);
		return NULL;
	}
	for (i = 0; i < l->glyphCount; i++) {
		indices[i] = i;
		advances[i] = CGSizeMake(ct_line_advance_at(l, i), 0.0);
	}
	/* THE ATTRIBUTES ARE THE STRING'S OWN FOR THE RUN'S RANGE — Apple's sentence allows either that or a dictionary
	 * "manufactured by the layout engine" for font substitution or missing keys, and THIS TIER HAS NOTHING TO
	 * MANUFACTURE: the font came from these very attributes. */
	attributes = (l->string == NULL) ? NULL
					 : CFAttributedStringGetAttributes(l->string, l->range.location, NULL);
	l->run = CTRunCreateWithAttributesAndGlyphs(attributes, l->font, l->range, indices, l->glyphs,
						    l->positions, advances, l->glyphCount);
	free(indices);
	free(advances);
	if (l->run == NULL) {
		return NULL;
	}
	l->runs = CFArrayCreate(kCFAllocatorDefault, (const void **)&l->run, 1, &kCFTypeArrayCallBacks);
	if (l->runs == NULL) {
		return NULL;
	}
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
 * ONE BOUNDARY IS RECORDED RATHER THAN GUESSED, AND IT IS THE FONT. A LINE HERE HOLDS ONE FONT, so a TOKEN DRAWN IN
 * ANOTHER FACE cannot be added to it: the token's glyph INDICES are indices IN ITS OWN FONT, and inserting them into a
 * line drawn with the other one would put garbage where an ellipsis belongs — a wrong answer that LOOKS like a
 * truncation, which is the kind this tree refuses. Apple's answer is a line with several runs, and this tier's is to
 * REFUSE; the case holds the refusal. A token in the SAME face — the ellipsis it usually is — works, and that is the
 * case the door was written for.
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
