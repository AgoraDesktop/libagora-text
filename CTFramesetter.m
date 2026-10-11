/*
 * CTFramesetter.m — the paragraph's layout loop, and the one place a frame is filled in.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE LOOP, AND WHY EVERY DECISION IN IT IS WHERE IT IS:
 *
 *   1. **THE BOX COMES FROM THE PATH, AND IT IS THE PATH'S OWN BOUNDING BOX** (`CGPathGetPathBoundingBox`,
 *      the tight one — `CGPathGetBoundingBox` "INCLUDES THE CONTROL POINTS", which for text is the wrong
 *      box). A NULL path is an empty box, which lays out one line and stops.
 *   2. **THE PEN DESCENDS FROM THE BOX'S TOP, AND THE LINES' ORIGINS ARE THE BOX'S LEFT EDGE** — Apple's
 *      sentence for `CTFrameGetLineOrigins` (relative to the frame's path, y up), and the reason a caller
 *      needs no arithmetic: the origins are already in the box's own coordinates.
 *   3. **THE HEIGHT OF A LINE IS `ascent + descent + leading` FROM THE LINE ITSELF**, measured through
 *      `CTLineGetTypographicBounds` — so a paragraph of mixed fonts steps by the LINE's height and not by
 *      one font's, which is what the maxima in that door are for.
 *   4. **A LINE THAT WOULD LEAVE THE BOX ENDS THE LAYOUT**, except that the FIRST line is always kept (see
 *      `CTFramesetter.h`). `CTFrameGetVisibleStringRange` is where that stop is reported.
 *   5. **WHITESPACE AT THE START OF THE NEXT LINE IS SKIPPED**, because the break lands *after* a space
 *      (`CTTypesetter.h`) and a wrapped line must not begin with one — the single line-breaking courtesy
 *      this tier adds beyond the break itself, and it is here rather than in the typesetter because only the
 *      layout knows that a new line is starting.
 */
#import <Foundation/Foundation.h>

#include "CTFramesetter.h"
#include "CTFrame_internal.h"
#include <CoreText/CTLine.h>
#include <CoreText/CTParagraphStyle.h>
#include <CoreText/CTStringAttributes.h>
#include <CoreText/CTTypesetter.h>
#include <CoreGraphics/CGPath.h>

#include <stdlib.h>
#include <string.h>

@interface CTFramesetter : NSObject {
	@public
	id string;		/* the NSAttributedString the layout is made of, RETAINED */
}
@end

@implementation CTFramesetter

- (void)dealloc
{
	[string release];
	[super dealloc];
}

@end

CTFramesetterRef CTFramesetterCreateWithAttributedString(NSAttributedString *string)
{
	CTFramesetter *framesetter;

	if (string == nil || [string length] == 0) {
		return NULL;
	}
	/* The string must name a font, exactly as a line's must: the typesetter below enforces it as well, but a
	 * refusal here is one the caller can act on before any layout is attempted. */
	if (CTTypesetterCreateWithAttributedString(string) == NULL) {
		return NULL;
	}
	framesetter = [[CTFramesetter alloc] init];
	framesetter->string = [string retain];
	return (CTFramesetterRef)framesetter;
}

CTTypesetterRef CTFramesetterGetTypesetter(CTFramesetterRef framesetter)
{
	if (framesetter == NULL) {
		return NULL;
	}
	return CTTypesetterCreateWithAttributedString((NSAttributedString *)framesetter->string);
}

/* **A PARAGRAPH'S NUMBERS, READ ONCE AND READ BY BOTH DOORS IN THIS FILE.** CT-M7d added the MEASURING door
 * below, and it needs the same indents, the same height rules and the same spacing the layout uses — so the
 * reading is ONE function rather than two, because two readers of one paragraph style is exactly the shape of
 * bug this tree removes: the layout would use one set of indents and the measurement another, and the two would
 * disagree about how wide a line is while both being "right" about their own copy.
 *
 * THE DEFAULTS ARE WHAT AN ABSENT SETTING MEANS, and they are set BEFORE the reads rather than after:
 * `CTParagraphStyleGetValueForSpecifier` answers false and writes NOTHING for a mismatched buffer size, so a
 * default written afterwards would be a default that could not survive a mismatch. */
struct ct_para_numbers {
	CTTextAlignment alignment;
	double first_indent;
	int first_set;
	double head_indent;
	double tail_indent;
	double height_multiple;
	double min_height;
	double max_height;
	double line_spacing;
	double space_before;
};

static void ct_read_paragraph_numbers(NSAttributedString *string, NSUInteger at, struct ct_para_numbers *out)
{
	NSDictionary *attrs;
	CTParagraphStyleRef style;

	out->alignment = kCTNaturalTextAlignment;
	out->first_indent = 0.0;
	out->first_set = 0;
	out->head_indent = 0.0;
	out->tail_indent = 0.0;
	out->height_multiple = 0.0;
	out->min_height = 0.0;
	out->max_height = 0.0;
	out->line_spacing = 0.0;
	out->space_before = 0.0;
	if (string == nil || at > [string length]) {
		return;
	}
	/* **APPLE'S MODEL: A PARAGRAPH STYLE BELONGS TO A RANGE, AND THE LAYOUT IS OF A PARAGRAPH** — which is why
	 * this is read ONCE, at the paragraph's start, rather than per line. */
	attrs = [string attributesAtIndex:at effectiveRange:NULL];
	style = (CTParagraphStyleRef)[attrs objectForKey:kCTParagraphStyleAttributeName];
	if (style == NULL) {
		return;
	}
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierAlignment,
					     sizeof out->alignment, &out->alignment);
	/* **THE INDENTS, WHOSE SEMANTICS ARE THE ERA'S OWN WORDS** (`CTParagraphStyle.h`): `FirstLineHeadIndent` is
	 * "the distance in points from the leading margin of a frame to" the first line's start, `HeadIndent` the
	 * same for every line, and **`TailIndent` is the one with a SIGN: "if positive, this value is the distance
	 * from the LEADING margin ... If 0 or negative, it's the distance from the TRAILING margin"** — which is why
	 * it is turned into an END POSITION by the callers and not kept as a width. */
	out->first_set = CTParagraphStyleGetValueForSpecifier(
		style, kCTParagraphStyleSpecifierFirstLineHeadIndent, sizeof out->first_indent, &out->first_indent)
				 ? 1 : 0;
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierHeadIndent,
					     sizeof out->head_indent, &out->head_indent);
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierTailIndent,
					     sizeof out->tail_indent, &out->tail_indent);
	/* AND THE HEIGHTS, whose ORDER is the era's sentence: see `ct_line_height`. */
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierLineHeightMultiple,
					     sizeof out->height_multiple, &out->height_multiple);
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierMinimumLineHeight,
					     sizeof out->min_height, &out->min_height);
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierMaximumLineHeight,
					     sizeof out->max_height, &out->max_height);
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierLineSpacing,
					     sizeof out->line_spacing, &out->line_spacing);
	CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierParagraphSpacingBefore,
					     sizeof out->space_before, &out->space_before);
}

/* **AND HOW TALL A LINE IS, IN THE ERA'S OWN ORDER**: the natural height is "multiplied by this factor (if
 * positive) BEFORE being constrained by minimum and maximum line height", and `LineSpacing` is added
 * afterwards. A `MinimumLineHeight` above the natural one is what makes a paragraph of one font step evenly,
 * which is the check the probes make — and now BOTH doors make it through this one function. */
static double ct_line_height(const struct ct_para_numbers *p, double ascent, double descent, double leading)
{
	double natural = ascent + descent + leading;
	double height = natural;

	if (p->height_multiple > 0.0) {
		height = natural * p->height_multiple;
	}
	if (p->min_height > 0.0 && height < p->min_height) {
		height = p->min_height;
	}
	if (p->max_height > 0.0 && height > p->max_height) {
		height = p->max_height;
	}
	return height + p->line_spacing;
}

CTFrameRef CTFramesetterCreateFrame(CTFramesetterRef framesetter, NSRange stringRange, CGPathRef path,
				    NSDictionary *frameAttributes)
{
	NSAttributedString *string;
	CTTypesetterRef typesetter;
	CTFrame *frame;
	NSMutableArray *lines;
	CGPoint *origins;
	CTTextAlignment paragraph_alignment = kCTNaturalTextAlignment;
	/* THE PARAGRAPH'S NUMBERS, EACH DEFAULTING TO WHAT AN ABSENT SETTING MEANS (see the read block). */
	double para_first_indent = 0.0;
	int para_first_set = 0;
	double para_head_indent = 0.0;
	double para_tail_indent = 0.0;
	double para_height_multiple = 0.0;
	double para_min_height = 0.0;
	double para_max_height = 0.0;
	double para_line_spacing = 0.0;
	double para_space_before = 0.0;
	NSString *text;
	const char *utf8;
	CGRect box;
	double y;
	long start;
	long end;
	NSUInteger cap;
	NSUInteger count = 0;

	/* **THE ATTRIBUTES ARE KEPT, AND THE KEYS ARE STILL UNSPELLABLE** — which is the distinction M6p draws:
	 * this tier cannot NAME Apple's `kCTFrame…` keys, so it cannot act on a frame-attributes dictionary, but
	 * the dictionary the caller passed is the dictionary `CTFrameGetFrameAttributes` must answer, and a door
	 * that threw it away could not. It is retained by the frame, not read here. */
	if (framesetter == NULL) {
		return NULL;
	}
	/* **THE ONE FRAME ATTRIBUTE THIS TIER ACTS ON (M7d), AND THE ONE IT REFUSES.** Apple's dictionary's other
	 * keys are ones this tier cannot spell; this key it can, and its `RightToLeft` value means a VERTICAL layout
	 * ("lines stacked right to left FOR VERTICAL TEXT", rotated 90 degrees counterclockwise when drawing), which
	 * this tier does not do. So the frame is REFUSED rather than stacked the wrong way — the rule `CTFrame.h`
	 * argues for and the three other vertical refusals already state. An absent key, or `TopToBottom`, lays out. */
	if (frameAttributes != nil &&
	    [[frameAttributes objectForKey:kCTFrameProgressionAttributeName] intValue] ==
		    (int)kCTFrameProgressionRightToLeft) {
		return NULL;
	}
	string = (NSAttributedString *)framesetter->string;
	text = [string string];
	utf8 = [text UTF8String];
	if (utf8 == NULL) {
		return NULL;
	}
	box = (path != NULL) ? CGPathGetPathBoundingBox(path) : CGRectMake(0.0, 0.0, 0.0, 0.0);
	start = stringRange.location;
	end = stringRange.location + stringRange.length;
	if (end > (long)[string length]) {
		end = (long)[string length];
	}
	/* **THE PARAGRAPH'S NUMBERS, READ ONCE** — through the shared reader, so this door and the measuring one
	 * below cannot disagree about a paragraph (`ct_read_paragraph_numbers` has the reason). The locals are kept
	 * because the layout's own arithmetic below already speaks in them. */
	{
		struct ct_para_numbers para;

		ct_read_paragraph_numbers(string, stringRange.location, &para);
		paragraph_alignment = para.alignment;
		para_first_indent = para.first_indent;
		para_first_set = para.first_set;
		para_head_indent = para.head_indent;
		para_tail_indent = para.tail_indent;
		para_height_multiple = para.height_multiple;
		para_min_height = para.min_height;
		para_max_height = para.max_height;
		para_line_spacing = para.line_spacing;
		para_space_before = para.space_before;
	}
	typesetter = CTTypesetterCreateWithAttributedString(string);
	if (typesetter == NULL) {
		return NULL;
	}
	/* ONE LINE PER CHARACTER IS THE WORST CASE, AND THAT IS THE CAPACITY: a box narrow enough to fit one
	 * character per line is a legal (if silly) input, and a realloc per line is not worth avoiding it. */
	cap = (NSUInteger)(end - start) + 1;
	origins = (CGPoint *)malloc(cap * sizeof(CGPoint));
	lines = [NSMutableArray array];
	/* **THE FIRST LINE'S INDENT IS A LAYOUT FACT, so the pen starts below `ParagraphSpacingBefore`** — and
	 * the indents themselves are applied per line below, because the first line's differs from the rest. */
	y = box.size.height - para_space_before;
	while (start < end && origins != NULL) {
		/* **THE LINE'S START AND ITS AVAILABLE WIDTH, WHICH IS WHERE THE THREE INDENTS MEET**: the start is
		 * the first-line indent for the FIRST line and the head indent after that, and the end is the tail
		 * indent's own position (positive: from the leading margin; 0 or negative: from the trailing one). */
		/* **THE FIRST LINE TAKES ITS OWN INDENT IF IT HAS ONE, AND THE HEAD INDENT OTHERWISE** — which is
		 * Apple's relationship between the two settings, and the bug this probe's own note caught: the first
		 * version used the first-line indent UNCONDITIONALLY (so an unset one, zero, wiped out a set head
		 * indent for the first line). The absence of a setting is not a value of zero. */
		double line_indent = (count == 0 && para_first_set) ? para_first_indent : para_head_indent;
		double tail_x = (para_tail_indent > 0.0)
					? (box.origin.x + para_tail_indent)
					: (box.origin.x + box.size.width + para_tail_indent);
		double line_width_avail = tail_x - (box.origin.x + line_indent);
		long brk;

		if (line_width_avail < 1.0) {
			line_width_avail = 1.0;	/* an over-indented paragraph still lays out one narrow line at a time */
		}
		brk = CTTypesetterSuggestLineBreak(typesetter, start, line_width_avail);
		CTLineRef line;
		CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;
		double line_height;

		if (brk <= start) {
			break;	/* nothing can advance: stop rather than spin */
		}
		if (brk > end) {
			brk = end;
		}
		line = CTTypesetterCreateLine(typesetter, NSMakeRange((NSUInteger)start, (NSUInteger)(brk - start)));
		if (line == NULL) {
			break;
		}
		CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		{
			struct ct_para_numbers para;

			para.height_multiple = para_height_multiple;
			para.min_height = para_min_height;
			para.max_height = para_max_height;
			para.line_spacing = para_line_spacing;
			line_height = ct_line_height(&para, (double)ascent, (double)descent, (double)leading);
		}
		if (count > 0 && y - line_height < 0.0) {
			CTLineRelease(line);	/* the first line is always kept; later ones must fit */
			break;
		}
		y -= (double)ascent;	/* the baseline is the ascent below the line's top */
		/* **AND THE ALIGNMENT MOVES THE LINE WITHIN THE BOX** — the half of M4's plan proof that says
		 * "alignment honoured". It is a per-LINE x offset, which is exactly why justification is refused by
		 * name: justified text moves GLYPHS within a line, not the line within the box. */
		{
			double line_width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
			double left = box.origin.x + line_indent;
			double available = tail_x - left;
			double x = left;

			if (paragraph_alignment == kCTRightTextAlignment) {
				x = left + available - line_width;
			} else if (paragraph_alignment == kCTCenterTextAlignment) {
				x = left + (available - line_width) / 2.0;
			}
			origins[count] = CGPointMake((CGFloat)x, (CGFloat)y);
		}
		[lines addObject:line];
		[line release];		/* the array holds it */
		count++;
		y -= (line_height - (double)ascent);	/* the rest of the line's height, ascent already spent */
		start = brk;
		/* AND THE NEXT LINE DOES NOT BEGIN WITH THE SPACE THE BREAK LANDED AFTER (see the file comment). */
		while (start < end && (utf8[start] == ' ' || utf8[start] == '\t' || utf8[start] == '\n' ||
				       utf8[start] == '\r')) {
			start++;
		}
	}
	CTTypesetterRelease(typesetter);
	frame = [[CTFrame alloc] init];
	frame->lines = [lines retain];
	frame->origins = origins;
	frame->line_count = (long)count;
	frame->string_range = stringRange;
	frame->visible_range = NSMakeRange(stringRange.location, (NSUInteger)(start - stringRange.location));
	frame->box = box;
	/* AND THE TWO THINGS THE FRAME IS NOW ASKED FOR (M6p), retained for as long as it lives. */
	frame->path = (CGPathRef)CGPathRetain(path);
	frame->frame_attributes = [frameAttributes retain];
	return (CTFrameRef)frame;
}

/* **M7d: THE MEASURING DOOR — THE WRAP RUN BACKWARDS.** It asks the SAME questions the layout asks, in the
 * same order, and keeps no lines: how wide is the line that starts here, how tall is it under this paragraph's
 * own height rules, and does it still fit the height constraint. That is what makes the measuring door and the
 * frame agree rather than merely agree on the easy cases, and the probe checks the agreement directly — a frame
 * built over the range this door reports must fit the size it reports.
 *
 * **IT REPORTS WHAT ACTUALLY FIT, WITH NO FIRST-LINE EXCEPTION**, which is Apple's own wording for `fitRange`
 * ("the range of the string that actually fit in the constrained size") and a deliberate difference from
 * `CTFramesetterCreateFrame`, whose rule that the first line is always kept is about giving a caller the REASON
 * in the answer. A measuring door that reported a size larger than the constraint would be answering a different
 * question than the one it was asked.
 *
 * AND THE WIDTH IS THE WIDEST LINE ACTUALLY USED rather than the constraint echoed back, because "the actual
 * dimensions" is what the era asks for — and because a door that echoed its input could not be checked at all.
 * `CGFLOAT_MAX` in either dimension means that dimension is unconstrained, which is how a caller asks "how
 * tall would this be at this width". */
CGSize CTFramesetterSuggestFrameSizeWithConstraints(CTFramesetterRef framesetter, NSRange stringRange,
						    NSDictionary *frameAttributes, CGSize constraints,
						    NSRange *fitRange)
{
	NSAttributedString *string;
	CTTypesetterRef typesetter;
	struct ct_para_numbers para;
	NSString *text;
	const char *utf8;
	double width_limit, height_limit, y, max_width;
	long start, end, lines_placed;

	if (fitRange != NULL) {
		*fitRange = NSMakeRange(stringRange.location, 0);
	}
	if (framesetter == NULL) {
		return CGSizeMake(0.0, 0.0);
	}
	/* THE SAME REFUSAL THE FRAME MAKES, for the same reason: a caller measuring for a vertical layout must not
	 * be handed a size that looks like an answer. */
	if (frameAttributes != nil &&
	    [[frameAttributes objectForKey:kCTFrameProgressionAttributeName] intValue] ==
		    (int)kCTFrameProgressionRightToLeft) {
		return CGSizeMake(0.0, 0.0);
	}
	string = (NSAttributedString *)framesetter->string;
	text = [string string];
	utf8 = [text UTF8String];
	if (utf8 == NULL) {
		return CGSizeMake(0.0, 0.0);
	}
	start = stringRange.location;
	/* **A ZERO-LENGTH RANGE MEANS THE REST OF THE STRING** — Apple's own sentence for this door ("the
	 * framesetter will continue to add lines until it runs out of text or space"), and the same reading the
	 * frame's own loop gives a range that runs past the end. */
	end = (stringRange.length > 0) ? (stringRange.location + (long)stringRange.length) : (long)[string length];
	if (end > (long)[string length]) {
		end = (long)[string length];
	}
	ct_read_paragraph_numbers(string, stringRange.location, &para);
	typesetter = CTTypesetterCreateWithAttributedString(string);
	if (typesetter == NULL) {
		return CGSizeMake(0.0, 0.0);
	}
	width_limit = (constraints.width >= CGFLOAT_MAX) ? (double)CGFLOAT_MAX : (double)constraints.width;
	height_limit = (constraints.height >= CGFLOAT_MAX) ? (double)CGFLOAT_MAX : (double)constraints.height;
	/* THE SPACING BEFORE THE PARAGRAPH IS PART OF ITS HEIGHT, exactly as it is in the frame — where the pen
	 * starts below it. */
	y = para.space_before;
	max_width = 0.0;
	lines_placed = 0;
	while (start < end) {
		/* THE SAME THREE INDENTS THE LAYOUT USES, with the box's origin at zero because a measurement has no
		 * box: the tail indent becomes an END POSITION in the constraint's coordinates. */
		double line_indent = (lines_placed == 0 && para.first_set) ? para.first_indent : para.head_indent;
		double tail_x = (para.tail_indent > 0.0)
					? para.tail_indent
					: ((width_limit >= CGFLOAT_MAX) ? (double)CGFLOAT_MAX
									: width_limit + para.tail_indent);
		double avail = (tail_x >= CGFLOAT_MAX) ? (double)CGFLOAT_MAX : (tail_x - line_indent);
		long brk;
		CTLineRef line;
		CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;
		double line_width, line_height;

		if (avail < 1.0) {
			avail = 1.0;
		}
		brk = CTTypesetterSuggestLineBreak(typesetter, start, avail);
		if (brk <= start) {
			break;
		}
		if (brk > end) {
			brk = end;
		}
		line = CTTypesetterCreateLine(typesetter, NSMakeRange((NSUInteger)start, (NSUInteger)(brk - start)));
		if (line == NULL) {
			break;
		}
		line_width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
		line_height = ct_line_height(&para, (double)ascent, (double)descent, (double)leading);
		CTLineRelease(line);
		if (y + line_height > height_limit) {
			break;	/* this line does not fit, so the measurement ends where the layout would end */
		}
		y += line_height;
		if (line_width > max_width) {
			max_width = line_width;
		}
		lines_placed++;
		start = brk;
		/* AND THE NEXT LINE DOES NOT BEGIN WITH THE SPACE THE BREAK LANDED AFTER — the same courtesy the
		 * layout pays, from the same rule. */
		while (start < end && (utf8[start] == ' ' || utf8[start] == '\t' || utf8[start] == '\n' ||
				       utf8[start] == '\r')) {
			start++;
		}
	}
	CTTypesetterRelease(typesetter);
	if (fitRange != NULL) {
		*fitRange = NSMakeRange(stringRange.location, (NSUInteger)(start - stringRange.location));
	}
	return CGSizeMake((CGFloat)max_width, (CGFloat)y);
}

CTFramesetterRef CTFramesetterRetain(CTFramesetterRef framesetter)
{
	return (CTFramesetterRef)[(id)(uintptr_t)framesetter retain];
}

void CTFramesetterRelease(CTFramesetterRef framesetter)
{
	[(id)(uintptr_t)framesetter release];
}
