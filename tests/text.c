/*
 * Core Text — the facts of the surface written so far: CTFontTraits' values and classes, and
 * CTFontDescriptor's twenty-two attribute keys.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * WHY THIS IS THE FIRST CASE IN THIS LIBRARY. The values in `CTFontTraits.h` are the one part of that header a
 * reader cannot check by looking, and the library that came before this one recorded its own values as "the
 * era's declaration ORDER ... Apple publishes case names and no values" — a rule that is FALSE for this header
 * and that happened to land on the right numbers. A case that names the numbers is what tells the two apart.
 *
 * ONE CHILD PER CASE, which is this tree's convention (libagora-drawing's suite) and not decoration: a wrong
 * answer is counted, a crash is not, and a crash in a single-process run would lose every case after it.
 */
#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFontTraits.h>

#include <CoreFoundation/CoreFoundation.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_checks;
static int g_fail;

static void check(int ok, const char *fmt, ...)
{
	va_list ap;

	g_checks++;
	if (ok) {
		return;
	}
	g_fail++;
	va_start(ap, fmt);
	fprintf(stderr, "FAIL: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

/* THE SYMBOLIC TRAITS, THE CLASS SHIFT AND THE ELEVEN CLASSES — every value Apple's header prints, named
 * here so that a change to any of them is a failing case rather than a silent one. */
static int case_symbolic_trait_values(void)
{
	int before = g_fail;

	check(kCTFontItalicTrait == 1, "kCTFontItalicTrait is %d, want 1", (int)kCTFontItalicTrait);
	check(kCTFontBoldTrait == 2, "kCTFontBoldTrait is %d, want 2", (int)kCTFontBoldTrait);
	check(kCTFontExpandedTrait == 32, "kCTFontExpandedTrait is %d, want 32", (int)kCTFontExpandedTrait);
	check(kCTFontCondensedTrait == 64, "kCTFontCondensedTrait is %d, want 64", (int)kCTFontCondensedTrait);
	check(kCTFontMonoSpaceTrait == 1024, "kCTFontMonoSpaceTrait is %d, want 1024",
	      (int)kCTFontMonoSpaceTrait);
	check(kCTFontVerticalTrait == 2048, "kCTFontVerticalTrait is %d, want 2048",
	      (int)kCTFontVerticalTrait);
	check(kCTFontUIOptimizedTrait == 4096, "kCTFontUIOptimizedTrait is %d, want 4096",
	      (int)kCTFontUIOptimizedTrait);

	check(kCTFontClassMaskShift == 28, "kCTFontClassMaskShift is %d, want 28",
	      (int)kCTFontClassMaskShift);
	check(kCTFontClassMaskTrait == (CTFontSymbolicTraits)(15u << 28),
	      "kCTFontClassMaskTrait is 0x%08x, want 0xf0000000", (unsigned)kCTFontClassMaskTrait);

	/* THE CLASS ITSELF IS THE TOP FOUR BITS, so the value of a class is its OpenType number UP THERE. The
	 * assertion uses the shifted literal rather than the symbol, which is what makes it a check of the
	 * position and not a restatement of the definition. */
	check((CTFontSymbolicTraits)kCTFontOldStyleSerifsClass == (1u << 28),
	      "kCTFontOldStyleSerifsClass is 0x%08x, want 0x10000000",
	      (unsigned)kCTFontOldStyleSerifsClass);
	check((CTFontSymbolicTraits)kCTFontSymbolicClass == (12u << 28),
	      "kCTFontSymbolicClass is 0x%08x, want 0xc0000000", (unsigned)kCTFontSymbolicClass);
	check(kCTFontUnknownClass == 0, "kCTFontUnknownClass is 0x%08x, want 0",
	      (unsigned)kCTFontUnknownClass);

	/* AND THE GAPS AT 6 AND 11 ARE THE 'OS/2' ASSIGNMENTS SKIPPING THEM — kept rather than closed up. A
	 * library that renumbered the classes to remove the gaps would be noticed at the two values and nowhere
	 * else, which is why they are asserted. */
	check((CTFontSymbolicTraits)kCTFontSlabSerifsClass != (6u << 28),
	      "a class took the value 6 << 28, which no OpenType class has");
	check((CTFontSymbolicTraits)kCTFontScriptsClass != (11u << 28),
	      "a class took the value 11 << 28, which no OpenType class has");

	check(sizeof(CTFontSymbolicTraits) == 4 && sizeof(CTFontStylisticClass) == 4,
	      "the traits are %zu and %zu bytes, want 4 and 4", sizeof(CTFontSymbolicTraits),
	      sizeof(CTFontStylisticClass));
	return g_fail - before;
}

/* THE FOUR STRINGS. They are the AppKit's vocabulary — the names `NSFontDescriptor` uses for the same four
 * traits — and there is no local counterpart to compare against yet, so they are asserted as literals. */
static int case_trait_key_strings(void)
{
	int before = g_fail;
	/* AUTOMATIC, NEITHER static NOR const: the trait keys are `extern const CFStringRef` VARIABLES, so their
	 * values are not constant expressions and a static or const aggregate cannot be initialised from them.
	 * THAT IS A MISTAKE MADE TWICE IN THIS TREE — libagora-drawing's suite carries the same note, added after
	 * the same error, and this file made it again before this line was written. The note is repeated rather
	 * than referenced because it has now failed to carry once. */
	struct { CFStringRef key; const char *want; } keys[] = {
		{ kCTFontSymbolicTrait, "NSFontSymbolicTrait" },
		{ kCTFontWeightTrait, "NSFontWeightTrait" },
		{ kCTFontWidthTrait, "NSFontWidthTrait" },
		{ kCTFontSlantTrait, "NSFontSlantTrait" },
	};
	size_t i;

	for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
		char buf[64];

		check(keys[i].key != NULL, "trait key %zu is NULL", i);
		if (keys[i].key == NULL) {
			continue;
		}
		buf[0] = '\0';
		check(CFStringGetCString(keys[i].key, buf, (CFIndex)sizeof buf, kCFStringEncodingUTF8) &&
		      strcmp(buf, keys[i].want) == 0,
		      "a trait key is \"%s\", want \"%s\"", buf, keys[i].want);
		check(CFGetTypeID(keys[i].key) == CFStringGetTypeID(), "trait key %zu is not a CFString", i);
	}

	/* AND THEY ARE FOUR DISTINCT KEYS RATHER THAN ONE STRING REUSED. */
	check(!CFEqual(keys[0].key, keys[1].key) && !CFEqual(keys[1].key, keys[2].key) &&
	      !CFEqual(keys[2].key, keys[3].key),
	      "two of the four trait keys are the same string");
	return g_fail - before;
}

/* ------------------------------------------------------------------------ */
/* the runner                                                                */
/* ------------------------------------------------------------------------ */

static int run_in_child(const char *name, int (*fn)(void))
{
	pid_t pid;
	int status;

	fflush(NULL);
	pid = fork();
	if (pid == 0) {
		int rc = fn();

		_exit((rc == 0 && g_fail == 0) ? 0 : 1);
	}
	if (pid < 0) {
		fprintf(stderr, "FAIL: %s: fork failed\n", name);
		return 1;
	}
	if (waitpid(pid, &status, 0) < 0) {
		fprintf(stderr, "FAIL: %s: waitpid failed\n", name);
		return 1;
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		printf("ok   %s\n", name);
		return 0;
	}
	if (WIFSIGNALED(status)) {
		fprintf(stderr, "FAIL: %s: killed by signal %d\n", name, WTERMSIG(status));
	} else {
		fprintf(stderr, "FAIL: %s: checks failed\n", name);
	}
	return 1;
}

/* THE DESCRIPTOR'S KEY STRINGS, WHICH ARE A RECORDED GUESS RATHER THAN A FACT (dec-4795696b09d362f6) and
 * are therefore worth holding precisely: the eight that pair with an AppKit constant are the ones with a
 * reason behind them, and the other fourteen follow the same form. A change to any of them should be a
 * deliberate act, so each is named. */
static int case_descriptor_key_strings(void)
{
	int before = g_fail;
	/* AUTOMATIC, NOT static AND NOT const: these keys are `extern const CFStringRef` VARIABLES, so their
	 * values are not constant expressions. The same note is in libagora-drawing's suite, where the same
	 * error was made; it is repeated rather than referenced because it has already failed to carry once. */
	struct { CFStringRef key; const char *want; } keys[] = {
		{ kCTFontURLAttribute, "NSFontURLAttribute" },
		{ kCTFontNameAttribute, "NSFontNameAttribute" },
		{ kCTFontDisplayNameAttribute, "NSFontDisplayNameAttribute" },
		{ kCTFontFamilyNameAttribute, "NSFontFamilyNameAttribute" },
		{ kCTFontStyleNameAttribute, "NSFontStyleNameAttribute" },
		{ kCTFontTraitsAttribute, "NSFontTraitsAttribute" },
		{ kCTFontVariationAttribute, "NSFontVariationAttribute" },
		{ kCTFontSizeAttribute, "NSFontSizeAttribute" },
		{ kCTFontMatrixAttribute, "NSFontMatrixAttribute" },
		{ kCTFontCascadeListAttribute, "NSFontCascadeListAttribute" },
		{ kCTFontCharacterSetAttribute, "NSFontCharacterSetAttribute" },
		{ kCTFontLanguagesAttribute, "NSFontLanguagesAttribute" },
		{ kCTFontBaselineAdjustAttribute, "NSFontBaselineAdjustAttribute" },
		{ kCTFontMacintoshEncodingsAttribute, "NSFontMacintoshEncodingsAttribute" },
		{ kCTFontFeaturesAttribute, "NSFontFeaturesAttribute" },
		{ kCTFontFeatureSettingsAttribute, "NSFontFeatureSettingsAttribute" },
		{ kCTFontFixedAdvanceAttribute, "NSFontFixedAdvanceAttribute" },
		{ kCTFontOrientationAttribute, "NSFontOrientationAttribute" },
		{ kCTFontFormatAttribute, "NSFontFormatAttribute" },
		{ kCTFontRegistrationScopeAttribute, "NSFontRegistrationScopeAttribute" },
		{ kCTFontPriorityAttribute, "NSFontPriorityAttribute" },
		{ kCTFontEnabledAttribute, "NSFontEnabledAttribute" },
	};
	size_t i, j;

	for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
		char buf[96];

		check(keys[i].key != NULL, "descriptor key %zu is NULL", i);
		if (keys[i].key == NULL) {
			continue;
		}
		buf[0] = '\0';
		check(CFStringGetCString(keys[i].key, buf, (CFIndex)sizeof buf, kCFStringEncodingUTF8) &&
		      strcmp(buf, keys[i].want) == 0,
		      "a descriptor key is \"%s\", want \"%s\"", buf, keys[i].want);
		for (j = i + 1; j < sizeof keys / sizeof keys[0]; j++) {
			if (CFEqual(keys[i].key, keys[j].key)) {
				check(0, "descriptor keys %zu and %zu are the same string", i, j);
			}
		}
	}
	return g_fail - before;
}

static const struct { const char *name; int (*fn)(void); } g_cases[] = {
	{ "The symbolic traits carry Apple's published values", case_symbolic_trait_values },
	{ "The four trait keys are the AppKit's strings", case_trait_key_strings },
	{ "The descriptor's twenty-two key strings", case_descriptor_key_strings },
};

int main(void)
{
	size_t i, total = sizeof g_cases / sizeof g_cases[0];
	int passed = 0, failed = 0;

	setvbuf(stdout, NULL, _IONBF, 0);
	for (i = 0; i < total; i++) {
		if (run_in_child(g_cases[i].name, g_cases[i].fn) == 0) {
			passed++;
		} else {
			failed++;
		}
	}
	printf("text: %d of %d cases passed, %d failed\n", passed, passed + failed, failed);
	return failed == 0 ? 0 : 1;
}
