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

#include <CoreFoundation/CFCharacterSet.h>
#include <CoreFoundation/CFRuntime.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------------------------
 * THE FONT ATTRIBUTE'S NAME, WHICH THIS TREE DOES NOT HAVE YET, AND THE ONE PLACE CTLine REACHES OUTSIDE ITSELF.
 * Apple's key is `kCTFontAttributeName`, declared in `CTStringAttributes.h` — MUCH LATER in the include order this
 * repository writes the headers in, so the constant does not exist. THE VALUE IS WRITTEN HERE RATHER THAN A
 * CONSTANT INVENTED: "NSFontAttributeName" is the value that header will carry (it is one of the keys the AppKit's
 * dictionary crosses on, which is the rule this tree measured), and THIS FUNCTION IS WHAT GOES AWAY the day
 * `CTStringAttributes.h` lands — nothing else in the library names the key.
 * ----------------------------------------------------------------------------------------------- */
static CFStringRef ct_line_font_attribute(void)
{
	return CFSTR("NSFontAttributeName");
}

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

/* THE ONE PLACE A LINE IS MADE. `font` is BORROWED by the caller and RETAINED here, so the creator below can hand
 * it straight over. */
static CTLineRef ct_line_create_with_font(CFAttributedStringRef string, CTFontRef font)
{
	struct __CTLine *line;
	CFStringRef text;
	CFRange whole;
	UniChar *chars;
	CGSize *advances;
	CFIndex n, i;
	double x = 0.0;

	text = CFAttributedStringGetString(string);
	n = CFStringGetLength(text);
	line = ct_line_alloc();
	if (line == NULL) {
		return NULL;
	}
	line->string = (CFAttributedStringRef)CFRetain(string);
	line->font = (CTFontRef)CFRetain(font);
	line->range = CFRangeMake(0, n);
	/* THE HEIGHTS ARE THE FONT'S AND NOT THE GLYPHS', which is what a framesetter needs: line after line placed
	 * without measuring glyph by glyph. They are read even for a line of nothing, so that a line still has a
	 * baseline to sit on. */
	line->ascent = CTFontGetAscent(font);
	line->descent = CTFontGetDescent(font);
	line->leading = CTFontGetLeading(font);
	if (n <= 0) {
		return (CTLineRef)line;
	}

	whole = CFRangeMake(0, n);
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
	CFStringGetCharacters(text, whole, chars);

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
	/* THE TRAILING WHITESPACE IS MEASURED BACKWARDS OVER THE CHARACTERS, because that is what the door is about:
	 * the part of the line's width that is invisible at its end. */
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
CTLineRef CTLineCreateWithAttributedString(CFAttributedStringRef string)
{
	CFTypeRef value;
	CFRange effective = CFRangeMake(0, 0);

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
	value = CFAttributedStringGetAttribute(string, 0, ct_line_font_attribute(), &effective);
	if (value == NULL || CFGetTypeID(value) != CTFontGetTypeID()) {
		return NULL;
	}
	return ct_line_create_with_font(string, (CTFontRef)value);
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
 * OWED DOORS, EACH WITH THE REASON IT CANNOT BE ANSWERED HERE. They are DECLARED in CTLine.h and NOT DEFINED, so a
 * caller who reaches for one gets a link error rather than an invented answer.
 *
 *   CTLineCreateTruncatedLine                     NEEDS THE TRUNCATION ENGINE, AND THE ENGINE IS THE DOOR. The token
 *                                                 is measurable here — it is a line, and this file makes lines — and
 *                                                 cutting is a choice of glyphs; but WHICH glyphs is documented
 *                                                 against rules Apple does not print: where an ellipsis may fall,
 *                                                 what happens when the token is wider than the width, and which
 *                                                 clusters may not be split. It waits for the typesetter, whose job
 *                                                 the breaking is.
 *
 *   CTLineCreateJustifiedLine                      NEEDS THE JUSTIFICATION ENGINE. `justificationFactor` scales how
 *                                                 the slack is distributed, and WHERE it goes — between words, at
 *                                                 which boundaries, and what negative justification does to the
 *                                                 glyphs — is a set of rules Apple states nowhere in the header.
 *                                                 Spreading the slack evenly between glyphs would look plausible
 *                                                 and be wrong, which is the failure this tree refuses to ship.
 *
 * AND `CTLineGetGlyphRuns` WAS THE THIRD, DECLARED HERE AND NOT DEFINED FOR AS LONG AS `CTRun` DID NOT EXIST — which
 * is why Apple's own include order puts CTRun.h AFTER CTLine.h. IT IS DEFINED ABOVE AS OF THIS COMMIT, and the
 * `owed` test lost a name at the same time: that test fails if an owed door IS defined, so the two moved together
 * and neither could have moved alone.
 * =============================================================================================== */
