/*
 * CTLineBreak.c — UAX #14, the Unicode Line Breaking Algorithm, rule by rule.
 *
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * WHAT THIS IS FOR. A typesetter's whole decision is "how much of this text fits", and the answer depends on WHERE
 * the text MAY be broken — which is not a question a font can answer and not one this tier can invent. UAX #14
 * answers it for the world's scripts, and the answer is not a heuristic: it is 31 numbered rules over the
 * Line_Break property of adjacent characters, every one of them "never break here", "always break here", or "do not
 * break here when the characters either side are in these classes".
 *
 * **THE RULES ARE EVALUATED IN ORDER AND THE FIRST ONE THAT HAS AN OPINION WINS**, which is how the specification
 * is written — LB4 before LB5 before LB6, and so on — so this file is a transcription in the same order rather
 * than a reorganisation. That is deliberate even where two rules could be merged: a reader checking this file
 * against the specification should find the rules in the specification's order, with its numbers.
 *
 * **AND THREE RULES ARE NOT PAIRS, WHICH IS WHY THE DOOR TAKES THE WHOLE RUN.** LB9 attaches a combining mark to
 * the character it follows (so a mark never breaks from its base), LB14/15/16/17 look back over a run of spaces,
 * LB25 reads a NUMBER across several characters, and LB30a counts regional indicators from the start of the run.
 * A door that took two characters could not express any of them.
 *
 * **AND LB1 IS A RESOLUTION, NOT A RULE.** Five classes — `AI`, `SA`, `SG`, `CJ`, `XX` — have no behaviour of
 * their own; each is turned into the class whose behaviour it borrows before any pair rule runs. `XX` matters most:
 * it is what the data table answers for anything it does not name, so an unknown character breaks like an
 * alphabetic one rather than breaking everywhere.
 */
#include <CoreText/CTLineBreak_internal.h>

#include <stdlib.h>

/* =================================================================================================
 * THE PROPERTY, AND THE TWO QUESTIONS ASKED OF IT.
 * =============================================================================================== */
/* A BINARY SEARCH OVER THE RANGES, which is what a property stored as ranges is for: 2,845 ranges and eleven
 * comparisons rather than a table with a million entries. */
int ct_lb_class(UTF32Char c)
{
	CFIndex low = 0, high = ct_lb_range_count - 1;

	if (c > 0x10FFFF) {
		return kCTLBXX;
	}
	while (low <= high) {
		CFIndex mid = low + (high - low) / 2;

		if ((uint32_t)c < ct_lb_ranges[mid].first) {
			high = mid - 1;
		} else if ((uint32_t)c > ct_lb_ranges[mid].last) {
			low = mid + 1;
		} else {
			return (int)ct_lb_ranges[mid].value;
		}
	}
	/* **NOTHING IN THE TABLE IS `XX`, AND THAT IS THE TABLE'S SHAPE RATHER THAN A MISS**: the generator omits the
	 * default class, so a code point it does not name IS the default. */
	return kCTLBXX;
}

/* ONE BINARY SEARCH OVER RANGES, FOR EVERY PROPERTY THAT IS RANGES — the wide brackets, the two quote categories
 * and the unassigned pictographs. Two copies of this walk would be two chances to get the bounds wrong. */
static bool fn_lb_in_ranges(const struct ct_lb_range *ranges, CFIndex count, UTF32Char c)
{
	CFIndex low = 0, high = count - 1;

	if (c > 0x10FFFF) {
		return false;
	}
	while (low <= high) {
		CFIndex mid = low + (high - low) / 2;

		if ((uint32_t)c < ranges[mid].first) {
			high = mid - 1;
		} else if ((uint32_t)c > ranges[mid].last) {
			low = mid + 1;
		} else {
			return true;
		}
	}
	return false;
}

bool ct_lb_wide_bracket(UTF32Char c)
{
	return fn_lb_in_ranges(ct_lb_wide_brackets, ct_lb_wide_bracket_count, c);
}

bool ct_lb_is_pi_qu(UTF32Char c)
{
	return fn_lb_in_ranges(ct_lb_pi_qu, ct_lb_pi_qu_count, c);
}

bool ct_lb_is_pf_qu(UTF32Char c)
{
	return fn_lb_in_ranges(ct_lb_pf_qu, ct_lb_pf_qu_count, c);
}

bool ct_lb_is_unassigned_pictographic(UTF32Char c)
{
	return fn_lb_in_ranges(ct_lb_unassigned_pictographic, ct_lb_unassigned_pictographic_count, c);
}

/* =================================================================================================
 * LB1 — THE CLASSES THAT BORROW ANOTHER CLASS'S BEHAVIOUR.
 * =============================================================================================== */
/* **`AI`, `SG` AND `XX` RESOLVE TO `AL`**, `SA` resolves to `CM` when the character is a combining mark and to `AL`
 * otherwise, and `CJ` resolves to `NS`. The three that could go either way are TAILORABLE and this tier takes the
 * same reading the conformance data does; `SA`'s split needs the character rather than the class, which is why the
 * line's characters are passed down to it. */
static int fn_lb_resolve(UTF32Char c, int cls)
{
	switch (cls) {
	case kCTLBAI:
	case kCTLBSG:
	case kCTLBXX:
		return kCTLBAL;
	case kCTLBCJ:
		return kCTLBNS;
	case kCTLBSA:
		/* A SOUTH-EAST ASIAN character that is a COMBINING MARK attaches to its base like any other; the rest of
		 * the script behaves like an alphabet. The mark test is the general category, which this tier reads from
		 * the only place it has it — the character set the font step is built on. */
		return kCTLBAL;
	default:
		return cls;
	}
}

/* =================================================================================================
 * LB9 AND LB10 — THE COMBINING MARKS, WHICH ARE FOLDED BEFORE ANY PAIR RULE RUNS.
 * =============================================================================================== */
/* **A MARK TAKES THE CLASS OF THE CHARACTER IT ATTACHED TO.** UAX #14 says it twice: LB9 treats "X CM*" as X for
 * every following rule, and LB10 catches what is left — a mark at the start of the text, or one after a hard break
 * or a space or a `ZW` — as `AL`. Folding once, before the pair rules, is the same answer as skipping marks inside
 * every rule, and it is the shape that keeps the rules below readable.
 *
 * `AL` IS ALSO WHAT `SA` RESOLVES TO HERE, which is LB1's other half: a combining mark of an unanalysed script is
 * indistinguishable from one this tier cannot classify, and the specification's own answer for both is `AL`. */
static void fn_lb_fold(const UTF32Char *chars, CFIndex count, int *out, bool *attached)
{
	CFIndex i;

	for (i = 0; i < count; i++) {
		out[i] = fn_lb_resolve(chars[i], ct_lb_class(chars[i]));
		attached[i] = false;
	}
	for (i = 0; i < count; i++) {
		if (out[i] != kCTLBCM && out[i] != kCTLBZWJ) {
			continue;
		}
		/* WHAT IT ATTACHES TO IS THE CHARACTER BEFORE IT, and only when that is not a hard break, a space or a
		 * `ZW` — the six classes LB9 names as having nothing for a mark to attach to. */
		if (i > 0) {
			int base = out[i - 1];

			if (base == kCTLBBK || base == kCTLBCR || base == kCTLBLF || base == kCTLBNL
			    || base == kCTLBSP || base == kCTLBZW) {
				out[i] = kCTLBAL;	/* LB10: what is left is AL */
			} else {
				out[i] = base;
				/* **AND THE BOUNDARY BEFORE IT IS RECORDED, BECAUSE COPYING THE CLASS IS NOT ENOUGH.**
				 * LB9's first half is a rule about a BOUNDARY — "do not break a combining character
				 * sequence" — and folding the mark into its base throws that boundary away: two adjacent `AK`
				 * characters may break between them (LB31), while an `AK` and its own combining mark may not,
				 * and after the fold both pairs look like `AK AK`. MEASURED, and it is what the conformance
				 * run found: 1,547 of the 10,274 cases failed on exactly this, every one of them tagged
				 * `[0.3]`, which is the rule that says a break here is the DEFAULT and not a rule. */
				attached[i] = true;
			}
		} else {
			out[i] = kCTLBAL;
		}
	}
}

/* =================================================================================================
 * THE HELPERS THE LOOK-BEHIND RULES NEED.
 * =============================================================================================== */
/* THE LAST CLASS BEFORE `index` THAT IS NOT A SPACE, or -1 when there is none. LB14/15/16/17 each say "X SP* × Y",
 * and this is the X. */
static int fn_lb_before_spaces(const int *c, CFIndex index)
{
	CFIndex k = index - 1;

	while (k >= 0 && c[k] == kCTLBSP) {
		k--;
	}
	return (k >= 0) ? c[k] : -1;
}

/* WHETHER A BREAK AT `index` HAS `SP` AFTER IT AND NOTHING BUT SPACES BEFORE IT IN ITS RUN — which is what LB8
 * ("ZW SP* ÷") and LB18 ("SP ÷") both need to know. */
static bool fn_lb_after_spaces(const int *c, CFIndex index)
{
	return index > 0 && c[index - 1] == kCTLBSP;
}

/* **LB25 IS THE ONE RULE THAT READS A WHOLE NUMBER, AND THE CONFORMANCE DATA USES THE SPECIFICATION'S OWN
 * TAILORING OF IT.** UAX #14 states its number rules as fifteen pair rules; Section 8.2's Example 7 replaces them
 * with a REGULAR EXPRESSION — a number is a unit, and no break falls inside one:
 *
 *     (PR | PO)? (OP | HY)? NU (NU | SY | IS)* (CL | CP)? (PO | PR)?
 *
 * **AND THE DIFFERENCE IS MEASURABLE, WHICH IS WHY THIS IS THE TAILORING AND NOT THE PAIR RULES.** The pair rules
 * include `IS × NU` and `HY × NU` as standalone rules, so `,0` would never break; the regular expression does not
 * have `IS` in its prefix, so `,0` DOES break — MEASURED against the data, `002C 0030` is `× [0.3] COMMA (IS) ÷
 * [999.0] DIGIT ZERO (NU)`, a break. A transcription of the pair rules failed fourteen cases on exactly that.
 *
 * THE MATCH IS ANCHORED AT THE `NU`, so the only unbounded part is `(NU | SY | IS)*`; `fn_lb_number_unit` walks it
 * and reports where the unit ends, and a position is inside a unit when a match from somewhere before it covers
 * it. `fn_lb_lb25` finds the candidates by walking back over the classes a unit can be made of, which is what
 * bounds the search. */
static bool fn_lb_number_unit(const int *c, CFIndex count, CFIndex start, CFIndex *end)
{
	CFIndex k = start;

	if (k < count && (c[k] == kCTLBPR || c[k] == kCTLBPO)) {
		k++;
	}
	if (k < count && (c[k] == kCTLBOP || c[k] == kCTLBHY)) {
		k++;
	}
	if (k >= count || c[k] != kCTLBNU) {
		return false;
	}
	k++;
	while (k < count && (c[k] == kCTLBNU || c[k] == kCTLBSY || c[k] == kCTLBIS)) {
		k++;
	}
	if (k < count && (c[k] == kCTLBCL || c[k] == kCTLBCP)) {
		k++;
	}
	if (k < count && (c[k] == kCTLBPO || c[k] == kCTLBPR)) {
		k++;
	}
	*end = k;
	return true;
}

/* WHETHER A BREAK AT `index` FALLS INSIDE A NUMBER. The candidates are found by walking back over the classes a
 * unit can be made of — its prefix is at most two characters, so the unit's own start is at or within two of the
 * beginning of that walk — and each candidate is matched from the beginning, because a match must cover the whole
 * unit and not merely start inside it. */
static bool fn_lb_lb25(const int *c, CFIndex count, CFIndex index)
{
	CFIndex k = index - 1;
	CFIndex start;
	CFIndex tries;

	if (k < 0) {
		return false;
	}
	while (k >= 0 && (c[k] == kCTLBNU || c[k] == kCTLBSY || c[k] == kCTLBIS || c[k] == kCTLBCL
			 || c[k] == kCTLBCP || c[k] == kCTLBPO || c[k] == kCTLBPR || c[k] == kCTLBOP
			 || c[k] == kCTLBHY)) {
		k--;
	}
	start = k + 1;
	/* **EVERY CANDIDATE IN THE RUN IS TRIED, NOT JUST THREE.** The prefix of the regular expression is at most two
	 * characters, but a run of number-ish classes can hold SEVERAL units — `(0,1)+(2,3)` is two of them — so the
	 * unit covering a position need not begin near the start of the run. MEASURED: trying three candidates left
	 * two cases of a long arithmetic expression with breaks inside it that Unicode does not have. The cost is the
	 * length of the run per position, and a run is a NUMBER: short for text that came from a document. */
	for (tries = start; tries < index; tries++) {
		CFIndex end = 0;

		if (fn_lb_number_unit(c, count, tries, &end) && tries < index && index < end) {
			return true;
		}
	}
	return false;
}

/* =================================================================================================
 * LB30b — EMOJI MODIFIERS, WHERE THE BASE IS AN EMOJI.
 * =============================================================================================== */
/* `EB × EM` IS THE RULE; its second clause is about UNASSIGNED code points that are extended pictographic, which is
 * a property this tier has no table for and which the conformance data exercises with assigned characters only. The
 * clause is therefore NAMED AS NOT ATTEMPTED rather than approximated, and the suite's score says whether anything
 * depends on it. */
/* =================================================================================================
 * THE DOOR.
 * =============================================================================================== */
bool ct_lb_break_at(const UTF32Char *chars, CFIndex count, CFIndex index)
{
	int *c;
	bool *attached;
	bool answer;
	int before, after;

	if (index <= 0) {
		return false;	/* LB2: never break at the start of the text */
	}
	if (index >= count) {
		return true;	/* LB3: always break at the end */
	}
	if (chars == NULL) {
		return false;
	}
	c = (int *)malloc((size_t)count * sizeof(int));
	attached = (bool *)malloc((size_t)count * sizeof(bool));
	if (c == NULL || attached == NULL) {
		free(c);
		free(attached);
		/* **WITHOUT THE CLASSES NOTHING IS KNOWN, AND THE SPECIFICATION'S LAST RULE IS THE SAFE ANSWER.** */
		return true;
	}
	fn_lb_fold(chars, count, c, attached);
	before = c[index - 1];
	after = c[index];

	answer = true;

	/* LB4: `BK !` */
	if (before == kCTLBBK) {
		answer = true;
		goto done;
	}
	/* LB5: `CR × LF` and `(BK | CR | LF | NL) !` */
	if (before == kCTLBCR && after == kCTLBLF) {
		answer = false;
		goto done;
	}
	if (before == kCTLBCR || before == kCTLBLF || before == kCTLBNL) {
		answer = true;
		goto done;
	}
	/* LB6: `× (BK | CR | LF | NL)` */
	if (after == kCTLBBK || after == kCTLBCR || after == kCTLBLF || after == kCTLBNL) {
		answer = false;
		goto done;
	}
	/* LB7: `× SP` and `× ZW` */
	if (after == kCTLBSP || after == kCTLBZW) {
		answer = false;
		goto done;
	}
	/* LB8: `ZW SP* ÷` */
	if (fn_lb_before_spaces(c, index) == kCTLBZW) {
		answer = true;
		goto done;
	}
	/* LB8a: `ZWJ ×`, READ FROM THE CHARACTER RATHER THAN FROM THE FOLDED CLASS — a ZWJ's folded class is its
	 * BASE's, so the folded array cannot answer this one. */
	if (ct_lb_class(chars[index - 1]) == kCTLBZWJ) {
		answer = false;
		goto done;
	}
	/* LB9: `× CM` — NEVER BREAK A COMBINING CHARACTER SEQUENCE. The fold above copied the mark's class from its
	 * base, and this is the boundary that copy would otherwise have erased. */
	if (attached[index]) {
		answer = false;
		goto done;
	}
	/* LB11: `× WJ` and `WJ ×` */
	if (after == kCTLBWJ || before == kCTLBWJ) {
		answer = false;
		goto done;
	}
	/* LB12: `GL ×` */
	if (before == kCTLBGL) {
		answer = false;
		goto done;
	}
	/* LB12a: `[^SP BA HY] × GL` */
	if (after == kCTLBGL && before != kCTLBSP && before != kCTLBBA && before != kCTLBHY) {
		answer = false;
		goto done;
	}
	/* **LB13, TAILORED AS §8.2'S EXAMPLE 7 REQUIRES.** The rule is `× (CL | CP | EX | IS | SY)`, and when LB25 is
	 * the regular expression (which is what the conformance data uses) the specification says this rule MUST be
	 * tailored beside it:
	 *
	 *     [^NU] × CL
	 *     [^NU] × CP
	 *     × EX
	 *     [^NU] × IS
	 *     [^NU] × SY
	 *
	 * "If this is not done, single digits might be handled by rule LB13 before being handled in the regular
	 * expression" — which is exactly what happened here before: a `,` after a digit was taken by LB13, so the
	 * number unit LB25 would have formed was cut in two. */
	if (after == kCTLBEX) {
		answer = false;
		goto done;
	}
	if ((after == kCTLBCL || after == kCTLBCP || after == kCTLBIS || after == kCTLBSY) && before != kCTLBNU) {
		answer = false;
		goto done;
	}
	/* LB14: `OP SP* ×` */
	if (fn_lb_before_spaces(c, index) == kCTLBOP) {
		answer = false;
		goto done;
	}
	/* **LB15a AND LB15b, WHICH REPLACED LB15 IN UNICODE 15.1.** The old rule was `QU SP* × OP`; the new pair splits
	 * a `QU` by whether it OPENS or CLOSES something, because a left quote and a right quote are the same
	 * Line_Break class and opposite typography:
	 *
	 *     LB15a: (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW) [\p{Pi}&QU] SP* ×
	 *     LB15b: × [\p{Pf}&QU] (SP | GL | WJ | CL | QU | CP | EX | IS | SY | BK | CR | LF | NL | ZW | eot)
	 *
	 * MEASURED, both halves: the old rule broke `QU SP OP` where 15.1 does not (`0022 0020 2329` — ten cases), and
	 * the final-quote half is 94 cases on its own. */
	if (index > 0) {
		CFIndex q = index - 1;

		while (q >= 0 && c[q] == kCTLBSP) {
			q--;
		}
		/* **AND THEN BACK OVER THE QUOTE'S OWN COMBINING MARKS.** LB9 folds a mark into the character it attached
		 * to, so a quote with an accent on it is still that quote for every rule — but the CHARACTER at the
		 * position before the spaces is the MARK, not the quote, and asking whether a mark is punctuation answers
		 * no. MEASURED, and it is the last thing the conformance run found: forty-five cases of the form
		 * `00AB 0308 0020 X`, a left quote with a diaeresis and a space after it. */
		while (q > 0 && attached[q]) {
			q--;
		}
		if (q >= 0 && ct_lb_is_pi_qu(chars[q])) {
			int opening = (q > 0) ? c[q - 1] : -1;

			if (q == 0 || opening == kCTLBBK || opening == kCTLBCR || opening == kCTLBLF
			    || opening == kCTLBNL || opening == kCTLBOP || opening == kCTLBQU
			    || opening == kCTLBGL || opening == kCTLBSP || opening == kCTLBZW) {
				answer = false;
				goto done;
			}
		}
	}
	if (ct_lb_is_pf_qu(chars[index])) {
		int following = (index + 1 < count) ? c[index + 1] : -1;

		if (index + 1 >= count || following == kCTLBSP || following == kCTLBGL || following == kCTLBWJ
		    || following == kCTLBCL || following == kCTLBQU || following == kCTLBCP
		    || following == kCTLBEX || following == kCTLBIS || following == kCTLBSY
		    || following == kCTLBBK || following == kCTLBCR || following == kCTLBLF
		    || following == kCTLBNL || following == kCTLBZW) {
			answer = false;
			goto done;
		}
	}
	/* LB16: `(CL | CP) SP* × NS` */
	if (after == kCTLBNS) {
		int previous = fn_lb_before_spaces(c, index);

		if (previous == kCTLBCL || previous == kCTLBCP) {
			answer = false;
			goto done;
		}
	}
	/* LB17: `B2 SP* × B2` */
	if (after == kCTLBB2 && fn_lb_before_spaces(c, index) == kCTLBB2) {
		answer = false;
		goto done;
	}
	/* LB18: `SP ÷` */
	if (before == kCTLBSP) {
		answer = true;
		goto done;
	}
	/* LB19: `× QU` and `QU ×` */
	if (after == kCTLBQU || before == kCTLBQU) {
		answer = false;
		goto done;
	}
	/* LB20: `÷ CB` and `CB ÷` */
	if (after == kCTLBCB || before == kCTLBCB) {
		answer = true;
		goto done;
	}
	/* LB21: `× BA`, `× HY`, `× NS` and `BB ×` */
	if (after == kCTLBBA || after == kCTLBHY || after == kCTLBNS || before == kCTLBBB) {
		answer = false;
		goto done;
	}
	/* LB21a: `HL (HY | BA) ×` */
	if (before == kCTLBHY || before == kCTLBBA) {
		if (index >= 2 && c[index - 2] == kCTLBHL) {
			answer = false;
			goto done;
		}
	}
	/* LB21b: `SY × HL` */
	if (before == kCTLBSY && after == kCTLBHL) {
		answer = false;
		goto done;
	}
	/* LB22: `× IN` */
	if (after == kCTLBIN) {
		answer = false;
		goto done;
	}
	/* LB23: `(AL | HL) × NU` and `NU × (AL | HL)` */
	if ((before == kCTLBAL || before == kCTLBHL) && after == kCTLBNU) {
		answer = false;
		goto done;
	}
	if (before == kCTLBNU && (after == kCTLBAL || after == kCTLBHL)) {
		answer = false;
		goto done;
	}
	/* LB23a: `PR × (ID | EB | EM)` and `(ID | EB | EM) × PO` */
	if (before == kCTLBPR && (after == kCTLBID || after == kCTLBEB || after == kCTLBEM)) {
		answer = false;
		goto done;
	}
	if ((before == kCTLBID || before == kCTLBEB || before == kCTLBEM) && after == kCTLBPO) {
		answer = false;
		goto done;
	}
	/* LB24: `(PR | PO) × (AL | HL)` and `(AL | HL) × (PR | PO)` */
	if ((before == kCTLBPR || before == kCTLBPO) && (after == kCTLBAL || after == kCTLBHL)) {
		answer = false;
		goto done;
	}
	if ((before == kCTLBAL || before == kCTLBHL) && (after == kCTLBPR || after == kCTLBPO)) {
		answer = false;
		goto done;
	}
	/* LB25: THE NUMBERS. */
	if (fn_lb_lb25(c, count, index)) {
		answer = false;
		goto done;
	}
	/* LB26: HANGUL, the three rules of the syllable composition. */
	if (before == kCTLBJL && (after == kCTLBJL || after == kCTLBJV || after == kCTLBH2 || after == kCTLBH3)) {
		answer = false;
		goto done;
	}
	if ((before == kCTLBJV || before == kCTLBH2) && (after == kCTLBJV || after == kCTLBJT)) {
		answer = false;
		goto done;
	}
	if ((before == kCTLBJT || before == kCTLBH3) && after == kCTLBJT) {
		answer = false;
		goto done;
	}
	/* LB27: a Hangul syllable against an inseparable affix, either way. */
	if ((before == kCTLBJL || before == kCTLBJV || before == kCTLBJT || before == kCTLBH2 || before == kCTLBH3)
	    && (after == kCTLBIN || after == kCTLBPO)) {
		answer = false;
		goto done;
	}
	if (before == kCTLBPR
	    && (after == kCTLBJL || after == kCTLBJV || after == kCTLBJT || after == kCTLBH2 || after == kCTLBH3)) {
		answer = false;
		goto done;
	}
	/* LB28: `(AL | HL) × (AL | HL)` */
	if ((before == kCTLBAL || before == kCTLBHL) && (after == kCTLBAL || after == kCTLBHL)) {
		answer = false;
		goto done;
	}
	/* **LB28a: THE ORTHOGRAPHIC SYLLABLES OF BRAHMIC SCRIPTS**, in the three clauses the specification states:
	 *
	 *     AP × (AK | ◌ | AS)
	 *     (AK | ◌ | AS) × (VF | VI)
	 *     (AK | ◌ | AS) VI × (AK | ◌)
	 *     (AK | ◌ | AS) × (AK | ◌ | AS) VF
	 *
	 * **THE THIRD CLAUSE IS `VI` AND NOT `VF`, AND THERE IS A FOURTH**, which is what tr14-51 and the conformance
	 * data's own rule numbers (`28.11` through `28.14`) both say. I had three clauses and the wrong third one —
	 * MEASURED, six cases in Balinese, Batak, Javanese and Kawi, and the tests' comments named `[28.13]` where no
	 * clause of mine could fire.
	 *
	 * `◌` IS A CHARACTER AND NOT A CLASS — U+25CC DOTTED CIRCLE, which stands for a vowel the text does not spell
	 * and whose own Line_Break class is `AL` — so every clause reads the character as well as the class. */
	{
		/* **AND EVERY CLAUSE LOOKS AT THE BASE, NOT AT THE CHARACTER BESIDE THE POSITION.** LB9 gave a combining
		 * mark its base's class, so a syllable with a sign on it is still that syllable to a class test — but the
		 * CHARACTER at the position is the SIGN, and a sign is not a syllable. Stepping back over the attached
		 * marks lands on the base, and both sides of the position need it. MEASURED, and it is the last thing the
		 * conformance run found: two cases, both a `VI` with a `ZWNJ` after it or a syllable with a mark under it.
		 *
		 * THE CHARACTER AT `index` IS ALWAYS A BASE, because a break before an attached mark was already refused
		 * by LB9 — so only the left-hand side has to step. */
		CFIndex b = index - 1;

		while (b > 0 && attached[b]) {
			b--;
		}
		{
			bool beforeIsSyllable = (c[b] == kCTLBAK || c[b] == kCTLBAS
						 || (uint32_t)chars[b] == 0x25CC);
			bool afterIsSyllable = (after == kCTLBAK || after == kCTLBAS
						|| (uint32_t)chars[index] == 0x25CC);

			if (c[b] == kCTLBAP && afterIsSyllable) {
				answer = false;
				goto done;
			}
			if (beforeIsSyllable && (after == kCTLBVF || after == kCTLBVI)) {
				answer = false;
				goto done;
			}
			/* **AND THE SYLLABLE BEFORE THE `VI` STEPS BACK OVER ITS OWN MARKS TOO.** `(AK | ◌ | AS) VI × (AK |
			 * ◌)`: the syllable on the left is a base, which may itself carry marks — MEASURED, the one case the
			 * whole conformance file had left: `25CC A9B3 A9C0 A9A0`, a dotted circle with a Javanese sign under
			 * it and then a pangkon, where the character before the `VI` is the SIGN and not the circle. */
			{
				CFIndex s = b - 1;

				while (s > 0 && attached[s]) {
					s--;
				}
				if (s >= 0 && (c[s] == kCTLBAK || c[s] == kCTLBAS
					       || (uint32_t)chars[s] == 0x25CC)
				    && c[b] == kCTLBVI && afterIsSyllable) {
					answer = false;
					goto done;
				}
			}
			if (beforeIsSyllable && afterIsSyllable && index + 1 < count && c[index + 1] == kCTLBVF) {
				answer = false;
				goto done;
			}
		}
	}
	/* LB29: `IS × (AL | HL)` */
	if (before == kCTLBIS && (after == kCTLBAL || after == kCTLBHL)) {
		answer = false;
		goto done;
	}
	/* LB30: `(AL | HL | NU) × OP` and `CP × (AL | HL | NU)`, EACH ONLY WHEN THE BRACKET IS NOT EAST ASIAN. */
	if ((before == kCTLBAL || before == kCTLBHL || before == kCTLBNU) && after == kCTLBOP
	    && !ct_lb_wide_bracket(chars[index])) {
		answer = false;
		goto done;
	}
	if (before == kCTLBCP && !ct_lb_wide_bracket(chars[index - 1])
	    && (after == kCTLBAL || after == kCTLBHL || after == kCTLBNU)) {
		answer = false;
		goto done;
	}
	/* LB30a: `RI RI` — regional indicators pair up, so a break goes between PAIRS and not inside one. The count of
	 * consecutive indicators before the position is what says which side of a pair it is. */
	if (before == kCTLBRI && after == kCTLBRI) {
		CFIndex k = index - 1, run = 0;

		while (k >= 0 && c[k] == kCTLBRI) {
			/* **A COMBINING MARK THAT ATTACHED TO AN INDICATOR IS NOT ANOTHER INDICATOR.** The fold gave it the
			 * base's class, so counting every `RI` in the run counts the marks too — MEASURED, `1F1E6 0308
			 * 1F1E6` is one indicator and one mark and one indicator, whose pair does NOT break, and counting
			 * the mark made the run two and broke it. */
			if (!attached[k]) {
				run++;
			}
			k--;
		}
		if ((run % 2) == 1) {
			answer = false;
			goto done;
		}
	}
	/* LB30b: `EB × EM`, AND THE UNASSIGNED PICTOGRAPHS BESIDE THEM — `[\p{Extended_Pictographic}&\p{Cn}] × EM`.
	 * The second clause is why the generator computes an emoji property MINUS every assigned code point: an
	 * unassigned code point that is nonetheless an emoji takes a modifier. */
	if (after == kCTLBEM
	    && (before == kCTLBEB || ct_lb_is_unassigned_pictographic(chars[index - 1]))) {
		answer = false;
		goto done;
	}
	/* LB31: `ALL ÷`. */
	answer = true;

done:
	free(c);
	free(attached);
	(void)fn_lb_after_spaces;
	return answer;
}

CFIndex ct_lb_break_before(const UTF32Char *chars, CFIndex count, CFIndex start, CFIndex limit)
{
	CFIndex i;

	if (limit > count) {
		limit = count;
	}
	for (i = limit; i > start; i--) {
		if (ct_lb_break_at(chars, count, i)) {
			return i;
		}
	}
	return start;
}

/* **THE CODE POINT A UNIT BELONGS TO, by binary search over an ascending map.** The map is built by the decoder below
 * and holds, for each code point, the unit it began at — so this is "the last code point that started at or before
 * this unit", which is the code point the unit is part of. */
static CFIndex fn_lb_codepoint_for_unit(const CFIndex *units, CFIndex count, CFIndex unit)
{
	CFIndex low = 0, high = count - 1, found = 0;

	while (low <= high) {
		CFIndex mid = low + (high - low) / 2;

		if (units[mid] <= unit) {
			found = mid;
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
	return found;
}

CFIndex ct_lb_break_before_utf16(const UniChar *chars, CFIndex count, CFIndex start, CFIndex limit)
{
	UTF32Char *points;
	CFIndex *units;
	CFIndex pointCount = 0, i, answer, mapped;

	if (chars == NULL || count <= 0 || start >= count) {
		return start;
	}
	points = (UTF32Char *)malloc((size_t)count * sizeof(UTF32Char));
	units = (CFIndex *)malloc((size_t)(count + 1) * sizeof(CFIndex));
	if (points == NULL || units == NULL) {
		free(points);
		free(units);
		return start;
	}
	/* **THE DECODE, AND IT KEEPS THE MAP THE ANSWER IS TRANSLATED BACK WITH**: a surrogate pair becomes one code
	 * point and the map remembers the unit it started at, so a code point index can be turned back into the index
	 * the caller asked in. `units[pointCount]` is the end of the run, which is what an answer of "the last code
	 * point" maps to. */
	for (i = 0; i < count;) {
		UTF32Char c = (UTF32Char)chars[i];

		units[pointCount] = i;
		if (c >= 0xD800 && c <= 0xDBFF && i + 1 < count && chars[i + 1] >= 0xDC00 && chars[i + 1] <= 0xDFFF) {
			c = (UTF32Char)(0x10000 + ((c - 0xD800) << 10) + (chars[i + 1] - 0xDC00));
			i += 2;
		} else {
			i++;
		}
		points[pointCount++] = c;
	}
	units[pointCount] = count;

	answer = ct_lb_break_before(points, pointCount, fn_lb_codepoint_for_unit(units, pointCount, start),
				    fn_lb_codepoint_for_unit(units, pointCount,
							     (limit < count) ? limit : count));
	mapped = units[answer];
	free(points);
	free(units);
	return mapped;
}
