/*
 * CTBidi.c — the two rules built so far: which class a code point is, and which way the paragraph runs.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE TABLE ANSWERS THE CLASS; P2 AND P3 ANSWER THE PARAGRAPH.** A binary search over `CTBidiData.c` is the whole
 * of the first, and the second is the fourteen lines below it: find the first strong character while stepping over
 * isolate runs. Everything else UAX #9 does — the X, W, N and I rules that give every OTHER character a level, and
 * the L1/L2 rules that put them in visual order — is named as the next slice in `CTBidi_internal.h` rather than
 * half-built here.
 */
#include <CoreText/CTBidi_internal.h>

/* **THE CLASS OF A CODE POINT.** The table holds only the ranges whose class is NOT the default, so a miss is L —
 * which is also the answer for the unassigned code points the table deliberately does not carry. */
CTBidiClass ct_bidi_class(uint32_t codePoint)
{
	size_t lo = 0;
	size_t hi = ct_bidi_range_count;

	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		const struct ct_bidi_range *range = &ct_bidi_ranges[mid];

		if (codePoint < range->first) {
			hi = mid;
		} else if (codePoint > range->last) {
			lo = mid + 1;
		} else {
			return range->bidi_class;
		}
	}
	return kCTBidiL;
}

/* **UAX #9 P2 AND P3, WHICH ARE THE WHOLE OF WHAT IS BUILT.** P2 takes the first STRONG character and P3 turns that
 * into a level: 0 (left to right) for L, 1 (right to left) for R or AL, and the caller's `fallbackLevel` when there
 * is no strong character at all. */
int ct_bidi_paragraph_level(const UniChar *text, CFIndex count, int fallbackLevel)
{
	CFIndex i;
	int isolateDepth = 0;

	if (text == NULL) {
		return fallbackLevel;
	}
	for (i = 0; i < count; i++) {
		CTBidiClass bidiClass = ct_bidi_class(text[i]);

		/* **P2 STEPS OVER AN ISOLATE RUN RATHER THAN READING IT.** An initiator opens the run, everything between it
		 * and its matching PDI is skipped, and the PDI closes it; a nested initiator deepens the run, and an
		 * UNMATCHED PDI (depth already zero) is skipped by the same line that closes a matched one. */
		if (bidiClass == kCTBidiLRI || bidiClass == kCTBidiRLI || bidiClass == kCTBidiFSI) {
			isolateDepth++;
			continue;
		}
		if (bidiClass == kCTBidiPDI) {
			if (isolateDepth > 0) {
				isolateDepth--;
			}
			continue;
		}
		if (isolateDepth > 0) {
			continue;
		}
		/* THE FIRST STRONG CHARACTER DECIDES. */
		if (bidiClass == kCTBidiL) {
			return 0;
		}
		if (bidiClass == kCTBidiR || bidiClass == kCTBidiAL) {
			return 1;
		}
	}
	/* P3: WITH NO STRONG CHARACTER THE PARAGRAPH IS LEFT TO RIGHT, unless the higher-level protocol said otherwise. */
	return fallbackLevel;
}

/* =================================================================================================
 * UAX #9 X1-X8: THE EXPLICIT LEVELS.
 * =============================================================================================== */

/* THE STANDARD'S OWN CEILING, in its own units: 125 embedding levels, with the two slots X1 keeps above the base. */
#define CT_BIDI_MAX_DEPTH 125

struct ct_bidi_status {
	int level;
	CTBidiClass override;	/* kCTBidiL, kCTBidiR, or kCTBidiON for "no override" */
	bool isolate;
};

/* THE LEAST ODD AND LEAST EVEN LEVELS GREATER THAN A GIVEN ONE — X2's and X3's arithmetic, named so the four
 * embedding rules do not each re-derive it. */
static int fn_least_greater_odd(int level)
{
	return (level + 1) | 1;
}

static int fn_least_greater_even(int level)
{
	return (level + 2) & ~1;
}

/* **X5c's QUESTION, ASKED OF AN FSI: WHICH WAY DOES THE TEXT IT ISOLATES RUN?** It is P2/P3 AGAIN, over the
 * characters from just after the FSI to the matching PDI, SKIPPING any nested isolate run — and answering the
 * paragraph level when there is no strong character inside. */
static int fn_isolate_direction(const UniChar *text, CFIndex count, CFIndex start, int fallback)
{
	CFIndex i;
	int depth = 0;

	for (i = start; i < count; i++) {
		CTBidiClass bidiClass = ct_bidi_class(text[i]);

		if (bidiClass == kCTBidiLRI || bidiClass == kCTBidiRLI || bidiClass == kCTBidiFSI) {
			depth++;
			continue;
		}
		if (bidiClass == kCTBidiPDI) {
			if (depth == 0) {
				break;		/* the matching PDI ends the isolated text */
			}
			depth--;
			continue;
		}
		if (depth > 0) {
			continue;
		}
		if (bidiClass == kCTBidiL) {
			return 0;
		}
		if (bidiClass == kCTBidiR || bidiClass == kCTBidiAL) {
			return 1;
		}
	}
	return fallback;
}

bool ct_bidi_explicit_levels(const UniChar *text, CFIndex count, int paragraphLevel,
			     uint8_t *levels, CTBidiClass *classes)
{
	struct ct_bidi_status stack[CT_BIDI_MAX_DEPTH + 2];
	int sp = 1;
	int overflowIsolate = 0;
	int overflowEmbedding = 0;
	int validIsolate = 0;
	CFIndex i;

	if (text == NULL || levels == NULL || classes == NULL || count < 0) {
		return false;
	}
	if (paragraphLevel != 0 && paragraphLevel != 1) {
		paragraphLevel = 0;
	}
	/* X1: the stack begins with the paragraph level, no override and not an isolate. */
	stack[0].level = paragraphLevel;
	stack[0].override = kCTBidiON;
	stack[0].isolate = false;

	for (i = 0; i < count; i++) {
		CTBidiClass bidiClass = ct_bidi_class(text[i]);
		int currentLevel = stack[sp - 1].level;
		int newLevel = currentLevel;

		levels[i] = (uint8_t)currentLevel;
		classes[i] = bidiClass;

		switch (bidiClass) {
		case kCTBidiRLE:
		case kCTBidiLRE:
		case kCTBidiRLO:
		case kCTBidiLRO: {
			bool rtl = (bidiClass == kCTBidiRLE || bidiClass == kCTBidiRLO);
			CTBidiClass override = (bidiClass == kCTBidiRLO) ? kCTBidiR
					     : (bidiClass == kCTBidiLRO) ? kCTBidiL : kCTBidiON;

			newLevel = rtl ? fn_least_greater_odd(currentLevel) : fn_least_greater_even(currentLevel);
			if (newLevel <= CT_BIDI_MAX_DEPTH && overflowIsolate == 0 && overflowEmbedding == 0) {
				stack[sp].level = newLevel;
				stack[sp].override = override;
				stack[sp].isolate = false;
				sp++;
			} else if (overflowIsolate == 0) {
				overflowEmbedding++;
			}
			/* X9: the initiator itself is REMOVED for the rules that follow. */
			classes[i] = kCTBidiBN;
			break;
		}
		case kCTBidiRLI:
		case kCTBidiLRI:
		case kCTBidiFSI: {
			bool rtl = (bidiClass == kCTBidiFSI)
				   ? (fn_isolate_direction(text, count, i + 1, paragraphLevel) == 1)
				   : (bidiClass == kCTBidiRLI);
			bool valid;

			newLevel = rtl ? fn_least_greater_odd(currentLevel) : fn_least_greater_even(currentLevel);
			valid = (newLevel <= CT_BIDI_MAX_DEPTH && overflowIsolate == 0 && overflowEmbedding == 0);
			if (valid) {
				stack[sp].level = newLevel;
				stack[sp].override = kCTBidiON;
				stack[sp].isolate = true;
				sp++;
				validIsolate++;
			} else {
				overflowIsolate++;
			}
			/* THE ISOLATE INITIATOR KEEPS ITS OWN LEVEL — the one OUTSIDE the isolate. X9 does not remove it. */
			break;
		}
		case kCTBidiPDI: {
			if (overflowIsolate > 0) {
				overflowIsolate--;
			} else if (validIsolate > 0) {
				overflowEmbedding = 0;
				while (sp > 1 && !stack[sp - 1].isolate) {
					sp--;
				}
				if (sp > 1) {
					sp--;
				}
				validIsolate--;
			}
			/* AND THE PDI CARRIES THE LEVEL IN EFFECT AFTER THE ISOLATE CLOSES. */
			levels[i] = (uint8_t)stack[sp - 1].level;
			break;
		}
		case kCTBidiPDF: {
			if (overflowIsolate > 0) {
				/* inside an overflowed isolate: nothing */
			} else if (overflowEmbedding > 0) {
				overflowEmbedding--;
			} else if (sp > 1 && !stack[sp - 1].isolate) {
				sp--;
			}
			classes[i] = kCTBidiBN;	/* X9 */
			break;
		}
		case kCTBidiB: {
			/* X8: a paragraph separator is set to the paragraph level, and a B in the MIDDLE of a line begins a new
			 * paragraph — so the stack is reset and the text after it is read from the paragraph level, which is the
			 * honest reading for a line this tier treats as one paragraph. */
			levels[i] = (uint8_t)paragraphLevel;
			sp = 1;
			overflowIsolate = 0;
			overflowEmbedding = 0;
			validIsolate = 0;
			break;
		}
		default: {
			/* X6: an ordinary character takes the current level, and an OVERRIDE replaces its class. */
			if (stack[sp - 1].override != kCTBidiON) {
				classes[i] = stack[sp - 1].override;
			}
			break;
		}
		}
	}
	return true;
}

/* =================================================================================================
 * UAX #9 W1-W7: THE WEAK TYPES.
 * =============================================================================================== */

/* **SOS AND EOS.** A level run's boundary type is R when the HIGHER of the run's level and its neighbour's is odd,
 * and L when it is even — the standard's rule, and the reason a run at level 1 inside a level-0 paragraph starts as
 * R. */
static CTBidiClass fn_sequence_boundary(int runLevel, int neighbourLevel)
{
	int higher = (runLevel > neighbourLevel) ? runLevel : neighbourLevel;

	return ((higher % 2) == 0) ? kCTBidiL : kCTBidiR;
}

/* THE LAST STRONG TYPE BEFORE AN INDEX, or `fallback` when the run has none before it. W2 wants R, L and AL; W7
 * wants L and R (AL is gone by then). ONE function, because the search differs only in which types stop it. */
static CTBidiClass fn_last_strong_before(const CTBidiClass *classes, CFIndex runStart, CFIndex index,
					 bool al_is_strong, CTBidiClass fallback)
{
	CFIndex j = index;

	while (j > runStart) {
		CTBidiClass previous = classes[j - 1];

		if (previous == kCTBidiL || previous == kCTBidiR || (al_is_strong && previous == kCTBidiAL)) {
			return previous;
		}
		j--;
	}
	return fallback;
}

void ct_bidi_resolve_weak(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel)
{
	CFIndex runStart = 0;

	if (classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	while (runStart < count) {
		int runLevel = levels[runStart];
		CFIndex runEnd = runStart + 1;
		int previousLevel = (runStart > 0) ? levels[runStart - 1] : paragraphLevel;
		int nextLevel;
		CTBidiClass sos;
		CFIndex i;

		while (runEnd < count && levels[runEnd] == runLevel) {
			runEnd++;
		}
		nextLevel = (runEnd < count) ? levels[runEnd] : paragraphLevel;
		sos = fn_sequence_boundary(runLevel, previousLevel);

		/* W1: a NON-SPACING MARK takes the type of what it follows, or sos at the run's start. */
		for (i = runStart; i < runEnd; i++) {
			if (classes[i] == kCTBidiNSM) {
				classes[i] = (i == runStart) ? sos : classes[i - 1];
			}
		}
		/* W2: a European number after an Arabic letter becomes an ARABIC number. */
		for (i = runStart; i < runEnd; i++) {
			if (classes[i] == kCTBidiEN
			    && fn_last_strong_before(classes, runStart, i, true, sos) == kCTBidiAL) {
				classes[i] = kCTBidiAN;
			}
		}
		/* W3: an Arabic letter becomes R. */
		for (i = runStart; i < runEnd; i++) {
			if (classes[i] == kCTBidiAL) {
				classes[i] = kCTBidiR;
			}
		}
		/* W4: a separator or a common separator BETWEEN two numbers of the same kind becomes that number. */
		for (i = runStart + 1; i + 1 < runEnd; i++) {
			if (classes[i] == kCTBidiES && classes[i - 1] == kCTBidiEN && classes[i + 1] == kCTBidiEN) {
				classes[i] = kCTBidiEN;
			} else if (classes[i] == kCTBidiCS) {
				if (classes[i - 1] == kCTBidiEN && classes[i + 1] == kCTBidiEN) {
					classes[i] = kCTBidiEN;
				} else if (classes[i - 1] == kCTBidiAN && classes[i + 1] == kCTBidiAN) {
					classes[i] = kCTBidiAN;
				}
			}
		}
		/* W5: a run of European terminators ADJACENT to a European number becomes that number. */
		for (i = runStart; i < runEnd; ) {
			if (classes[i] == kCTBidiET) {
				CFIndex j = i;
				bool adjacent = false;

				while (j < runEnd && classes[j] == kCTBidiET) {
					j++;
				}
				if (i > runStart && classes[i - 1] == kCTBidiEN) {
					adjacent = true;
				}
				if (j < runEnd && classes[j] == kCTBidiEN) {
					adjacent = true;
				}
				if (adjacent) {
					CFIndex k;

					for (k = i; k < j; k++) {
						classes[k] = kCTBidiEN;
					}
				}
				i = j;
			} else {
				i++;
			}
		}
		/* W6: what is left of the terminators and separators is OTHER NEUTRAL. */
		for (i = runStart; i < runEnd; i++) {
			if (classes[i] == kCTBidiET || classes[i] == kCTBidiES || classes[i] == kCTBidiCS) {
				classes[i] = kCTBidiON;
			}
		}
		/* W7: a European number after a Latin strong type becomes L. */
		for (i = runStart; i < runEnd; i++) {
			if (classes[i] == kCTBidiEN
			    && fn_last_strong_before(classes, runStart, i, false, sos) == kCTBidiL) {
				classes[i] = kCTBidiL;
			}
		}
		runStart = runEnd;
	}
}

/* =================================================================================================
 * UAX #9 N1 AND N2: THE NEUTRAL TYPES.
 * =============================================================================================== */

/* **NI: A NEUTRAL OR AN ISOLATE FORMATTING CHARACTER.** The standard's own name for the set the N rules resolve, and
 * the reason the isolate initiators and the PDI are here: they are NOT ink, and a neutral beside one must be read
 * against whatever is on the other side of it. */
static bool fn_is_ni(CTBidiClass bidiClass)
{
	return bidiClass == kCTBidiB || bidiClass == kCTBidiS || bidiClass == kCTBidiWS
	    || bidiClass == kCTBidiON || bidiClass == kCTBidiBN
	    || bidiClass == kCTBidiLRI || bidiClass == kCTBidiRLI || bidiClass == kCTBidiFSI || bidiClass == kCTBidiPDI;
}

/* **WHAT A NEUTRAL READS AS ITS NEIGHBOUR.** A European or Arabic number INFLUENCES a neutral exactly as R does —
 * Apple's rule and Unicode's — and anything that is not L or R is not strong at all, which the caller reads as "no
 * answer" and resolves with N2. */
static CTBidiClass fn_strong_for_neutral(CTBidiClass bidiClass)
{
	if (bidiClass == kCTBidiL) {
		return kCTBidiL;
	}
	if (bidiClass == kCTBidiR || bidiClass == kCTBidiEN || bidiClass == kCTBidiAN) {
		return kCTBidiR;
	}
	return kCTBidiON;
}

void ct_bidi_resolve_neutral(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel)
{
	CFIndex runStart = 0;

	if (classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	while (runStart < count) {
		int runLevel = levels[runStart];
		CFIndex runEnd = runStart + 1;
		int previousLevel = (runStart > 0) ? levels[runStart - 1] : paragraphLevel;
		int nextLevel;
		CTBidiClass sos;
		CTBidiClass eos;
		CTBidiClass embedding = ((runLevel % 2) != 0) ? kCTBidiR : kCTBidiL;
		CFIndex i;

		while (runEnd < count && levels[runEnd] == runLevel) {
			runEnd++;
		}
		nextLevel = (runEnd < count) ? levels[runEnd] : paragraphLevel;
		sos = fn_sequence_boundary(runLevel, previousLevel);
		eos = fn_sequence_boundary(runLevel, nextLevel);

		/* **N1 WOULD JOIN, N2 DECIDES.** Walk the run's NEUTRAL RUNS — the maximal stretches of NI — and for each read
		 * the strong type on either side (a number counting as R, and `sos`/`eos` at the ends). */
		i = runStart;
		while (i < runEnd) {
			if (fn_is_ni(classes[i])) {
				CFIndex j = i;
				CTBidiClass before;
				CTBidiClass after;
				CTBidiClass resolved;
				CFIndex k;

				while (j < runEnd && fn_is_ni(classes[j])) {
					j++;
				}
				before = (i > runStart) ? fn_strong_for_neutral(classes[i - 1]) : sos;
				after = (j < runEnd) ? fn_strong_for_neutral(classes[j]) : eos;
				/* N1: BOTH SIDES AGREE; N2: otherwise the EMBEDDING direction. */
				resolved = (before != kCTBidiON && before == after) ? before : embedding;
				for (k = i; k < j; k++) {
					classes[k] = resolved;
				}
				i = j;
			} else {
				i++;
			}
		}
		runStart = runEnd;
	}
}
