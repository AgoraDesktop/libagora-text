/*
 * CTTypesetter.m — the typesetter class, and the greedy word wrap.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE MEASURING INSTRUMENT IS ALREADY BUILT: A LINE.** `CTTypesetterSuggestLineBreak` could shape the
 * string itself, but the line already does — with the right fonts per attribute range, with kerning and
 * ligatures on — so this file shapes the string ONCE through `CTLineCreateWithAttributedString` and reads
 * the answer out of the glyphs it produced. That is also what makes the answer CONSISTENT with the line the
 * caller will actually create for the range: both come out of one shaper over one string.
 *
 * THE WALK, IN FOUR STEPS, AND EACH IS A SENTENCE OF THE CONTRACT RATHER THAN A TRICK:
 *
 *   1. find the first glyph whose cluster is at or after `start` (a byte offset, the units `CTRun.h`
 *      records), and accumulate advances from there;
 *   2. the last glyph whose END is within `width` names the byte where the line could stop;
 *   3. **back up to the last whitespace at or before that byte**, because the door's contract is that the
 *      break lands after whitespace when whitespace fits — this is the whole of the word wrap;
 *   4. and never return `start` itself: a caller looping on this door must always advance, so an
 *      over-long word breaks at the next glyph's cluster rather than at nothing.
 */
#import <Foundation/Foundation.h>

#include "CTTypesetter.h"
#include "CTRun_internal.h"
#include <CoreText/CTFont.h>
#include <CoreText/CTStringAttributes.h>

#include <string.h>

@interface CTTypesetter : NSObject {
	@public
	id string;		/* the NSAttributedString every door measures, RETAINED */
}
@end

@implementation CTTypesetter

- (void)dealloc
{
	[string release];
	[super dealloc];
}

@end

CTTypesetterRef CTTypesetterCreateWithAttributedString(NSAttributedString *string)
{
	CTTypesetter *typesetter;
	NSDictionary *attributes;

	if (string == nil || [string length] == 0) {
		return NULL;
	}
	/* **NO FONT, NO TYPESETTER.** The font comes out of the attributes, exactly as it does for a line, and a
	 * string that names none cannot be measured. */
	attributes = [string attributesAtIndex:0 effectiveRange:NULL];
	if ([attributes objectForKey:kCTFontAttributeName] == NULL) {
		return NULL;
	}
	typesetter = [[CTTypesetter alloc] init];
	typesetter->string = [string retain];
	return (CTTypesetterRef)typesetter;
}

CTLineRef CTTypesetterCreateLine(CTTypesetterRef typesetter, NSRange range)
{
	NSAttributedString *piece;

	if (typesetter == NULL || range.length == 0) {
		return NULL;
	}
	/* THE PIECE KEEPS ITS OWN ATTRIBUTES where the tree's Foundation can hand them over, so a range that
	 * spans an attribute change still produces runs — `CTLineCreateWithAttributedString` is what splits
	 * them. */
	piece = [[(NSAttributedString *)typesetter->string attributedSubstringFromRange:range] retain];
	{
		CTLineRef line = CTLineCreateWithAttributedString(piece);

		[piece release];
		return line;
	}
}

long CTTypesetterSuggestLineBreak(CTTypesetterRef typesetter, long start, double width)
{
	NSString *text;
	CTLineRef line;
	NSArray *runs;
	const char *utf8;
	size_t length;
	double base_x = -1.0;	/* the pen where the walk begins: the first glyph at or after `start` */
	long last_fit = -1;	/* the cluster of the last glyph that fits */
	long next_cluster = -1;	/* the cluster of the first glyph that does NOT: where the line could end */
	long candidate;
	long break_at;
	NSUInteger r;

	if (typesetter == NULL || start < 0) {
		return 0;
	}
	text = [(NSAttributedString *)typesetter->string string];
	utf8 = [text UTF8String];
	if (utf8 == NULL) {
		return start;
	}
	length = strlen(utf8);
	if ((size_t)start >= length) {
		return (long)length;
	}
	/* THE WHOLE STRING, SHAPED ONCE — see the file comment: this IS the instrument, and it is the same one
	 * the caller's own line will use. */
	line = CTLineCreateWithAttributedString((NSAttributedString *)typesetter->string);
	if (line == NULL) {
		return (long)length;
	}
	runs = CTLineGetGlyphRuns(line);
	for (r = 0; r < [runs count]; r++) {
		CTRunRef run = (CTRunRef)[runs objectAtIndex:r];
		const CGPoint *positions = CTRunGetPositionsPtr(run);
		const CGSize *advances = CTRunGetAdvancesPtr(run);
		const long *indices = CTRunGetStringIndicesPtr(run);
		long n = CTRunGetGlyphCount(run);
		long i;

		for (i = 0; i < n; i++) {
			if (indices[i] < start) {
				continue;	/* STEP 1: the walk begins at the first glyph that reached `start` */
			}
			if (base_x < 0.0) {
				base_x = positions[i].x;	/* and it is measured FROM there */
			}
			if (positions[i].x + advances[i].width - base_x <= width) {
				last_fit = indices[i];	/* STEP 2: the last glyph that FITS */
				next_cluster = -1;
			} else if (last_fit >= 0 && next_cluster < 0) {
				next_cluster = indices[i];	/* the first one that does not: the line's end byte */
			}
		}
	}
	candidate = (next_cluster >= 0) ? next_cluster
					: ((last_fit >= 0) ? (long)length : start);
	if (candidate <= start) {
		/* STEP 4: nothing fit (an over-long word, or a width smaller than one glyph), so the line takes ONE
		 * GLYPH and the caller's loop always advances. */
		candidate = start + 1;
	}
	/* **STEP 3, AND THIS IS THE WORD WRAP ITSELF — BUT ONLY WHEN THE LINE HAD TO STOP INSIDE THE STRING**
	 * (`next_cluster >= 0`: some glyph did not fit). When everything fits, the line IS the whole remaining
	 * text and there is nothing to wrap — backing up from the string's end would push its last words onto a
	 * next line that should not exist, which is a bug this probe caught. */
	break_at = candidate;
	if (next_cluster >= 0) {
		while (break_at > start + 1) {
			char before = utf8[break_at - 1];

			if (before == ' ' || before == '\t' || before == '\n' || before == '\r') {
				break;
			}
			break_at--;
		}
	}
	if ((size_t)break_at > length) {
		break_at = (long)length;
	}
	[runs release];
	CTLineRelease(line);
	return break_at;
}

CTTypesetterRef CTTypesetterRetain(CTTypesetterRef typesetter)
{
	return (CTTypesetterRef)[(id)(uintptr_t)typesetter retain];
}

void CTTypesetterRelease(CTTypesetterRef typesetter)
{
	[(id)(uintptr_t)typesetter release];
}

/* **M7d: THE TWO OPTION KEYS, AND THE DOOR THAT READS THEM.** The values are ours — Apple publishes the NAME
 * of each option and its value's TYPE, never the string — which is what makes them safe: an option key is
 * only ever compared with the constant declared beside it. */
NSString *const kCTTypesetterOptionDisableBidiProcessing = @"CTTypesetterDisableBidiProcessing";
NSString *const kCTTypesetterOptionForcedEmbeddingLevel = @"CTTypesetterForcedEmbeddingLevel";

/* **ONE OF THE TWO OPTIONS IS SATISFIED HERE AND THE OTHER IS REFUSED, AND THE DIFFERENCE IS THE POINT.**
 * This tier performs no Unicode Bidirectional Algorithm pass at all — the shaper takes HarfBuzz's own guess
 * for the direction and nothing reorders anything — so `DisableBidiProcessing` describes behaviour that is
 * ALREADY the behaviour: asking for it changes nothing, and the probe checks that the typesetter it answers is
 * the same one the plain door answers, break for break.
 *
 * `ForcedEmbeddingLevel` is a DIFFERENT KIND OF ASK: it names a level the layout must apply, and there is no
 * bidi pass to apply it to. So the door REFUSES BY NAME and answers NULL — Apple's own result for a creation
 * that fails — rather than handing back a typesetter whose text will be laid out in the wrong direction. That
 * is this tree's rule at the drawing layer (`CTLine.h`'s vertical refusal, `CTParagraphStyle.h`'s) applied to
 * a layout option: a wrong picture is worse than a missing one. */
CTTypesetterRef CTTypesetterCreateWithAttributedStringAndOptions(NSAttributedString *string,
								NSDictionary *options)
{
	if (options != nil && [options objectForKey:kCTTypesetterOptionForcedEmbeddingLevel] != nil) {
		return NULL;	/* refused by name: no bidi pass exists to carry an embedding level */
	}
	/* AND THE FLAG IS CONSULTED RATHER THAN BYPASSED, so its presence is a fact this door has looked at
	 * rather than one it never checks — the answer is the plain door's, for the reason above. */
	if (options != nil) {
		(void)[options objectForKey:kCTTypesetterOptionDisableBidiProcessing];
	}
	return CTTypesetterCreateWithAttributedString(string);
}
