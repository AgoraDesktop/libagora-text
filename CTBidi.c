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

#include <stdlib.h>

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

/* **THE WEAK RULES OVER ONE SEQUENCE.** `sos` is the sequence's start-of-sequence type and the ARRAY IS the sequence
 * — inside an isolating run sequence there is no run boundary left to find, which is what X10's join buys. */
static void fn_apply_weak(CTBidiClass *classes, CFIndex count, CTBidiClass sos)
{
	const CFIndex runStart = 0;
	const CFIndex runEnd = count;
	CFIndex i;

	if (count <= 0) {
		return;
	}

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

/* **THE NEUTRAL RULES OVER ONE SEQUENCE**, with the sequence's `sos`, `eos` and EMBEDDING direction given. */
static void fn_apply_neutral(CTBidiClass *classes, CFIndex count, CTBidiClass sos, CTBidiClass eos,
			     CTBidiClass embedding)
{
	const CFIndex runStart = 0;
	const CFIndex runEnd = count;
	CFIndex i;

	if (count <= 0) {
		return;
	}

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
}

/* =================================================================================================
 * UAX #9 X10: THE ISOLATING RUN SEQUENCES.
 * =============================================================================================== */

/* THE THREE, AND THE THIRD IS THE ONE THAT JOINS: an isolate initiator's MATCHING PDI begins the next level run of
 * the SAME sequence, which is how the text on both sides of an isolate comes to see each other. */
static bool fn_is_isolate_initiator(CTBidiClass bidiClass)
{
	return bidiClass == kCTBidiLRI || bidiClass == kCTBidiRLI || bidiClass == kCTBidiFSI;
}

/* **THE SEQUENCES OF A PARAGRAPH.** Level runs first, then each run whose LAST character is an isolate initiator is
 * chained to the run its matching PDI STARTS; a chain is one isolating run sequence. What comes back is an ARRAY OF
 * SEQUENCE IDS per character (`seqId`), which is all the drivers need — the characters of a sequence appear in
 * INCREASING text order, so a walk over the text filtered by the id IS the sequence. Also filled: each sequence's
 * `sos`, `eos` (from the higher of its level and the level just outside the sequence) and its level. The caller frees
 * all four arrays. */
static CFIndex fn_sequences(const CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel,
			    int **seqIdOut, CTBidiClass **sosOut, CTBidiClass **eosOut, int **levelOut)
{
	CFIndex *match;
	CFIndex *stack;
	CFIndex *runStart;
	CFIndex *runEnd;
	CFIndex *runNext;
	CFIndex *runPrev;
	CFIndex *seqOfRun;
	CFIndex *firstRun;
	CFIndex *lastRun;
	int *seqId;
	CTBidiClass *sos;
	CTBidiClass *eos;
	int *seqLevel;
	CFIndex runCount = 0;
	CFIndex seqCount = 0;
	CFIndex sp = 0;
	CFIndex i, r, s;

	*seqIdOut = NULL;
	*sosOut = NULL;
	*eosOut = NULL;
	*levelOut = NULL;
	if (count <= 0) {
		return 0;
	}
	match = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	stack = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	runStart = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	runEnd = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	runNext = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	runPrev = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	seqOfRun = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	firstRun = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	lastRun = (CFIndex *)malloc((size_t)count * sizeof(CFIndex));
	seqId = (int *)malloc((size_t)count * sizeof(int));
	if (match == NULL || stack == NULL || runStart == NULL || runEnd == NULL || runNext == NULL
	    || runPrev == NULL || seqOfRun == NULL || firstRun == NULL || lastRun == NULL || seqId == NULL) {
		goto done;
	}
	/* THE MATCHING PDIs: a stack of initiators, and a PDI closes the innermost one. */
	for (i = 0; i < count; i++) {
		match[i] = -1;
	}
	for (i = 0; i < count; i++) {
		if (fn_is_isolate_initiator(classes[i])) {
			stack[sp++] = i;
		} else if (classes[i] == kCTBidiPDI && sp > 0) {
			CFIndex open = stack[--sp];

			match[open] = i;
			match[i] = open;
		}
	}
	/* THE LEVEL RUNS. */
	for (i = 0; i < count; ) {
		CFIndex j = i + 1;

		while (j < count && levels[j] == levels[i]) {
			j++;
		}
		runStart[runCount] = i;
		runEnd[runCount] = j;
		runCount++;
		i = j;
	}
	/* THE CHAINS: a run whose last character is an isolate initiator whose matching PDI STARTS a LATER run. */
	for (r = 0; r < runCount; r++) {
		runNext[r] = -1;
	}
	for (r = 0; r < runCount; r++) {
		CFIndex last = runEnd[r] - 1;

		if (match[last] >= 0) {
			for (s = r + 1; s < runCount; s++) {
				if (runStart[s] == match[last]) {
					runNext[r] = s;
					break;
				}
			}
		}
	}
	for (r = 0; r < runCount; r++) {
		runPrev[r] = -1;
	}
	for (r = 0; r < runCount; r++) {
		if (runNext[r] >= 0) {
			runPrev[runNext[r]] = r;
		}
	}
	/* THE SEQUENCES: every chain start (a run with no predecessor) walks its chain. */
	for (r = 0; r < runCount; r++) {
		seqOfRun[r] = -1;
	}
	for (r = 0; r < runCount; r++) {
		CFIndex q;

		if (runPrev[r] >= 0) {
			continue;
		}
		q = r;
		while (q >= 0 && seqOfRun[q] < 0) {
			seqOfRun[q] = seqCount;
			q = runNext[q];
		}
		seqCount++;
	}
	sos = (CTBidiClass *)malloc((size_t)((seqCount > 0) ? seqCount : 1) * sizeof(CTBidiClass));
	eos = (CTBidiClass *)malloc((size_t)((seqCount > 0) ? seqCount : 1) * sizeof(CTBidiClass));
	seqLevel = (int *)malloc((size_t)((seqCount > 0) ? seqCount : 1) * sizeof(int));
	if (sos == NULL || eos == NULL || seqLevel == NULL) {
		goto done;	/* the cleanup below frees whichever of the three arrived */
	}
	for (s = 0; s < seqCount; s++) {
		firstRun[s] = -1;
		lastRun[s] = -1;
	}
	for (r = 0; r < runCount; r++) {
		if (firstRun[seqOfRun[r]] < 0) {
			firstRun[seqOfRun[r]] = r;
		}
		lastRun[seqOfRun[r]] = r;
	}
	for (s = 0; s < seqCount; s++) {
		CFIndex fr = firstRun[s];
		CFIndex lr = lastRun[s];
		int level = levels[runStart[fr]];
		int before = (runStart[fr] > 0) ? levels[runStart[fr] - 1] : paragraphLevel;
		int after = (runEnd[lr] < count) ? levels[runEnd[lr]] : paragraphLevel;

		seqLevel[s] = level;
		sos[s] = fn_sequence_boundary(level, before);
		eos[s] = fn_sequence_boundary(level, after);
	}
	for (r = 0; r < runCount; r++) {
		for (i = runStart[r]; i < runEnd[r]; i++) {
			seqId[i] = (int)seqOfRun[r];
		}
	}
	*seqIdOut = seqId;
	*sosOut = sos;
	*eosOut = eos;
	*levelOut = seqLevel;
	seqId = NULL;
	sos = NULL;
	eos = NULL;
	seqLevel = NULL;
done:
	free(match);
	free(stack);
	free(runStart);
	free(runEnd);
	free(runNext);
	free(runPrev);
	free(seqOfRun);
	free(firstRun);
	free(lastRun);
	free(seqId);	/* NULL once they have been handed over, which is what makes this one cleanup */
	free(sos);
	free(eos);
	free(seqLevel);
	return (*seqIdOut != NULL) ? seqCount : 0;
}

/* **AND THE TWO DRIVERS** — written out rather than through a callback, because a callback per sequence would hide
 * the one thing worth reading: which boundaries go with which array. Each builds the sequences, gathers ONE
 * sequence's classes into a contiguous array (the characters are in increasing text order, so a walk filtered by the
 * id IS the sequence), applies the rules with the sequence's own boundaries, and writes the answer back. */

void ct_bidi_resolve_weak(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel)
{
	int *seqId = NULL;
	CTBidiClass *sos = NULL;
	CTBidiClass *eos = NULL;
	int *seqLevel = NULL;
	CFIndex seqCount;
	CFIndex s, k;

	if (classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	seqCount = fn_sequences(classes, levels, count, paragraphLevel, &seqId, &sos, &eos, &seqLevel);
	for (s = 0; s < seqCount; s++) {
		CTBidiClass *sequence = NULL;
		CFIndex n = 0;

		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				n++;
			}
		}
		if (n == 0) {
			continue;
		}
		sequence = (CTBidiClass *)malloc((size_t)n * sizeof(CTBidiClass));
		if (sequence == NULL) {
			break;
		}
		n = 0;
		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				sequence[n++] = classes[k];
			}
		}
		fn_apply_weak(sequence, n, sos[s]);
		n = 0;
		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				classes[k] = sequence[n++];
			}
		}
		free(sequence);
	}
	free(seqId);
	free(sos);
	free(eos);
	free(seqLevel);
}

void ct_bidi_resolve_neutral(CTBidiClass *classes, const uint8_t *levels, CFIndex count, int paragraphLevel)
{
	int *seqId = NULL;
	CTBidiClass *sos = NULL;
	CTBidiClass *eos = NULL;
	int *seqLevel = NULL;
	CFIndex seqCount;
	CFIndex s, k;

	if (classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	seqCount = fn_sequences(classes, levels, count, paragraphLevel, &seqId, &sos, &eos, &seqLevel);
	for (s = 0; s < seqCount; s++) {
		CTBidiClass *sequence = NULL;
		CFIndex n = 0;

		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				n++;
			}
		}
		if (n == 0) {
			continue;
		}
		sequence = (CTBidiClass *)malloc((size_t)n * sizeof(CTBidiClass));
		if (sequence == NULL) {
			break;
		}
		n = 0;
		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				sequence[n++] = classes[k];
			}
		}
		fn_apply_neutral(sequence, n, sos[s], eos[s], ((seqLevel[s] % 2) != 0) ? kCTBidiR : kCTBidiL);
		n = 0;
		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				classes[k] = sequence[n++];
			}
		}
		free(sequence);
	}
	free(seqId);
	free(sos);
	free(eos);
	free(seqLevel);
}

/* =================================================================================================
 * UAX #9 I1 AND I2: THE IMPLICIT LEVELS.
 * =============================================================================================== */

/* **THE LEVEL A CHARACTER ACTUALLY SITS AT.** The embedding level is what the EXPLICIT phase set; the implicit rules
 * raise it for a character whose direction DISAGREES with its level, and the amounts are the standard's: a strong R
 * in an even (left-to-right) level goes up ONE, a number goes up TWO, and in an odd (right-to-left) level L and the
 * numbers go up one while R stays. That is why a Hebrew letter inside a Latin paragraph ends at level 1 — an ODD
 * level — which is exactly the fact L2 reorders on. */
void ct_bidi_resolve_implicit(const CTBidiClass *classes, uint8_t *levels, CFIndex count)
{
	CFIndex i;

	if (classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	for (i = 0; i < count; i++) {
		CTBidiClass bidiClass = classes[i];

		if ((levels[i] % 2) == 0) {
			/* I1: an even level. */
			if (bidiClass == kCTBidiR) {
				levels[i] = (uint8_t)(levels[i] + 1);
			} else if (bidiClass == kCTBidiEN || bidiClass == kCTBidiAN) {
				levels[i] = (uint8_t)(levels[i] + 2);
			}
		} else {
			/* I2: an odd level. */
			if (bidiClass == kCTBidiL || bidiClass == kCTBidiEN || bidiClass == kCTBidiAN) {
				levels[i] = (uint8_t)(levels[i] + 1);
			}
		}
	}
}

/* =================================================================================================
 * UAX #9 N0: THE BRACKET PAIRS.
 * =============================================================================================== */

/* THE TABLE'S ROW FOR A CODE POINT, or NULL when it is not a bracket at all. */
static const struct ct_bidi_bracket *fn_bracket(uint32_t codePoint)
{
	size_t lo = 0;
	size_t hi = ct_bidi_bracket_count;

	while (lo < hi) {
		size_t mid = lo + (hi - lo) / 2;
		const struct ct_bidi_bracket *row = &ct_bidi_brackets[mid];

		if (codePoint < row->bracket) {
			hi = mid;
		} else if (codePoint > row->bracket) {
			lo = mid + 1;
		} else {
			return row;
		}
	}
	return NULL;
}

/* ONE MATCHED PAIR, as the two positions in the SEQUENCE's own order. */
struct ct_bidi_pair {
	CFIndex open;
	CFIndex close;
};

void ct_bidi_resolve_brackets(const UniChar *text, CTBidiClass *classes, const uint8_t *levels, CFIndex count,
			      int paragraphLevel)
{
	int *seqId = NULL;
	CTBidiClass *sos = NULL;
	CTBidiClass *eos = NULL;
	int *seqLevel = NULL;
	CFIndex seqCount;
	CFIndex s;

	if (text == NULL || classes == NULL || levels == NULL || count <= 0) {
		return;
	}
	seqCount = fn_sequences(classes, levels, count, paragraphLevel, &seqId, &sos, &eos, &seqLevel);
	for (s = 0; s < seqCount; s++) {
		CFIndex *idx;
		CFIndex *openAt;
		uint32_t *openCp;
		struct ct_bidi_pair *pairs;
		CFIndex n = 0;
		CFIndex sp = 0;
		CFIndex pairCount = 0;
		CFIndex k, m;
		CTBidiClass embedding = ((seqLevel[s] % 2) != 0) ? kCTBidiR : kCTBidiL;

		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				n++;
			}
		}
		if (n == 0) {
			continue;
		}
		idx = (CFIndex *)malloc((size_t)n * sizeof(CFIndex));
		openAt = (CFIndex *)malloc((size_t)n * sizeof(CFIndex));
		openCp = (uint32_t *)malloc((size_t)n * sizeof(uint32_t));
		pairs = (struct ct_bidi_pair *)malloc((size_t)n * sizeof(struct ct_bidi_pair));
		if (idx == NULL || openAt == NULL || openCp == NULL || pairs == NULL) {
			free(idx);
			free(openAt);
			free(openCp);
			free(pairs);
			break;
		}
		n = 0;
		for (k = 0; k < count; k++) {
			if (seqId[k] == (int)s) {
				idx[n++] = k;
			}
		}
		/* **BD16: AN OPENER PUSHES, A CLOSER MATCHES THE NEAREST OPENER IT PAIRS WITH.** The stack holds the
		 * opener's POSITION IN THE SEQUENCE and its own code point, so the pair is two sequence positions. A closer
		 * that matches nothing pops nothing; when it matches, the opener AND everything above it go — which is what
		 * makes an unpaired opener inside a matched pair fall out. */
		for (m = 0; m < n; m++) {
			const struct ct_bidi_bracket *row = fn_bracket(text[idx[m]]);

			if (row == NULL) {
				continue;
			}
			if (row->isOpen) {
				if (sp >= 63) {
					break;	/* the standard's stack is 63 deep: stop looking */
				}
				openAt[sp] = m;
				openCp[sp] = row->bracket;
				sp++;
			} else {
				CFIndex j;

				for (j = sp - 1; j >= 0; j--) {
					const struct ct_bidi_bracket *opener = fn_bracket(openCp[j]);

					if (opener != NULL && opener->paired == row->bracket) {
						break;
					}
				}
				if (j >= 0) {
					pairs[pairCount].open = openAt[j];
					pairs[pairCount].close = m;
					pairCount++;
					sp = j;	/* the opener and everything above it are gone */
				}
			}
		}
		/* **AND THE PAIRS ARE RESOLVED IN THE LOGICAL ORDER OF THEIR OPENING BRACKETS**, which is the standard's
		 * own order and the reason this is inserted-sorted rather than kept as the closers were found. */
		for (k = 1; k < pairCount; k++) {
			struct ct_bidi_pair key = pairs[k];
			CFIndex b;

			for (b = k - 1; b >= 0 && pairs[b].open > key.open; b--) {
				pairs[b + 1] = pairs[b];
			}
			pairs[b + 1] = key;
		}
		for (k = 0; k < pairCount; k++) {
			CFIndex open = pairs[k].open;
			CFIndex close = pairs[k].close;
			CTBidiClass insideEmbedding = kCTBidiON;
			CTBidiClass insideOpposite = kCTBidiON;
			CTBidiClass resolved = kCTBidiON;
			CFIndex q;

			/* N0.a: THE ENCLOSED STRONG TYPES, a number counting as R. */
			for (q = open + 1; q < close; q++) {
				CTBidiClass strong = fn_strong_for_neutral(classes[idx[q]]);

				if (strong == embedding) {
					insideEmbedding = strong;
				} else if (strong != kCTBidiON) {
					insideOpposite = strong;
				}
			}
			if (insideEmbedding != kCTBidiON) {
				/* N0.b: SOMETHING INSIDE AGREES with the embedding direction. */
				resolved = embedding;
			} else if (insideOpposite != kCTBidiON) {
				/* N0.c: THE INSIDE DISAGREES, so the CONTEXT BEFORE decides — and only an established one wins. */
				CTBidiClass before = sos[s];

				for (q = open - 1; q >= 0; q--) {
					CTBidiClass strong = fn_strong_for_neutral(classes[idx[q]]);

					if (strong != kCTBidiON) {
						before = strong;
						break;
					}
				}
				resolved = (before == insideOpposite) ? before : embedding;
			}
			/* N0.d: NOTHING INSIDE IS STRONG, so the brackets stay neutral and N1/N2 settle them. */
			if (resolved != kCTBidiON) {
				classes[idx[open]] = resolved;
				classes[idx[close]] = resolved;
			}
		}
		free(idx);
		free(openAt);
		free(openCp);
		free(pairs);
	}
	free(seqId);
	free(sos);
	free(eos);
	free(seqLevel);
}
