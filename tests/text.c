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
/* EVERY HEADER THE SUITE EXERCISES IS INCLUDED, one line each: a case that calls a door must be able to
 * see it, and forgetting one costs twenty errors in this file rather than one. */
/* THE UMBRELLA IS INCLUDED FIRST, so that every door below is reachable THROUGH IT: the one-line include Apple's
 * framework offers is proved by this translation unit compiling, and it is included before the individual headers
 * rather than after them for that reason. */
#include <CoreText/CoreText.h>
#include <CoreFoundation/CFPropertyList.h>
#include <math.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFData.h>
#include <CoreText/CTFontService_internal.h>
/* AND THE MATCHING TIER'S WALK, WHICH TWO CASES READ SO THAT "the first face in the tier's order" is asserted
 * against the order itself rather than against a family name this machine happens to hold first. */
#include <CoreText/CTFontMatching_internal.h>
#include <CoreText/CTFont_internal.h>
#include <CoreText/CTBidi_internal.h>
/* **AND THE SHAPING ENGINE, WHICH IS THE SUBSTRATE THE LAYOUT WILL SIT ON RATHER THAN BE WIRED INTO IT YET** — the
 * same standing `CTBidi_internal.h` had while the bidi milestone was being built rule by rule. */
#include <CoreText/CTShaping_internal.h>
#include <CoreText/CTFont.h>
/* AND THE ENGINE'S INTERNAL HEADER, FOR THE DOORS CORE TEXT OWNS BUT DOES NOT IMPLEMENT: the Unicode
 * glyph step and the face registry. */
#include <CoreGraphics/CGFont_internal.h>
#include <CoreText/CTFontCollection.h>
#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFontManager.h>
#include <CoreText/CTFontManagerErrors.h>
#include <CoreText/CTFontTraits.h>
#include <CoreText/CTLine.h>
#include <CoreText/CTParagraphStyle.h>
#include <CoreText/CTRun.h>
#include <CoreText/CTTypesetter.h>
#include <CoreText/CTFrame.h>
#include <CoreText/CTFramesetter.h>
#include <CoreText/CTStringAttributes.h>
#include <CoreText/CTTextTab.h>
#include <CoreText/CTGlyphInfo.h>

#include <CoreFoundation/CoreFoundation.h>

/* THE TWO CORE GRAPHICS HEADERS THE DRAWING CASE NEEDS, WHICH ARE THE SUITE'S AND NOT THE LIBRARY'S: the line's
 * draw door takes a `CGContextRef` the suite has to MAKE, and making one means a bitmap context and a colour
 * space. (`math.h` was here for a moment and is gone: the comparisons in the cases are written as differences
 * against a tolerance rather than as `fabs`, so nothing needs it.) */
#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGColorSpace.h>

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

/* THE FIRST OBJECT IN THIS LIBRARY, AND THE ASSERTION THAT MATTERS MOST IS ITS IDENTITY: Apple's contract
 * is that CFGetTypeID of a descriptor IS CTFontDescriptorGetTypeID(), which is only true when the identity
 * comes from Core Foundation's runtime. The deleted library answered every type id from a table of constants
 * kept in a file beside the objects, and this case is what tells the two arrangements apart. */
static int case_descriptor_object(void)
{
	int before = g_fail;
	CFMutableDictionaryRef attributes = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFMutableDictionaryRef additions = CFDictionaryCreateMutable(
		kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFStringRef name = CFSTR("Helvetica");
	const CGFloat twelve = 12.0;
	CFNumberRef size;
	CTFontDescriptorRef descriptor, same, copy, zero;
	CFTypeRef got;

	check(attributes != NULL && additions != NULL, "the test could not make its dictionaries");
	if (attributes == NULL || additions == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attributes, kCTFontNameAttribute, name);
	descriptor = CTFontDescriptorCreateWithAttributes(attributes);
	check(descriptor != NULL, "a descriptor could not be made from an attributes dictionary");
	if (descriptor == NULL) {
		return g_fail - before;
	}

	/* THE RUNTIME OWNS THE IDENTITY. */
	check(CFGetTypeID((CFTypeRef)descriptor) == CTFontDescriptorGetTypeID(),
	      "CFGetTypeID(descriptor) is %lu and CTFontDescriptorGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)descriptor),
	      (unsigned long)CTFontDescriptorGetTypeID());

	/* THE ACCESSORS, INCLUDING THE ONE THAT MUST ANSWER NOTHING RATHER THAN SOMETHING. */
	got = CTFontDescriptorCopyAttribute(descriptor, kCTFontNameAttribute);
	check(got != NULL && CFEqual(got, name), "the name attribute did not come back");
	check(CTFontDescriptorCopyAttribute(descriptor, kCTFontSizeAttribute) == NULL,
	      "an attribute that was never set came back with a value");

	/* THE COPY REPLACES WHAT IT NAMES AND KEEPS WHAT IT DOES NOT. */
	size = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &twelve);
	check(size != NULL, "the test could not make a CFNumber");
	if (size != NULL) {
		CFDictionarySetValue(additions, kCTFontSizeAttribute, size);
		copy = CTFontDescriptorCreateCopyWithAttributes(descriptor, additions);
		check(copy != NULL, "the copy door answered nothing");
		if (copy != NULL) {
			check(CTFontDescriptorCopyAttribute(copy, kCTFontSizeAttribute) != NULL,
			      "the copy did not take the size it was given");
			check(CTFontDescriptorCopyAttribute(copy, kCTFontNameAttribute) != NULL,
			      "the copy LOST the name the original had");
			check(!CFEqual(descriptor, copy),
			      "a copy with a different size compares equal to the original");
		}
		CFRelease(size);
	}

	/* EQUALITY IS BY VALUE: the attributes ARE the descriptor. */
	same = CTFontDescriptorCreateWithAttributes(attributes);
	check(same != NULL && CFEqual(descriptor, same),
	      "two descriptors made from the same attributes are not equal");
	check(same != NULL && CFHash(descriptor) == CFHash(same),
	      "two equal descriptors hash differently");

	/* AND A ZERO SIZE IS OMITTED RATHER THAN STORED, which is Apple's own sentence for that door. */
	zero = CTFontDescriptorCreateWithNameAndSize(name, 0.0);
	check(zero != NULL, "the name-and-size door answered nothing");
	check(zero != NULL && CTFontDescriptorCopyAttribute(zero, kCTFontSizeAttribute) == NULL,
	      "a size of 0.0 was STORED rather than omitted");
	check(zero != NULL && CTFontDescriptorCopyAttribute(zero, kCTFontNameAttribute) != NULL,
	      "the name-and-size door did not store the name");
	return g_fail - before;
}

/* ONE FACE, FOUND THE SAME TWO WAYS IN EVERY CASE THAT NEEDS ONE: Apple's CTFontCreateWithName asks a font
 * system for a face by name, this tier has no font system, so the name is offered to the face registry the
 * engine keeps and, if that finds nothing, the face is loaded from its own file. WHICH ROUTE ANSWERED IS
 * PRINTED RATHER THAN ASSUMED, because it is a fact about this tree that the suite can measure every run and
 * a comment cannot -- printed once per process, because every case runs in its own child and the answer is
 * the same one.
 *
 * THE FACE IS REGISTERED BEFORE THE NAME IS TRIED, which is what gives the name route something to find: an
 * earlier attempt answered NULL because the face was unregistered, and a NULL there means exactly that. */
static CTFontRef test_font(CGFloat size)
{
	static bool reported;
	const char *path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf";
	CGDataProviderRef provider;
	CGFontRef face = NULL;
	CTFontRef font;

	if (access(path, R_OK) == 0) {
		provider = CGDataProviderCreateWithFilename(path);
		if (provider != NULL) {
			face = CGFontCreateWithDataProvider(provider);
		}
		check(cg_font_register(path) != 0 || cg_font_is_registered(path),
		      "the face at %s could not be registered", path);
	} else if (!reported) {
		printf("  (note: %s is not on this machine)\n", path);
	}

	font = CTFontCreateWithName(CFSTR("DejaVuSansMono-Bold"), size, NULL);
	if (font == NULL && face != NULL) {
		if (!reported) {
			printf("  (note: the NAME route found no face for \"%s\" even when registered; using the file)\n",
			       "DejaVuSansMono-Bold");
		}
		font = CTFontCreateWithGraphicsFont(face, size, NULL, NULL);
	} else if (font != NULL && !reported) {
		printf("  (note: the NAME route found the face through the engine's registry)\n");
	}
	reported = true;
	return font;
}

/* A REAL FONT, END TO END, WITH THE GLYPH STEP THAT IS THIS LIBRARY'S WHOLE REASON FOR EXISTING. Everything
 * here is asserted against a face that is actually on this machine, and the door the architecture was built
 * for is CTFontGetGlyphsForCharacters: Core Graphics no longer offers it, Core Text must, and the answer has
 * to come from the engine behind CGFont.
 *
 * THE API'S THEORY OF ITSELF IS ALSO HELD: a font made from a name must NAME ITSELF back, its metrics must be
 * in POINTS (so twice the size is twice the ascent), and its identity must be the runtime's. */
static int case_font_object(void)
{
	int before = g_fail;
	CFStringRef name = CFSTR("DejaVuSansMono-Bold");
	CTFontRef font, doubled;
	CFTypeRef got;
	CGGlyph glyphs[3];
	UniChar chars[3];
	CGFloat ascent, twice;
	bool all;

	/* THE ROUTE TO THE FACE, AND THE LINE THE SUITE PRINTS SAYS WHICH ONE ANSWERED (see `test_font`). */
	font = test_font(12.0);
	check(font != NULL, "a font could not be made from \"DejaVuSansMono-Bold\", by name or from its file");
	if (font == NULL) {
		return g_fail - before;
	}

	/* THE IDENTITY IS THE RUNTIME'S, exactly as for a descriptor. */
	check(CFGetTypeID((CFTypeRef)font) == CTFontGetTypeID(),
	      "CFGetTypeID(font) is %lu and CTFontGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)font), (unsigned long)CTFontGetTypeID());

	/* IT NAMES ITSELF, THROUGH ONE OF THE TWO KEYS CORE GRAPHICS ANSWERS DIRECTLY. */
	got = CTFontCopyPostScriptName(font);
	check(got != NULL, "the font did not answer its own PostScript name");
	if (got != NULL) {
		check(CFEqual(got, name), "the font answered a PostScript name that is not the one it was made from");
		CFRelease(got);
	}

	/* THE NAME TABLE, THROUGH A KEY WITH NO CORE GRAPHICS DOOR: the family comes from `name` entry 1. */
	got = CTFontCopyFamilyName(font);
	check(got != NULL, "the font did not answer its family name from the name table");
	if (got != NULL) {
		check(CFGetTypeID(got) == CFStringGetTypeID(), "the family name is not a string");
		CFRelease(got);
	}
	check(CTFontCopyName(font, CFSTR("no such key")) == NULL,
	      "a key this library does not know answered a name anyway");

	/* THE METRICS ARE IN POINTS: twice the size, twice the ascent. */
	ascent = CTFontGetAscent(font);
	check(ascent > 0.0, "the ascent is %g, want a positive number of points", (double)ascent);
	check(CTFontGetUnitsPerEm(font) > 0, "the font reports %u units per em",
	      (unsigned)CTFontGetUnitsPerEm(font));
	check(CTFontGetGlyphCount(font) > 0, "the font reports %ld glyphs",
	      (long)CTFontGetGlyphCount(font));
	doubled = CTFontCreateWithName(name, 24.0, NULL);
	twice = (doubled != NULL) ? CTFontGetAscent(doubled) : 0.0;
	check(doubled != NULL && twice > ascent * 1.9 && twice < ascent * 2.1,
	      "the ascent at 24 points is %g and at 12 points %g, want about twice", (double)twice,
	      (double)ascent);

	/* AND THE DOOR THIS LIBRARY EXISTS FOR. 'A' and 'a' must map to different glyphs, and Apple's contract
	 * is the BOOLEAN: true only when every character mapped. */
	chars[0] = (UniChar)'A';
	chars[1] = (UniChar)'a';
	chars[2] = (UniChar)0xFFFF;
	all = CTFontGetGlyphsForCharacters(font, chars, glyphs, 2);
	check(all, "the font could not encode 'A' and 'a', which it must be able to");
	check(glyphs[0] != 0 && glyphs[1] != 0, "'A' and 'a' did not both map: %u and %u",
	      (unsigned)glyphs[0], (unsigned)glyphs[1]);
	check(glyphs[0] != glyphs[1], "the glyph step mapped 'A' and 'a' to the SAME glyph %u",
	      (unsigned)glyphs[0]);
	all = CTFontGetGlyphsForCharacters(font, chars, glyphs, 3);
	check(!all, "the font claimed to encode an unassigned code point");
	check(glyphs[2] == 0, "the unassigned code point mapped to glyph %u, want 0", (unsigned)glyphs[2]);

	/* A GLYPH BY NAME, WHICH IS THE OTHER HALF OF THE MAPPING. */
	check(CTFontGetGlyphWithName(font, CFSTR(".notdef")) != 0 ||
	      CTFontGetGlyphWithName(font, CFSTR("A")) != 0,
	      "the font mapped neither '.notdef' nor 'A' from a glyph name");
	return g_fail - before;
}

/* THE OUTLINE: THE GLYPH AS GEOMETRY, which is the other half of the step Core Graphics' deprecated drawing
 * doors left behind, and the reason `CTFontCreatePathForGlyph` exists beside `CTFontGetGlyphsForCharacters`.
 * Apple's own sentence for the door names its three ingredients AND THEIR ORDER -- "the path will reflect the
 * font point size, matrix, and transform parameter, in that order" -- so this case holds all three on a real
 * face: the outline is scaled BY THE SIZE, carried through THE FONT'S OWN MATRIX, and moved by the caller's
 * TRANSFORM.
 *
 * AND IT HOLDS THE TWO NEGATIVES, which are what keep a plausible door from passing. A glyph with no outline
 * (a space) is an EMPTY PATH and not NULL, because that is not an error; and a glyph the engine will not load
 * IS NULL, which is the "an error occurred" of Apple's result line. A door that answered NULL for both would
 * satisfy every positive assertion here and fail at the two. */
static int case_font_outline(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	UniChar ch = (UniChar)'A';
	UniChar sp_ch = (UniChar)' ';
	CGGlyph a = 0, sp = 0;
	CGAffineTransform shift = CGAffineTransformMakeTranslation(100.0, 7.0);
	CGAffineTransform lean = CGAffineTransformMakeTranslation(30.0, 0.0);
	CTFontRef with_matrix, taller;
	CGPathRef plain, moved, matrixed, scaled, blank;
	CGRect box, shifted, lean_box, tall_box;

	check(font != NULL, "no font, so there is no outline to ask for");
	if (font == NULL) {
		return g_fail - before;
	}
	check(CTFontGetGlyphsForCharacters(font, &ch, &a, 1) && a != 0,
	      "the face has no glyph for 'A', so no outline can be asked for");

	/* A REAL OUTLINE, NON-EMPTY, WITH A BOX OF ITS OWN. */
	plain = CTFontCreatePathForGlyph(font, a, NULL);
	check(plain != NULL, "CTFontCreatePathForGlyph answered nothing for 'A'");
	if (plain == NULL) {
		return g_fail - before;
	}
	check(!CGPathIsEmpty(plain), "the outline of 'A' is an EMPTY path");
	box = CGPathGetPathBoundingBox(plain);
	check(box.size.width > 0.0 && box.size.height > 0.0,
	      "the outline of 'A' has an empty bounding box: %g by %g", (double)box.size.width,
	      (double)box.size.height);

	/* THE TRANSFORM PARAMETER MOVES IT, which is Apple's own account of what that parameter is for. */
	moved = CTFontCreatePathForGlyph(font, a, &shift);
	check(moved != NULL, "the door answered nothing when it was given a transform");
	if (moved != NULL) {
		shifted = CGPathGetPathBoundingBox(moved);
		check(shifted.origin.x > box.origin.x + 99.0 && shifted.origin.x < box.origin.x + 101.0,
		      "the transformed outline starts at x %g and the plain one at %g, want a shift of 100",
		      (double)shifted.origin.x, (double)box.origin.x);
		check(shifted.origin.y > box.origin.y + 6.0 && shifted.origin.y < box.origin.y + 8.0,
		      "the transformed outline starts at y %g and the plain one at %g, want a shift of 7",
		      (double)shifted.origin.y, (double)box.origin.y);
	}

	/* THE FONT'S OWN MATRIX IS THE MIDDLE INGREDIENT, and no other door in this library reads it -- which is
	 * why the case reads it back through CTFontGetMatrix as well as through the path. */
	with_matrix = CTFontCreateWithName(CFSTR("DejaVuSansMono-Bold"), 12.0, &lean);
	check(with_matrix != NULL, "a font could not be made with a matrix");
	if (with_matrix != NULL) {
		CGAffineTransform got = CTFontGetMatrix(with_matrix);

		check(got.tx > 29.0 && got.tx < 31.0, "the font's own matrix came back with tx = %g, want 30",
		      (double)got.tx);
		matrixed = CTFontCreatePathForGlyph(with_matrix, a, NULL);
		check(matrixed != NULL, "the font that carries a matrix answered no outline");
		if (matrixed != NULL) {
			lean_box = CGPathGetPathBoundingBox(matrixed);
			check(lean_box.origin.x > box.origin.x + 29.0 &&
			      lean_box.origin.x < box.origin.x + 31.0,
			      "the matrixed font's outline starts at x %g and the plain font's at %g, want a shift "
			      "of 30 -- the font's own matrix is NOT in the path",
			      (double)lean_box.origin.x, (double)box.origin.x);
		}
	}

	/* THE SIZE IS IN THE PATH: twice the font, twice the box. */
	taller = test_font(24.0);
	check(taller != NULL, "the 24-point font could not be made");
	if (taller != NULL) {
		scaled = CTFontCreatePathForGlyph(taller, a, NULL);
		check(scaled != NULL, "the 24-point font answered no outline");
		if (scaled != NULL) {
			tall_box = CGPathGetPathBoundingBox(scaled);
			check(tall_box.size.height > box.size.height * 1.9 &&
			      tall_box.size.height < box.size.height * 2.1,
			      "the outline is %g tall at 24 points and %g at 12, want about twice",
			      (double)tall_box.size.height, (double)box.size.height);
		}
	}

	/* A GLYPH WITH NO OUTLINE IS AN EMPTY PATH, NOT NULL. */
	if (CTFontGetGlyphsForCharacters(font, &sp_ch, &sp, 1) && sp != 0) {
		blank = CTFontCreatePathForGlyph(font, sp, NULL);
		check(blank != NULL, "the space's outline answered NULL rather than an empty path");
		if (blank != NULL) {
			check(CGPathIsEmpty(blank), "the space has a NON-empty outline");
		}
	} else {
		printf("  (note: the face maps no glyph for the space character)\n");
	}

	/* AND A GLYPH THE ENGINE WILL NOT LOAD IS NULL, which is Apple's "an error occurred". 0xFFFE is past
	 * the end of this face's glyph array, so the engine refuses it rather than answering an empty outline. */
	check(CTFontCreatePathForGlyph(font, (CGGlyph)0xFFFE, NULL) == NULL,
	      "a glyph the engine cannot load answered a path rather than NULL");
	return g_fail - before;
}

/* THE COLLECTION IS THE THIRD OBJECT THIS LIBRARY MAKES AND THE FIRST ONE THAT IS A QUESTION rather than a
 * value: it holds what it was ASKED, and the door that would answer is owed. So what a case can hold is its
 * identity, its four creators and its EQUALITY -- and equality is the interesting one, because two collections
 * that compare equal are the same QUESTION, which is only true if a collection's value is the question and not
 * the answer.
 *
 * THE TWO QUESTIONS APPLE HAS ARE KEPT APART HERE, and the assertions say so: "every face there is" and "a
 * question with no conditions" are different collections even though neither names a descriptor. */
static int case_collection_object(void)
{
	int before = g_fail;
	CFStringRef name = CFSTR("DejaVuSansMono-Bold");
	const CGFloat twelve = 12.0;
	CFNumberRef value;
	CFMutableArrayRef query, more, empty;
	CFMutableDictionaryRef options;
	CTFontDescriptorRef descriptor;
	CTFontCollectionRef all, asked, same, bare, added, untouched, with_options, with_options_again;

	empty = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	query = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	more = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	options = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	value = CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &twelve);
	check(empty != NULL && query != NULL && more != NULL && options != NULL && value != NULL,
	      "the test could not make its inputs");
	if (empty == NULL || query == NULL || more == NULL || options == NULL || value == NULL) {
		return g_fail - before;
	}
	descriptor = CTFontDescriptorCreateWithNameAndSize(name, 12.0);
	check(descriptor != NULL, "the test could not make a descriptor to ask about");
	if (descriptor == NULL) {
		return g_fail - before;
	}
	CFArrayAppendValue(query, descriptor);
	CFArrayAppendValue(more, descriptor);
	CFDictionarySetValue(options, kCTFontCollectionRemoveDuplicatesOption, value);

	/* THE IDENTITY IS THE RUNTIME'S, as it is for the descriptor and the font. */
	all = CTFontCollectionCreateFromAvailableFonts(NULL);
	check(all != NULL, "the from-available-fonts door answered nothing");
	check(all != NULL && CFGetTypeID((CFTypeRef)all) == CTFontCollectionGetTypeID(),
	      "CFGetTypeID(collection) is %lu and CTFontCollectionGetTypeID() is %lu",
	      (unsigned long)((all != NULL) ? CFGetTypeID((CFTypeRef)all) : 0),
	      (unsigned long)CTFontCollectionGetTypeID());

	asked = CTFontCollectionCreateWithFontDescriptors(query, NULL);
	check(asked != NULL, "the descriptors door answered nothing");

	/* EQUALITY IS BY VALUE: the same question asked twice IS the same collection. */
	same = CTFontCollectionCreateWithFontDescriptors(query, NULL);
	check(same != NULL && CFEqual(asked, same), "the same question asked twice gave unequal collections");
	check(same != NULL && CFHash(asked) == CFHash(same), "two equal collections hash differently");

	/* AND THE TWO QUESTIONS ARE NOT ONE QUESTION, even when neither names a descriptor. */
	bare = CTFontCollectionCreateWithFontDescriptors(empty, NULL);
	check(bare != NULL, "the empty-query door answered nothing");
	check(bare != NULL && !CFEqual(all, bare),
	      "\"every face there is\" compares EQUAL to a question with no conditions");

	/* THE COPY MERGES: one more descriptor is a different question, and nothing to merge is the same one. */
	added = CTFontCollectionCreateCopyWithFontDescriptors(asked, more, NULL);
	check(added != NULL, "the copy door answered nothing");
	check(added != NULL && !CFEqual(asked, added),
	      "a copy that added a descriptor compares equal to the original");
	untouched = CTFontCollectionCreateCopyWithFontDescriptors(asked, NULL, NULL);
	check(untouched != NULL, "a copy with no descriptors answered nothing");
	check(untouched != NULL && CFEqual(asked, untouched),
	      "a copy with nothing to merge is not equal to the original");

	/* AND THE OPTIONS ARE PART OF THE VALUE, which is what "recorded rather than obeyed" costs: two collections
	 * that differ only in their options are different questions. */
	with_options = CTFontCollectionCreateWithFontDescriptors(query, options);
	with_options_again = CTFontCollectionCreateWithFontDescriptors(query, options);
	check(with_options != NULL && with_options_again != NULL, "the options-taking door answered nothing");
	check(with_options != NULL && with_options_again != NULL && CFEqual(with_options, with_options_again),
	      "two collections with the same options are not equal");
	check(with_options != NULL && !CFEqual(asked, with_options),
	      "a collection with options compares equal to one without");

	CFRelease(empty);
	CFRelease(query);
	CFRelease(more);
	CFRelease(options);
	CFRelease(value);
	CFRelease(descriptor);
	return g_fail - before;
}

/* THE ONE OPTION KEY, WHOSE VALUE IS A RECORDED INFERENCE AND NOT A MEASURED FACT -- which is exactly why it is
 * named here: a change to it should be a deliberate act, as for the descriptor's fourteen unprovable keys. AND
 * THE REASON IT CANNOT BE PROVED IS ITSELF MEASURED: at 10.6 the AppKit declares no `NSFontCollection` at all,
 * so there is no counterpart in the era to pair the name with. */
static int case_collection_option_key(void)
{
	int before = g_fail;
	char buf[96];

	check(kCTFontCollectionRemoveDuplicatesOption != NULL, "the option key is NULL");
	if (kCTFontCollectionRemoveDuplicatesOption == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID(kCTFontCollectionRemoveDuplicatesOption) == CFStringGetTypeID(),
	      "the option key is not a CFString");
	buf[0] = '\0';
	check(CFStringGetCString(kCTFontCollectionRemoveDuplicatesOption, buf, (CFIndex)sizeof buf,
				 kCFStringEncodingUTF8) &&
	      strcmp(buf, "NSFontCollectionRemoveDuplicatesOption") == 0,
	      "the option key is \"%s\", want \"NSFontCollectionRemoveDuplicatesOption\"", buf);
	return g_fail - before;
}

/* THE ERROR VOCABULARY, AND THE ASSERTION THAT MATTERS MOST HERE IS THE COLLISION. Apple's own header gives
 * `kCTFontManagerErrorInUse` AND `kCTFontManagerErrorSystemRequired` THE SAME VALUE, 202, so an in-use refusal
 * and a system-required refusal are indistinguishable to a caller. A library that "repaired" that, or that
 * renumbered the ranges, would otherwise be indistinguishable from this one until a caller's comparison went
 * wrong — which is why every code is named here rather than spot-checked.
 *
 * THE TWO STRINGS ARE INFERENCES RATHER THAN PUBLISHED VALUES, so they are held exactly as the descriptor's
 * fourteen unprovable keys are: a change to either should be a deliberate act. AND THE REASON THEY CANNOT BE
 * PROVED IS ITSELF MEASURED — the AppKit at 10.6 declares no counterpart (`NSFontManager.h` declares no
 * `extern const …StringRef` at all), so the rule that carries the other Core Text keys across does not apply. */
static int case_font_manager_errors(void)
{
	int before = g_fail;
	/* AUTOMATIC, NEITHER static NOR const: the error constants are `extern const CFStringRef` VARIABLES, so
	 * their values are not constant expressions. The same note is in this file twice already. */
	struct { CFStringRef key; const char *want; } keys[] = {
		{ kCTFontManagerErrorDomain, "CTFontManagerErrorDomain" },
		{ kCTFontManagerErrorFontURLsKey, "CTFontManagerErrorFontURLsKey" },
	};
	size_t i;

	/* APPLE'S PUBLISHED CODES, one at a time, in both ranges. */
	check(kCTFontManagerErrorFileNotFound == 101, "kCTFontManagerErrorFileNotFound is %d, want 101",
	      (int)kCTFontManagerErrorFileNotFound);
	check(kCTFontManagerErrorInsufficientPermissions == 102,
	      "kCTFontManagerErrorInsufficientPermissions is %d, want 102",
	      (int)kCTFontManagerErrorInsufficientPermissions);
	check(kCTFontManagerErrorUnrecognizedFormat == 103,
	      "kCTFontManagerErrorUnrecognizedFormat is %d, want 103",
	      (int)kCTFontManagerErrorUnrecognizedFormat);
	check(kCTFontManagerErrorInvalidFontData == 104,
	      "kCTFontManagerErrorInvalidFontData is %d, want 104", (int)kCTFontManagerErrorInvalidFontData);
	check(kCTFontManagerErrorAlreadyRegistered == 105,
	      "kCTFontManagerErrorAlreadyRegistered is %d, want 105", (int)kCTFontManagerErrorAlreadyRegistered);
	check(kCTFontManagerErrorNotRegistered == 201, "kCTFontManagerErrorNotRegistered is %d, want 201",
	      (int)kCTFontManagerErrorNotRegistered);

	/* AND THE COLLISION IS APPLE'S, SO IT IS KEPT: the two constants are asserted to still be ONE value. A
	 * library that gave them different ones would be answering a question the era's own header cannot. */
	check(kCTFontManagerErrorInUse == 202, "kCTFontManagerErrorInUse is %d, want 202",
	      (int)kCTFontManagerErrorInUse);
	check(kCTFontManagerErrorSystemRequired == kCTFontManagerErrorInUse,
	      "kCTFontManagerErrorSystemRequired is %d and kCTFontManagerErrorInUse is %d -- Apple's header gives "
	      "both 202 and this library keeps that rather than repairing it",
	      (int)kCTFontManagerErrorSystemRequired, (int)kCTFontManagerErrorInUse);

	for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
		char buf[96];

		check(keys[i].key != NULL, "error constant %zu is NULL", i);
		if (keys[i].key == NULL) {
			continue;
		}
		check(CFGetTypeID(keys[i].key) == CFStringGetTypeID(), "error constant %zu is not a CFString", i);
		buf[0] = '\0';
		check(CFStringGetCString(keys[i].key, buf, (CFIndex)sizeof buf, kCFStringEncodingUTF8) &&
		      strcmp(buf, keys[i].want) == 0,
		      "an error constant is \"%s\", want \"%s\"", buf, keys[i].want);
	}
	check(!CFEqual(keys[0].key, keys[1].key), "the error domain and the user-info key are the same string");
	return g_fail - before;
}

/* THE MANAGER'S READ-ONLY HALF, AND THE ASSERTION THAT MATTERS MOST IS THAT A REGISTRATION IS VISIBLE TO IT.
 * The three `Copy` doors are ONE walk of the engine's configured directories AND its registrations, seen three
 * ways — so a face that has been registered has to come back through ALL THREE: its PostScript name, its
 * family, and its URL. A library that kept a second list for the manager would show it in one and not the
 * others, and that is what this case is for. */
static int case_manager_listing(void)
{
	int before = g_fail;
	const char *path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf";
	CTFontRef font = test_font(12.0);	/* this REGISTERS the face, which is what makes it visible below */
	CFArrayRef names, families, urls;
	CFURLRef expected;
	CFRange whole;
	CFStringRef wanted = CFSTR("DejaVuSansMono-Bold");
	CFIndex i, count;

	check(font != NULL, "no face, so there is nothing to look for in the manager's list");

	names = CTFontManagerCopyAvailablePostScriptNames();
	families = CTFontManagerCopyAvailableFontFamilyNames();
	urls = CTFontManagerCopyAvailableFontURLs();
	check(names != NULL, "the PostScript-name door answered nothing");
	check(families != NULL, "the family-name door answered nothing");
	check(urls != NULL, "the URL door answered nothing");
	if (names == NULL || families == NULL || urls == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID(names) == CFArrayGetTypeID() && CFGetTypeID(families) == CFArrayGetTypeID() &&
	      CFGetTypeID(urls) == CFArrayGetTypeID(),
	      "one of the three listing doors answered something that is not an array");
	check(CFArrayGetCount(urls) > 0, "the URL list is EMPTY, so nothing is enumerable at all");
	check(CFArrayGetCount(families) > 0, "the family list is EMPTY, so no face's family could be read");

	/* THE REGISTERED FACE IS IN THE NAME LIST, under the name it answers for itself. */
	whole = CFRangeMake(0, CFArrayGetCount(names));
	check(CFArrayContainsValue(names, whole, wanted),
	      "the face at %s was registered and is NOT in the PostScript-name list", path);

	/* AND ITS URL IS THERE, which is the same fact seen the other way. */
	expected = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path,
							  (CFIndex)strlen(path), false);
	check(expected != NULL, "the test could not build the URL it is looking for");
	if (expected != NULL) {
		whole = CFRangeMake(0, CFArrayGetCount(urls));
		check(CFArrayContainsValue(urls, whole, expected),
		      "the registered face's URL is not in the URL list");
		CFRelease(expected);
	}

	/* AND THE NAME LIST IS UNIQUE AND ALL STRINGS, which is Apple's own word for it: a name appearing twice
	 * means the walk added one face twice, and a non-string means the projection went wrong. */
	count = CFArrayGetCount(names);
	for (i = 0; i < count; i++) {
		CFTypeRef value = CFArrayGetValueAtIndex(names, i);
		CFIndex j;

		check(CFGetTypeID(value) == CFStringGetTypeID(), "PostScript name %ld is not a string", (long)i);
		for (j = i + 1; j < count; j++) {
			if (CFEqual(value, CFArrayGetValueAtIndex(names, j))) {
				check(0, "PostScript names %ld and %ld are the same, and the list is meant to be "
					 "unique", (long)i, (long)j);
			}
		}
	}
	CFRelease(names);
	CFRelease(families);
	CFRelease(urls);
	return g_fail - before;
}

/* THE COMPARATOR, WHICH IS WHERE THIS TIER'S RECORDED DEVIATION LIVES. Two things are promised and both are
 * held: the contract of a `CFComparatorFunction` (consistent, so a sort terminates) and the deviation itself,
 * ASSERTED SO THAT IT IS A FACT RATHER THAN A COMMENT — the day the deviation is removed, this case fails and
 * says which line changed. */
static int case_manager_comparator(void)
{
	int before = g_fail;
	CFStringRef garamond = CFSTR("Garamond");
	CFStringRef adobe = CFSTR("Adobe Garamond");
	CFStringRef lower = CFSTR("arial");
	CFStringRef upper = CFSTR("ARIAL");
	CFStringRef foo2 = CFSTR("Foo2"), foo7 = CFSTR("Foo7"), foo25 = CFSTR("Foo25");
	CFComparisonResult ab, ba;

	/* A NAME IS EQUAL TO ITSELF, AND THE RELATION IS ANTISYMMETRIC. Without both, a sort may not terminate. */
	check(CTFontManagerCompareFontFamilyNames(garamond, garamond, NULL) == kCFCompareEqualTo,
	      "a family name does not compare equal to itself");
	ab = CTFontManagerCompareFontFamilyNames(garamond, adobe, NULL);
	ba = CTFontManagerCompareFontFamilyNames(adobe, garamond, NULL);
	check((ab == kCFCompareLessThan && ba == kCFCompareGreaterThan) ||
	      (ab == kCFCompareGreaterThan && ba == kCFCompareLessThan),
	      "the comparator is not antisymmetric: %ld and %ld", (long)ab, (long)ba);

	/* CASE-INSENSITIVE, WHICH IS WHAT A UI SORT WANTS: "arial" and "ARIAL" are one family name to a reader. */
	check(CTFontManagerCompareFontFamilyNames(lower, upper, NULL) == kCFCompareEqualTo,
	      "\"arial\" and \"ARIAL\" did not compare equal");

	/* AND NUMERIC-AWARE, so a family with a number in its name reads in the order a person expects. */
	check(CTFontManagerCompareFontFamilyNames(foo2, foo7, NULL) == kCFCompareLessThan,
	      "\"Foo2\" did not come before \"Foo7\"");
	check(CTFontManagerCompareFontFamilyNames(foo7, foo25, NULL) == kCFCompareLessThan,
	      "\"Foo7\" did not come before \"Foo25\"");

	/* THE DEVIATION: Apple's documented order puts the UN-PREFIXED name first, and this comparator puts
	 * "Adobe Garamond" first because 'A' < 'G'. */
	check(CTFontManagerCompareFontFamilyNames(adobe, garamond, NULL) == kCFCompareLessThan,
	      "the recorded deviation has changed: Apple's order puts \"Garamond\" before \"Adobe Garamond\" and "
	      "this comparator was recorded as putting the prefixed name first");

	/* AND SOMETHING THAT IS NOT A STRING GETS AN ANSWER RATHER THAN A CRASH, which is what a comparator may
	 * legally be handed. */
	check(CTFontManagerCompareFontFamilyNames(NULL, garamond, NULL) == kCFCompareEqualTo,
	      "a NULL family name did not answer kCFCompareEqualTo");
	return g_fail - before;
}

/* REGISTRATION, END TO END, INCLUDING THE TWO THINGS THAT ARE ONLY TRUE HERE: THE CODES ARE APPLE'S AND ARE
 * CARRIED APPLE'S WAY, and THE SCOPE THIS TIER DOES NOT HAVE IS REFUSED WITH NO ERROR SET. That second one is
 * the decision this file records, so it is asserted rather than described: a caller tells "the file was bad"
 * from "the scope cannot be honoured here" by whether the error came back set. */
static int case_manager_registration(void)
{
	int before = g_fail;
	const char *path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf";
	const char *missing_path = "/tmp/agora-text-no-such-font.ttf";
	const char *not_a_font_path = "/etc/hostname";
	CFURLRef url, missing, not_a_font;
	CFErrorRef error;
	CFArrayRef errors;
	CFMutableArrayRef pair;

	check(access(path, R_OK) == 0, "the face at %s is not readable, so this case cannot run", path);
	url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)path,
						      (CFIndex)strlen(path), false);
	missing = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)missing_path,
							  (CFIndex)strlen(missing_path), false);
	not_a_font = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
							     (const UInt8 *)not_a_font_path,
							     (CFIndex)strlen(not_a_font_path), false);
	check(url != NULL && missing != NULL && not_a_font != NULL, "the test could not build its URLs");
	if (url == NULL || missing == NULL || not_a_font == NULL) {
		return g_fail - before;
	}

	/* NOTHING IS REGISTERED TO BEGIN WITH: every case runs in its own child, so the registry is this
	 * process's and it is empty. */
	check(CTFontManagerGetScopeForURL(url) == kCTFontManagerScopeNone,
	      "a file nobody registered already reports a scope");

	/* THE ROUND TRIP, AND THE SCOPE IT REPORTS. */
	check(CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, NULL),
	      "registering a real face failed");
	check(CTFontManagerGetScopeForURL(url) == kCTFontManagerScopeProcess,
	      "a registered face does not report the process scope");

	/* AND REGISTERING IT AGAIN IS APPLE'S `AlreadyRegistered`, CARRIED THE WAY APPLE CARRIES IT: one CFError in
	 * kCTFontManagerErrorDomain whose user-info dictionary names the URL under kCTFontManagerErrorFontURLsKey. */
	error = NULL;
	check(!CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeProcess, &error),
	      "registering the same file twice SUCCEEDED");
	check(error != NULL, "the second registration failed with NO error at all");
	if (error != NULL) {
		CFTypeRef info = CFErrorCopyUserInfo(error);

		check(CFErrorGetCode(error) == kCTFontManagerErrorAlreadyRegistered,
		      "the error code is %ld, want %d", (long)CFErrorGetCode(error),
		      (int)kCTFontManagerErrorAlreadyRegistered);
		check(CFEqual(CFErrorGetDomain(error), kCTFontManagerErrorDomain),
		      "the error is not in the font manager's domain");
		check(info != NULL &&
		      CFDictionaryGetValue((CFDictionaryRef)info, kCTFontManagerErrorFontURLsKey) != NULL,
		      "the error does not carry the URLs that failed");
		if (info != NULL) {
			CFRelease(info);
		}
		CFRelease(error);
	}

	/* A MISSING FILE AND A FILE THAT IS NOT A FONT: two more of Apple's codes, reached. */
	error = NULL;
	check(!CTFontManagerRegisterFontsForURL(missing, kCTFontManagerScopeProcess, &error),
	      "a file that does not exist was REGISTERED");
	check(error != NULL && CFErrorGetCode(error) == kCTFontManagerErrorFileNotFound,
	      "a missing file did not answer %d", (int)kCTFontManagerErrorFileNotFound);
	if (error != NULL) {
		CFRelease(error);
	}
	error = NULL;
	check(!CTFontManagerRegisterFontsForURL(not_a_font, kCTFontManagerScopeProcess, &error),
	      "a file that is not a font was REGISTERED");
	check(error != NULL && CFErrorGetCode(error) == kCTFontManagerErrorUnrecognizedFormat,
	      "a file that is not a font did not answer %d", (int)kCTFontManagerErrorUnrecognizedFormat);
	if (error != NULL) {
		CFRelease(error);
	}

	/* THE SCOPE REFUSAL, WHICH IS THE DECISION THIS FILE RECORDS. A scope this tier cannot honour answers false
	 * WITH THE ERROR LEFT UNSET, because Apple's era has no code meaning "unsupported scope" and inventing one is
	 * the one thing this library will not do. */
	error = NULL;
	check(!CTFontManagerRegisterFontsForURL(url, kCTFontManagerScopeUser, &error),
	      "a scope this tier does not have was HONOURED");
	check(error == NULL, "the scope refusal SET AN ERROR, and Apple's era has no code for it");
	check(CTFontManagerGetScopeForURL(url) == kCTFontManagerScopeProcess,
	      "the refused registration changed the registry anyway");

	/* THE PLURAL DOOR: TWO BAD URLS ARE A FALSE ANSWER WITH ONE ERROR EACH, which is how Apple's plural doors
	 * report — the array names which of them failed rather than only that something did. */
	pair = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	check(pair != NULL, "the test could not make its array of URLs");
	if (pair != NULL) {
		CFArrayAppendValue(pair, not_a_font);
		CFArrayAppendValue(pair, missing);
		errors = NULL;
		check(!CTFontManagerRegisterFontsForURLs(pair, kCTFontManagerScopeProcess, &errors),
		      "an array of two bad URLs reported SUCCESS");
		check(errors != NULL && CFArrayGetCount(errors) == 2,
		      "an array of two bad URLs gave %ld errors, want 2",
		      (long)((errors != NULL) ? CFArrayGetCount(errors) : -1));
		if (errors != NULL) {
			CFRelease(errors);
		}
		CFRelease(pair);
	}

	/* UNREGISTRATION, AND THE CODE FOR ASKING TWICE. */
	check(CTFontManagerUnregisterFontsForURL(url, kCTFontManagerScopeProcess, NULL),
	      "unregistering a registered face failed");
	check(CTFontManagerGetScopeForURL(url) == kCTFontManagerScopeNone,
	      "an unregistered face still reports a scope");
	error = NULL;
	check(!CTFontManagerUnregisterFontsForURL(url, kCTFontManagerScopeProcess, &error),
	      "unregistering the same face twice SUCCEEDED");
	check(error != NULL && CFErrorGetCode(error) == kCTFontManagerErrorNotRegistered,
	      "the second unregistration did not answer %d", (int)kCTFontManagerErrorNotRegistered);
	if (error != NULL) {
		CFRelease(error);
	}

	CFRelease(url);
	CFRelease(missing);
	CFRelease(not_a_font);
	return g_fail - before;
}

/* THE AUTO-ACTIVATION SETTING IS A PREFERENCE, AND APPLE'S OWN SENTENCE SAYS WHAT TO DO WITH IT: "apply the
 * setting to the appropriate preferences location". This tier HAS such a location — Core Foundation's — so the
 * round trip is the case. IT WRITES A PREFERENCE FILE, under a pair of identifiers of its own so that no real
 * one is touched, and the case says so because a reader who finds the file deserves to know who made it. */
static int case_manager_settings(void)
{
	int before = g_fail;
	CFStringRef mine = CFSTR("com.example.agora-text-suite");
	CFStringRef other = CFSTR("com.example.agora-text-suite-never-set");

	check(CTFontManagerGetAutoActivationSetting(other) == kCTFontManagerAutoActivationDefault,
	      "a setting nobody ever wrote did not answer the default");
	CTFontManagerSetAutoActivationSetting(mine, kCTFontManagerAutoActivationEnabled);
	check(CTFontManagerGetAutoActivationSetting(mine) == kCTFontManagerAutoActivationEnabled,
	      "the setting that was just written did not come back");
	check(CTFontManagerGetAutoActivationSetting(other) == kCTFontManagerAutoActivationDefault,
	      "one domain's setting leaked into another's");
	CTFontManagerSetAutoActivationSetting(mine, kCTFontManagerAutoActivationDisabled);
	check(CTFontManagerGetAutoActivationSetting(mine) == kCTFontManagerAutoActivationDisabled,
	      "the setting was not replaced by the next one written");
	/* AND BACK TO THE DEFAULT, which is the closest this tier has to removing a preference. */
	CTFontManagerSetAutoActivationSetting(mine, kCTFontManagerAutoActivationDefault);
	check(CTFontManagerGetAutoActivationSetting(mine) == kCTFontManagerAutoActivationDefault,
	      "the setting did not go back to the default");
	return g_fail - before;
}

/* THE VOCABULARIES AND THE NUMBERING, WHICH ARE THE PART OF THIS TYPE A CALLER WRITES DOWN. Apple's header prints
 * a value beside every constant, so each is named here; AND THE WIDTHS ARE ASSERTED TOO, because a setting crosses
 * into a style as BYTES and a type that widened an alignment to four bytes would pass every comparison above and
 * still be wrong in the buffer a caller hands it. */
static int case_paragraph_style_values(void)
{
	int before = g_fail;

	check(kCTLeftTextAlignment == 0 && kCTRightTextAlignment == 1 && kCTCenterTextAlignment == 2 &&
	      kCTJustifiedTextAlignment == 3 && kCTNaturalTextAlignment == 4,
	      "the text alignments are %d, %d, %d, %d, %d; want 0 through 4",
	      (int)kCTLeftTextAlignment, (int)kCTRightTextAlignment, (int)kCTCenterTextAlignment,
	      (int)kCTJustifiedTextAlignment, (int)kCTNaturalTextAlignment);
	check(kCTLineBreakByWordWrapping == 0 && kCTLineBreakByCharWrapping == 1 &&
	      kCTLineBreakByClipping == 2 && kCTLineBreakByTruncatingHead == 3 &&
	      kCTLineBreakByTruncatingTail == 4 && kCTLineBreakByTruncatingMiddle == 5,
	      "the line break modes are not 0 through 5, in Apple's order");
	/* A WRITING DIRECTION IS SIGNED AND `Natural` IS NEGATIVE, which is the whole point of the value: it is not a
	 * direction but the ABSENCE of one, left to the bidirectional algorithm. */
	check(kCTWritingDirectionNatural == -1 && kCTWritingDirectionLeftToRight == 0 &&
	      kCTWritingDirectionRightToLeft == 1,
	      "the writing directions are %d, %d, %d; want -1, 0, 1", (int)kCTWritingDirectionNatural,
	      (int)kCTWritingDirectionLeftToRight, (int)kCTWritingDirectionRightToLeft);

	/* THE FOURTEEN SETTINGS IN APPLE'S ORDER: a caller writes these numbers into a CTParagraphStyleSetting, so a
	 * renumbering is a silently wrong buffer rather than a compile error. */
	check(kCTParagraphStyleSpecifierAlignment == 0 && kCTParagraphStyleSpecifierFirstLineHeadIndent == 1 &&
	      kCTParagraphStyleSpecifierHeadIndent == 2 && kCTParagraphStyleSpecifierTailIndent == 3 &&
	      kCTParagraphStyleSpecifierTabStops == 4 && kCTParagraphStyleSpecifierDefaultTabInterval == 5 &&
	      kCTParagraphStyleSpecifierLineBreakMode == 6 &&
	      kCTParagraphStyleSpecifierLineHeightMultiple == 7 &&
	      kCTParagraphStyleSpecifierMaximumLineHeight == 8 &&
	      kCTParagraphStyleSpecifierMinimumLineHeight == 9 && kCTParagraphStyleSpecifierLineSpacing == 10 &&
	      kCTParagraphStyleSpecifierParagraphSpacing == 11 &&
	      kCTParagraphStyleSpecifierParagraphSpacingBefore == 12 &&
	      kCTParagraphStyleSpecifierBaseWritingDirection == 13,
	      "the specifiers are not 0 through 13 in Apple's order");
	check(kCTParagraphStyleSpecifierCount == 14, "kCTParagraphStyleSpecifierCount is %d, want 14",
	      (int)kCTParagraphStyleSpecifierCount);

	/* AND THE WIDTHS, WHICH ARE HOW A VALUE IS READ AND WRITTEN. */
	check(sizeof(CTTextAlignment) == 1 && sizeof(CTLineBreakMode) == 1 && sizeof(CTWritingDirection) == 1,
	      "the value types are %zu, %zu and %zu bytes; want 1, 1 and 1", sizeof(CTTextAlignment),
	      sizeof(CTLineBreakMode), sizeof(CTWritingDirection));
	check(sizeof(CTParagraphStyleSpecifier) == 4, "a specifier is %zu bytes, want 4",
	      sizeof(CTParagraphStyleSpecifier));
	return g_fail - before;
}

/* THE OBJECT: WHAT A STYLE IS MADE OF, AND THE DOOR THAT ANSWERS WHETHER OR NOT ANYTHING WAS SET. Apple's contract
 * for that door is two sentences and both are held — an unset setting ANSWERS ITS DEFAULT, and an invalid specifier
 * ZEROES the buffer — and so is the TabStops DEFAULT, which is this tier's recorded deviation: an EMPTY ARRAY
 * where Apple's header names "12 left-aligned tabs, spaced by 28.0 points", because a tab is a CTTextTabRef and
 * CTTextTab.h is LATER in Apple's include order than this header. */
static int case_paragraph_style_object(void)
{
	int before = g_fail;
	CTParagraphStyleSetting settings[4];
	CFIndex i;
	CTTextAlignment alignment = kCTCenterTextAlignment;
	CTLineBreakMode mode = kCTLineBreakByTruncatingTail;
	CTWritingDirection direction = kCTWritingDirectionRightToLeft;
	CGFloat indent = 24.0;
	CTParagraphStyleRef style, same, other, copy;
	CFArrayRef empty_tabs, tabs, read_tabs;
	CFMutableArrayRef wide;
	CTTextAlignment got_alignment;
	CTLineBreakMode got_mode;
	CTWritingDirection got_direction;
	CGFloat got_indent;
	unsigned char zeroed;

	/* THE DEFAULT SET, WHICH APPLE SPELLS AS A NULL `settings`. */
	style = CTParagraphStyleCreate(NULL, 0);
	check(style != NULL, "the default style could not be made");
	if (style == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)style) == CTParagraphStyleGetTypeID(),
	      "CFGetTypeID(style) is %lu and CTParagraphStyleGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)style), (unsigned long)CTParagraphStyleGetTypeID());

	/* AND EVERY UNSEEN SETTING ANSWERS ITS DEFAULT, which is the accessor's reason for existing. */
	got_alignment = 0xff;
	check(CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierAlignment,
						   sizeof got_alignment, &got_alignment) &&
	      got_alignment == kCTNaturalTextAlignment,
	      "the alignment default came back as %d, want kCTNaturalTextAlignment (%d)", (int)got_alignment,
	      (int)kCTNaturalTextAlignment);
	got_mode = 0xff;
	check(CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierLineBreakMode,
						   sizeof got_mode, &got_mode) &&
	      got_mode == kCTLineBreakByWordWrapping,
	      "the line break default came back as %d, want kCTLineBreakByWordWrapping (%d)", (int)got_mode,
	      (int)kCTLineBreakByWordWrapping);
	got_direction = 0x7f;
	check(CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierBaseWritingDirection,
						   sizeof got_direction, &got_direction) &&
	      got_direction == kCTWritingDirectionNatural,
	      "the writing direction default came back as %d, want kCTWritingDirectionNatural (%d)",
	      (int)got_direction, (int)kCTWritingDirectionNatural);
	got_indent = -1.0;
	check(CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierHeadIndent,
						   sizeof got_indent, &got_indent) && got_indent == 0.0,
	      "the head indent default came back as %g, want 0", (double)got_indent);

	/* THE TabStops DEFAULT IS APPLE'S TWELVE TABS, WHICH IS WHAT THIS ROW WAS OWED FOR AS LONG AS `CTTextTab` DID
	 * NOT EXIST. Apple's header prints the default as "12 left-aligned tabs, spaced by 28.0 points", so the case
	 * asserts the COUNT, the ALIGNMENT of each one, and the LOCATIONS — the multiples of 28.0 that the second number
	 * names — rather than merely that the array is not empty. */
	empty_tabs = NULL;
	check(CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierTabStops,
						   sizeof empty_tabs, &empty_tabs) && empty_tabs != NULL,
	      "the TabStops default did not come back as an array at all");
	if (empty_tabs != NULL) {
		check(CFGetTypeID((CFTypeRef)empty_tabs) == CFArrayGetTypeID(),
		      "the TabStops default is not a CFArray");
		check(CFArrayGetCount(empty_tabs) == 12, "the TabStops default holds %ld tabs, want 12",
		      (long)CFArrayGetCount(empty_tabs));
		for (i = 0; i < CFArrayGetCount(empty_tabs); i++) {
			CTTextTabRef tab = (CTTextTabRef)CFArrayGetValueAtIndex(empty_tabs, i);

			check(tab != NULL && CFGetTypeID((CFTypeRef)tab) == CTTextTabGetTypeID(),
			      "default tab %ld is not a CTTextTab", (long)i);
			if (tab == NULL) {
				continue;
			}
			check(CTTextTabGetAlignment(tab) == kCTLeftTextAlignment,
			      "default tab %ld is aligned %d, want kCTLeftTextAlignment (%d)", (long)i,
			      (int)CTTextTabGetAlignment(tab), (int)kCTLeftTextAlignment);
			check(CTTextTabGetLocation(tab) == 28.0 * (double)(i + 1),
			      "default tab %ld is at %g, want %g", (long)i, CTTextTabGetLocation(tab),
			      28.0 * (double)(i + 1));
		}
	}

	/* SETTINGS GO IN AND COME BACK OUT, each at its own type's width. */
	settings[0].spec = kCTParagraphStyleSpecifierAlignment;
	settings[0].valueSize = sizeof alignment;
	settings[0].value = &alignment;
	settings[1].spec = kCTParagraphStyleSpecifierHeadIndent;
	settings[1].valueSize = sizeof indent;
	settings[1].value = &indent;
	settings[2].spec = kCTParagraphStyleSpecifierLineBreakMode;
	settings[2].valueSize = sizeof mode;
	settings[2].value = &mode;
	settings[3].spec = kCTParagraphStyleSpecifierBaseWritingDirection;
	settings[3].valueSize = sizeof direction;
	settings[3].value = &direction;
	other = CTParagraphStyleCreate(settings, 4);
	check(other != NULL, "a style with four settings could not be made");
	if (other != NULL) {
		got_alignment = 0;
		check(CTParagraphStyleGetValueForSpecifier(other, kCTParagraphStyleSpecifierAlignment,
							   sizeof got_alignment, &got_alignment) &&
		      got_alignment == kCTCenterTextAlignment, "the alignment did not come back");
		got_indent = 0.0;
		check(CTParagraphStyleGetValueForSpecifier(other, kCTParagraphStyleSpecifierHeadIndent,
							   sizeof got_indent, &got_indent) && got_indent == 24.0,
		      "the head indent came back as %g, want 24", (double)got_indent);
		got_mode = 0xff;
		check(CTParagraphStyleGetValueForSpecifier(other, kCTParagraphStyleSpecifierLineBreakMode,
							   sizeof got_mode, &got_mode) &&
		      got_mode == kCTLineBreakByTruncatingTail, "the line break mode did not come back");
		got_direction = 0x7f;
		check(CTParagraphStyleGetValueForSpecifier(other,
							   kCTParagraphStyleSpecifierBaseWritingDirection,
							   sizeof got_direction, &got_direction) &&
		      got_direction == kCTWritingDirectionRightToLeft,
		      "the writing direction did not come back");
		/* AND A SETTING IT WAS NOT GIVEN STILL ANSWERS ITS DEFAULT. */
		got_indent = -1.0;
		check(CTParagraphStyleGetValueForSpecifier(other, kCTParagraphStyleSpecifierTailIndent,
							   sizeof got_indent, &got_indent) &&
		      got_indent == 0.0, "a setting that was never given did not answer its default");
	}

	/* EQUALITY IS BY VALUE, AND "NEVER SET" IS NOT "SET TO THE DEFAULT". */
	same = CTParagraphStyleCreate(settings, 4);
	check(same != NULL && CFEqual(other, same), "two styles made from the same settings are not equal");
	check(same != NULL && CFHash(other) == CFHash(same), "two equal styles hash differently");
	check(!CFEqual(style, other), "the default style compares equal to one with four settings set");

	/* THE COPY IS EQUAL AND IS NOT THE SAME OBJECT. */
	copy = CTParagraphStyleCreateCopy(other);
	check(copy != NULL, "the copy door answered nothing");
	check(copy != NULL && copy != other, "the copy is the SAME POINTER as the original");
	check(copy != NULL && CFEqual(copy, other), "the copy is not equal to the original");

	/* AN UNKNOWN SPECIFIER ZEROES THE BUFFER AND ANSWERS FALSE, which is Apple's own sentence for it. */
	zeroed = 0xff;
	check(!CTParagraphStyleGetValueForSpecifier(style, (CTParagraphStyleSpecifier)999, sizeof zeroed, &zeroed),
	      "an unknown specifier answered TRUE");
	check(zeroed == 0, "an unknown specifier did not zero the buffer: 0x%02x", (unsigned)zeroed);

	/* AND A BUFFER TOO SMALL FOR THE TYPE IS NOT WRITTEN TO AT ALL, which is asserted by looking at the bytes
	 * rather than at a return value alone: a partial write of a number is the failure this guards. */
	{
		unsigned char small_buf[sizeof(CGFloat)];
		size_t k;
		int untouched = 1;

		memset(small_buf, 0xff, sizeof small_buf);
		check(!CTParagraphStyleGetValueForSpecifier(style, kCTParagraphStyleSpecifierHeadIndent, 1, small_buf),
		      "a buffer smaller than the specifier's type was accepted");
		for (k = 0; k < sizeof small_buf; k++) {
			if (small_buf[k] != 0xff) {
				untouched = 0;
			}
		}
		check(untouched, "a buffer too small for the value was written into anyway");
	}

	/* THE TabStops VALUE IS RETAINED BY THE STYLE, which is what makes the BORROWED reference the accessor hands
	 * back safe. The caller's own reference is released here on purpose: the style is the only thing holding the
	 * array when it is asked for it back. */
	wide = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	check(wide != NULL, "the test could not make its array of tab stops");
	if (wide != NULL) {
		tabs = wide;
		settings[0].spec = kCTParagraphStyleSpecifierTabStops;
		settings[0].valueSize = sizeof tabs;
		settings[0].value = &tabs;
		{
			CTParagraphStyleRef with_tabs = CTParagraphStyleCreate(settings, 1);

			check(with_tabs != NULL, "a style with a TabStops setting could not be made");
			if (with_tabs != NULL) {
				CFRelease(wide);	/* the CALLER's reference goes, which is the point of the check */
				read_tabs = NULL;
				check(CTParagraphStyleGetValueForSpecifier(with_tabs,
									   kCTParagraphStyleSpecifierTabStops,
									   sizeof read_tabs, &read_tabs) &&
				      read_tabs != NULL, "the TabStops value did not come back");
				check(read_tabs != NULL && read_tabs == tabs,
				      "the style answered a DIFFERENT array than the one it was given");
				check(read_tabs != NULL && CFArrayGetCount(read_tabs) == 0,
				      "the TabStops value came back changed");
			}
		}
	}

	/* AND A SETTING WHOSE SIZE DOES NOT MATCH ITS SPECIFIER IS SKIPPED RATHER THAN MISREAD, which is the
	 * forward-compatible behaviour Apple asks for: the style is still usable, and what could not be read answers
	 * its default. */
	{
		CTParagraphStyleSetting wrong;
		CTParagraphStyleRef odd;

		/* THE SIZE AND THE TYPE DISAGREE: an alignment is ONE byte wide and this setting claims a CGFloat's
		 * worth, which is the case Apple's "must match" describes. */
		wrong.spec = kCTParagraphStyleSpecifierAlignment;
		wrong.valueSize = sizeof indent;
		wrong.value = &indent;
		odd = CTParagraphStyleCreate(&wrong, 1);
		check(odd != NULL, "a style with a wrong-sized setting could not be made");
		if (odd != NULL) {
			got_alignment = 0xff;
			check(CTParagraphStyleGetValueForSpecifier(odd, kCTParagraphStyleSpecifierAlignment,
								   sizeof got_alignment, &got_alignment) &&
			      got_alignment == kCTNaturalTextAlignment,
			      "a setting whose size did not match its specifier was stored anyway");
		}
	}
	return g_fail - before;
}

/* THE FIRST LINE THIS LIBRARY EVER LAYS OUT, AND EVERY DOOR THAT SAYS SOMETHING ABOUT IT. The line is built from
 * a real face in one pass, so the assertions are all comparisons against the SAME facts asked another way: the
 * glyph count against the character count, the width against the advances the font reports when it is asked
 * directly, the caret offsets against each other, and the flush against arithmetic done here. A case that restated
 * the implementation's own sums would pass with the implementation wrong. */
static int case_line_object(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs, empty;
	CFAttributedStringRef astr, bare, spaced;
	CTLineRef line, second;
	CFStringRef text = CFSTR("WAVE");
	UniChar chars[4];
	CGGlyph glyphs[4];
	CGSize advances[4];
	CFIndex i;
	double sum = 0.0, width, flush;
	CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;
	CFRange range;
	CGFloat offset, secondary = -1.0;

	check(font != NULL, "no font, so there is no line to lay out");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	empty = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL && empty != NULL, "the test could not make its dictionaries");
	if (attrs == NULL || empty == NULL) {
		return g_fail - before;
	}
	/* THE KEY IS THE VALUE `CTLine.c` WRITES, WHICH IS THE BOUNDARY THAT IMPLEMENTATION RECORDS: the real constant
	 * `kCTFontAttributeName` lives in `CTStringAttributes.h`, which is LATER in this library's include order and
	 * does not exist yet. So the case names the same string, and both move together the day it lands. */
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, text, attrs);
	check(astr != NULL, "the test could not make its attributed string");
	if (astr == NULL) {
		return g_fail - before;
	}

	line = CTLineCreateWithAttributedString(astr);
	check(line != NULL, "a line could not be made from an attributed string that HAS a font");
	if (line == NULL) {
		return g_fail - before;
	}

	/* ITS IDENTITY IS THE RUNTIME'S, as it is for every object in this library. */
	check(CFGetTypeID((CFTypeRef)line) == CTLineGetTypeID(),
	      "CFGetTypeID(line) is %lu and CTLineGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)line), (unsigned long)CTLineGetTypeID());

	/* ONE GLYPH PER CHARACTER AND THE WHOLE STRING IS THE RANGE, which is the SINGLE-RUN boundary stated as a fact:
	 * every character the string has is in the line. */
	check(CTLineGetGlyphCount(line) == 4, "the line holds %ld glyphs, want 4", (long)CTLineGetGlyphCount(line));
	range = CTLineGetStringRange(line);
	check(range.location == 0 && range.length == 4, "the string range is (%ld, %ld), want (0, 4)",
	      (long)range.location, (long)range.length);

	/* AND THE WIDTH IS THE SUM OF THE ADVANCES, RE-MEASURED FROM THE FONT — the same question through another
	 * route, which is what makes this a check rather than a restatement. */
	CFStringGetCharacters(text, CFRangeMake(0, 4), chars);
	(void)CTFontGetGlyphsForCharacters(font, chars, glyphs, 4);
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, glyphs, advances, 4);
	for (i = 0; i < 4; i++) {
		sum += advances[i].width;
	}
	width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
	check(width > 0.0, "the line has no width at all");
	check(width - sum < 0.001 && sum - width < 0.001,
	      "the line is %g wide and its own glyphs' advances add up to %g", width, sum);
	/* AND THE HEIGHTS COME FROM THE FONT, which is what a framesetter places line after line with. WHAT IS
	 * ASSERTED IS THAT — the line's heights ARE the font's — and NOT A SIGN, because Apple's own header states no
	 * sign: it says only "the scaled font descent metric", scaled by the size and matrix.
	 *
	 * **AND THE SIGN QUESTION THIS COMMENT USED TO LEAVE OPEN HAS SINCE BEEN ANSWERED — BY THE ARITHMETIC, SINCE NO
	 * ERA HEADER SETTLES IT.** It used to read: "this tier's descent comes back negative ... WHETHER APPLE'S IS
	 * NEGATIVE TOO IS AN OPEN QUESTION THE ERA'S HEADERS DO NOT SETTLE — so it is recorded here as a finding for the
	 * tier that can answer it." THE ANSWER IS THAT THE DESCENT IS A MAGNITUDE AT EVERY LAYER BUT ONE: `CGFontGetDescent`
	 * and `CTFontGetDescent` answer a POSITIVE descent, because the sum that consumes them is
	 * `ascent + descent + leading`, and the only negative descent in the tree is `NSFont -descender`, which negates at
	 * the one layer whose own documentation asks for a negative value.
	 *
	 * THE GROUND IS MEASURED: a 13-point font came out 25.2 points per line — 1.94 em — because the negative descent
	 * SUBTRACTED 3.1 and `CGFontGetLeading` answered `face->height` (the LINE height, which this sum then added on top
	 * of the two it already had). The three candidate readings are 25.2, 9.5 and 15.7 em-scaled, AND ONLY THE LAST IS A
	 * LINE HEIGHT: 15.7 = 1.21 em. So the assertion below is now the one the sum exists for — the line's three parts
	 * add up to something a line could be — while the sign cases that follow still assert only "the line reports what
	 * the font reports", which is this line's whole contract and nothing more. */
	check(ascent + descent + leading > 0.9 * CTFontGetSize(font) &&
	      ascent + descent + leading < 1.6 * CTFontGetSize(font),
	      "the line's ascent + descent + leading is %g, which is not a line height for a %g pt font",
	      (double)(ascent + descent + leading), (double)CTFontGetSize(font));
	check(ascent - CTFontGetAscent(font) < 0.001 && CTFontGetAscent(font) - ascent < 0.001,
	      "the line's ascent %g is not the font's %g", (double)ascent, (double)CTFontGetAscent(font));
	check(descent - CTFontGetDescent(font) < 0.001 && CTFontGetDescent(font) - descent < 0.001,
	      "the line's descent %g is not the font's %g", (double)descent, (double)CTFontGetDescent(font));
	check(leading - CTFontGetLeading(font) < 0.001 && CTFontGetLeading(font) - leading < 0.001,
	      "the line's leading %g is not the font's %g", (double)leading, (double)CTFontGetLeading(font));

	/* THE TWO CARET DOORS ARE INVERSES. */
	offset = CTLineGetOffsetForStringIndex(line, 0, NULL);
	check(offset == 0.0, "the offset for index 0 is %g, want 0", (double)offset);
	offset = CTLineGetOffsetForStringIndex(line, 4, &secondary);
	check(offset - width < 0.001 && width - offset < 0.001,
	      "the offset one past the last character is %g, want the line's width %g", (double)offset, width);
	check(offset - secondary < 0.001 && secondary - offset < 0.001,
	      "the secondary offset %g is not the primary %g, and a single run has one caret per boundary",
	      (double)secondary, (double)offset);
	for (i = 1; i < 4; i++) {
		CGFloat previous = CTLineGetOffsetForStringIndex(line, i - 1, NULL);
		CGFloat current = CTLineGetOffsetForStringIndex(line, i, NULL);

		if (!(current > previous)) {
			check(0, "the offsets do not increase: index %ld is %g after %g", (long)i, (double)current,
			      (double)previous);
		}
	}
	/* AND HIT TESTING INVERTS THE OFFSETS. THE CLICKS ARE PLACED INSIDE A GLYPH'S ADVANCE AND NOT ON A BOUNDARY,
	 * because a click exactly halfway between two glyphs is a TIE — this implementation gives ties to the glyph on
	 * the right — and a case that asserted the other side would be asserting a coin toss rather than a contract. */
	{
		CGFloat second_start = CTLineGetOffsetForStringIndex(line, 1, NULL);
		CGFloat second_end = CTLineGetOffsetForStringIndex(line, 2, NULL);
		CGFloat inside = (CGFloat)(second_start + (second_end - second_start) / 4.0);
		CFIndex hit = CTLineGetStringIndexForPosition(line, CGPointMake(inside, 0.0));

		check(hit == 1, "a click a quarter into the second glyph answered index %ld, want 1", (long)hit);
		check(CTLineGetStringIndexForPosition(line, CGPointMake(second_end + 1.0, 0.0)) == 2,
		      "a click just inside the third glyph did not answer index 2");
		check(CTLineGetStringIndexForPosition(line, CGPointMake(-100.0, 0.0)) == 0,
		      "a click before the line did not answer the first index");
		check(CTLineGetStringIndexForPosition(line, CGPointMake(10000.0, 0.0)) == 4,
		      "a click past the line did not answer one past the last character");
	}

	/* FLUSH IS ARITHMETIC ON THE WIDTH, AND THE CLAMP IS APPLE'S OWN SENTENCE FOR THE FACTOR. */
	check(CTLineGetPenOffsetForFlush(line, 0.0, 200.0) == 0.0, "left flush is not 0");
	flush = CTLineGetPenOffsetForFlush(line, 1.0, 200.0);
	check(flush - (200.0 - width) < 0.001 && (200.0 - width) - flush < 0.001,
	      "right flush is %g, want %g", flush, 200.0 - width);
	flush = CTLineGetPenOffsetForFlush(line, 0.5, 200.0);
	check(flush - (200.0 - width) / 2.0 < 0.001 && (200.0 - width) / 2.0 - flush < 0.001,
	      "centre flush is %g, want %g", flush, (200.0 - width) / 2.0);
	check(CTLineGetPenOffsetForFlush(line, -5.0, 200.0) == 0.0,
	      "a factor below 0 is not clamped to left flush");
	flush = CTLineGetPenOffsetForFlush(line, 5.0, 200.0);
	check(flush - (200.0 - width) < 0.001 && (200.0 - width) - flush < 0.001,
	      "a factor above 1 is not clamped to right flush");

	/* EQUALITY IS BY VALUE: the same string in the same font is the same line. */
	second = CTLineCreateWithAttributedString(astr);
	check(second != NULL && CFEqual(line, second), "two lines from the same string and font are not equal");
	check(second != NULL && CFHash(line) == CFHash(second), "two equal lines hash differently");

	/* AND THE WAYS A LINE IS REFUSED: nothing at all, and a string with no font in it. */
	check(CTLineCreateWithAttributedString(NULL) == NULL, "a NULL attributed string made a line");
	bare = CFAttributedStringCreate(kCFAllocatorDefault, text, empty);
	check(bare == NULL || CTLineCreateWithAttributedString(bare) == NULL,
	      "a string with NO font attribute made a line, and a line has no default face to fall back to");

	/* AND A LINE WITH WHITESPACE AT ITS END REPORTS IT, which is the whole purpose of that door. */
	spaced = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("WAVE  "), attrs);
	check(spaced != NULL, "the test could not make the spaced attributed string");
	if (spaced != NULL) {
		CTLineRef s = CTLineCreateWithAttributedString(spaced);

		check(s != NULL, "a line ending in two spaces could not be made");
		if (s != NULL) {
			double whole = CTLineGetTypographicBounds(s, NULL, NULL, NULL);
			double tail = CTLineGetTrailingWhitespaceWidth(s);

			check(tail > 0.0, "a line ending in two spaces reports NO trailing whitespace");
			check(tail < whole, "the trailing whitespace is the whole line, which it cannot be");
		}
	}
	return g_fail - before;
}

/* HOW MANY PIXELS THE LAST DRAW MARKED, AND WHERE THE LEFTMOST MARK IS. The four bytes are the premultiplied RGBA
 * quartet, so "marked" is ANY of them being nonzero: an opaque black glyph writes alpha. */
static int line_ink_count(const unsigned char *pixels, size_t width, size_t height)
{
	size_t x, y;
	int count = 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			const unsigned char *p = pixels + (y * width + x) * 4;

			if (p[0] != 0 || p[1] != 0 || p[2] != 0 || p[3] != 0) {
				count++;
			}
		}
	}
	return count;
}

static int line_ink_left(const unsigned char *pixels, size_t width, size_t height)
{
	size_t x, y;

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			const unsigned char *p = pixels + (y * width + x) * 4;

			if (p[0] != 0 || p[1] != 0 || p[2] != 0 || p[3] != 0) {
				return (int)x;
			}
		}
	}
	return -1;
}

/* THE DOOR THAT PUTS INK ON A PAGE, AND IT IS CHECKED BY LOOKING AT PIXELS RATHER THAN BY CALLING IT. A case that
 * called CTLineDraw and asserted nothing would pass with a draw door that did nothing at all, so this draws into a
 * real bitmap and counts the marks — AND THEN DRAWS THE SAME LINE AT ANOTHER TEXT POSITION AND CHECKS THE MARKS
 * MOVED, which is the only way to tell the two readings of Core Graphics' own `ShowGlyphsAtPositions` apart: its
 * header says positions are in user space and "the CTM alone places the pen", while the advance doors' pen "starts
 * at the TEXT POSITION". Whichever this Core Graphics does, the LINE's contract is that its origin is the caller's
 * text position, and a picture says whether that is what happens. */
static int case_line_drawing(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	enum { W = 160, H = 40 };
	static unsigned char pixels[W * H * 4];
	CGColorSpaceRef space;
	CGContextRef ctx;
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr;
	CTLineRef line;
	CGRect ink;
	int at_zero, at_thirty;

	check(font != NULL, "no font, so there is nothing to draw");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL, "the test could not make its dictionary");
	if (attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("WAVE"), attrs);
	check(astr != NULL, "the test could not make its attributed string");
	if (astr == NULL) {
		return g_fail - before;
	}
	line = CTLineCreateWithAttributedString(astr);
	check(line != NULL, "the line to draw could not be made");
	if (line == NULL) {
		return g_fail - before;
	}

	space = CGColorSpaceCreateDeviceRGB();
	check(space != NULL, "the test could not make a colour space");
	if (space == NULL) {
		return g_fail - before;
	}
	/* THE ONE FORMAT THIS CORE GRAPHICS' BITMAP CONTEXT SUPPORTS, AND IT REFUSES THE OTHERS RATHER THAN
	 * REINTERPRETING THEM: 8 bits per component, 32 bits per pixel, `kCGImageAlphaPremultipliedFirst` with
	 * `kCGBitmapByteOrder32Little` — the format its own contexts use (see CGImage.h). Asking for
	 * `kCGImageAlphaPremultipliedLast` is a documented refusal, so a case that used it would be testing the
	 * refusal rather than the line. */
	ctx = CGBitmapContextCreate(pixels, W, H, 8, W * 4, space,
				    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
	check(ctx != NULL, "the test could not make a bitmap context to draw into");
	if (ctx == NULL) {
		return g_fail - before;
	}
	CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 1.0);

	/* THE IMAGE BOUNDS EXIST AND ENCLOSE SOMETHING, before anything is drawn. */
	ink = CTLineGetImageBounds(line, ctx);
	check(!CGRectIsNull(ink) && ink.size.width > 0.0 && ink.size.height > 0.0,
	      "the line's image bounds are empty: %g by %g", (double)ink.size.width, (double)ink.size.height);

	/* THE FIRST DRAW, AT THE CONTEXT'S ORIGIN. */
	memset(pixels, 0, sizeof pixels);
	CTLineDraw(line, ctx);
	check(line_ink_count(pixels, W, H) > 0, "CTLineDraw marked NO pixels at all");
	at_zero = line_ink_left(pixels, W, H);

	/* AND THE SECOND, THIRTY POINTS TO THE RIGHT — SAME LINE, SAME CONTEXT, ONE SETTING CHANGED. */
	memset(pixels, 0, sizeof pixels);
	CGContextSetTextPosition(ctx, 30.0, 0.0);
	CTLineDraw(line, ctx);
	check(line_ink_count(pixels, W, H) > 0, "the second draw marked no pixels at all");
	at_thirty = line_ink_left(pixels, W, H);
	check(at_zero >= 0 && at_thirty >= 0, "one of the two draws left nothing to measure");
	check(at_thirty > at_zero + 25 && at_thirty < at_zero + 35,
	      "moving the text position by 30 moved the ink by %d, from %d to %d", at_thirty - at_zero, at_zero,
	      at_thirty);

	/* AND THE IMAGE BOUNDS FOLLOW THE TEXT POSITION, which is Apple's own sentence for that door. */
	ink = CTLineGetImageBounds(line, ctx);
	check(!CGRectIsNull(ink) && ink.origin.x > 29.0 && ink.origin.x < 31.0,
	      "the image bounds did not follow the text position: x is %g, want about 30", (double)ink.origin.x);
	return g_fail - before;
}

/* THE RUN, WHICH IS WHAT A LINE IS MADE OF — AND THE DOOR THAT HAS BEEN OWED SINCE CTLine LANDED. Apple's own
 * include order puts CTRun.h AFTER CTLine.h, which is exactly why `CTLineGetGlyphRuns` could not be answered before
 * this type existed, and why both moved in ONE commit: the `owed` test fails if an owed door acquires a definition,
 * so a line that answered runs while the test still named it would have failed the suite.
 *
 * EVERY ASSERTION HERE IS AGAINST SOMETHING ELSE THE LIBRARY SAYS. The run's glyphs are checked against the FONT'S
 * OWN MAPPING of the same string, its positions against its own advances, its width against the LINE's, and its
 * string range against the line's — and the "Ptr" doors against the COPYING doors, which is the pair Apple
 * documents as possibly disagreeing (a run may keep its glyphs somewhere other than one contiguous block; this
 * implementation always keeps them contiguous, so the two MUST agree, and that is asserted rather than assumed). */
static int case_run_object(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr;
	CTLineRef line;
	CFArrayRef runs, runs_again;
	CTRunRef run;
	CFDictionaryRef run_attrs;
	CFStringRef text = CFSTR("WAVE");
	UniChar chars[4];
	CGGlyph expected[4];
	CGGlyph copied_glyphs[4];
	CGPoint copied_positions[4];
	CFIndex copied_indices[4];
	const CGGlyph *glyphs_ptr;
	const CGPoint *positions_ptr;
	const CGSize *advances_ptr;
	const CFIndex *indices_ptr;
	CGAffineTransform matrix;
	double width, line_width, part;
	CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;
	CFRange run_range;
	CFIndex i;

	check(font != NULL, "no font, so there is no run to ask about");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL, "the test could not make its dictionary");
	if (attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, text, attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	check(line != NULL, "the line whose run is under test could not be made");
	if (line == NULL) {
		return g_fail - before;
	}

	/* ONE RUN, BECAUSE ONE FONT — and the door is a `Get`, so the reference belongs to the line. */
	runs = CTLineGetGlyphRuns(line);
	check(runs != NULL, "the line answered no runs at all");
	if (runs == NULL) {
		return g_fail - before;
	}
	check(CFArrayGetCount(runs) == 1, "the line holds %ld runs, want 1", (long)CFArrayGetCount(runs));
	check(CFArrayGetValueAtIndex(runs, 0) != NULL, "the single run is NULL");
	/* AND THE SAME ARRAY COMES BACK TWICE, which is what a cached BORROWED reference means. */
	runs_again = CTLineGetGlyphRuns(line);
	check(runs_again == runs, "two calls answered different run arrays");

	run = (CTRunRef)CFArrayGetValueAtIndex(runs, 0);
	check(run != NULL && CFGetTypeID((CFTypeRef)run) == CTRunGetTypeID(),
	      "the run's identity is not the runtime's: %lu against %lu",
	      (unsigned long)((run != NULL) ? CFGetTypeID((CFTypeRef)run) : 0),
	      (unsigned long)CTRunGetTypeID());
	check(CTRunGetGlyphCount(run) == 4, "the run holds %ld glyphs, want 4",
	      (long)CTRunGetGlyphCount(run));

	/* ITS GLYPHS ARE THE FONT'S OWN MAPPING OF THE SAME CHARACTERS. */
	CFStringGetCharacters(text, CFRangeMake(0, 4), chars);
	(void)CTFontGetGlyphsForCharacters(font, chars, expected, 4);
	glyphs_ptr = CTRunGetGlyphsPtr(run);
	check(glyphs_ptr != NULL, "the run's glyph pointer is NULL, and this implementation always has one");
	if (glyphs_ptr != NULL) {
		for (i = 0; i < 4; i++) {
			if (glyphs_ptr[i] != expected[i]) {
				check(0, "the run's glyph %ld is %u and the font maps it to %u", (long)i,
				      (unsigned)glyphs_ptr[i], (unsigned)expected[i]);
			}
		}
	}

	/* AND THE COPYING DOOR AGREES WITH THE POINTER DOOR — Apple's own documentation allows a run to answer NULL
	 * from the pointer door and obliges the caller to fall back; this implementation's never do, so the two MUST
	 * agree, and a case that only read one of them would not know. */
	memset(copied_glyphs, 0, sizeof copied_glyphs);
	CTRunGetGlyphs(run, CFRangeMake(0, 0), copied_glyphs);
	check(glyphs_ptr != NULL && memcmp(copied_glyphs, glyphs_ptr, sizeof copied_glyphs) == 0,
	      "the copying door and the pointer door disagree about the glyphs");

	/* THE STRING INDICES MAP EACH GLYPH BACK TO ITS CHARACTER: one glyph per character here, so 0..3. */
	indices_ptr = CTRunGetStringIndicesPtr(run);
	check(indices_ptr != NULL, "the run's string-index pointer is NULL");
	if (indices_ptr != NULL) {
		for (i = 0; i < 4; i++) {
			if (indices_ptr[i] != i) {
				check(0, "the run's string index %ld is %ld, want %ld", (long)i,
				      (long)indices_ptr[i], (long)i);
			}
		}
	}
	/* AND THE RANGE RULE, WHICH IS APPLE'S OWN SENTENCE: "If the length of the range is set to 0, then the copy
	 * operation will continue from the range's start index to the end of the run." */
	memset(copied_indices, 0, sizeof copied_indices);
	CTRunGetStringIndices(run, CFRangeMake(2, 0), copied_indices);
	check(copied_indices[0] == 2 && copied_indices[1] == 3,
	      "a range starting at 2 did not continue to the end of the run: %ld, %ld", (long)copied_indices[0],
	      (long)copied_indices[1]);

	/* THE POSITIONS ARE THE RUNNING SUM OF THE ADVANCES, which is what makes them positions rather than guesses —
	 * and the last advance is what is left of the run's width. */
	positions_ptr = CTRunGetPositionsPtr(run);
	advances_ptr = CTRunGetAdvancesPtr(run);
	check(positions_ptr != NULL && advances_ptr != NULL, "a position or advance pointer is NULL");
	if (positions_ptr != NULL && advances_ptr != NULL) {
		CGFloat x = 0.0;

		for (i = 0; i < 4; i++) {
			if (positions_ptr[i].x - x > 0.001 || x - positions_ptr[i].x > 0.001) {
				check(0, "position %ld is %g and the advances add up to %g", (long)i,
				      (double)positions_ptr[i].x, (double)x);
			}
			x += advances_ptr[i].width;
		}
	}
	memset(copied_positions, 0, sizeof copied_positions);
	CTRunGetPositions(run, CFRangeMake(1, 0), copied_positions);
	check(positions_ptr != NULL && copied_positions[0].x == positions_ptr[1].x,
	      "a range starting at 1 did not begin at the second glyph");

	/* ITS WIDTH IS THE LINE'S AND ITS HEIGHTS ARE THE FONT'S, then a PART of it measures narrower than the whole —
	 * which is what a range-taking measurement door is for, and what the line's own doors cannot do. */
	width = CTRunGetTypographicBounds(run, CFRangeMake(0, 0), &ascent, &descent, &leading);
	line_width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
	check(width > 0.0, "the run has no width at all");
	check(width - line_width < 0.001 && line_width - width < 0.001,
	      "the run is %g wide and its line is %g, and a one-run line is as wide as its run", width, line_width);
	check(ascent - CTFontGetAscent(font) < 0.001 && CTFontGetAscent(font) - ascent < 0.001,
	      "the run's ascent %g is not the font's %g", (double)ascent, (double)CTFontGetAscent(font));
	part = CTRunGetTypographicBounds(run, CFRangeMake(0, 2), NULL, NULL, NULL);
	check(part > 0.0 && part < width, "the first half of the run is %g wide and the whole is %g", part, width);

	/* ITS ATTRIBUTES ARE THE STRING'S OWN, which is the half of Apple's sentence that applies when a layout engine
	 * has nothing to manufacture. */
	run_attrs = CTRunGetAttributes(run);
	check(run_attrs != NULL, "the run answered no attributes");
	if (run_attrs != NULL) {
		check(CFGetTypeID((CFTypeRef)run_attrs) == CFDictionaryGetTypeID(),
		      "the run's attributes are not a dictionary");
		check(CFDictionaryGetValue(run_attrs, CFSTR("NSFontAttributeName")) != NULL,
		      "the run's attributes do not carry the font they were laid out with");
	}

	run_range = CTRunGetStringRange(run);
	check(run_range.location == 0 && run_range.length == 4,
	      "the run's string range is (%ld, %ld) and its line's is (0, 4)", (long)run_range.location,
	      (long)run_range.length);

	/* THE TWO DOORS THIS TIER ANSWERS RATHER THAN DERIVES, ASSERTED SO THAT CHANGING THEM IS DELIBERATE:
	 * `NoStatus`, because this library does no bidirectional reordering and sets no run matrix; and the identity
	 * matrix, for the same reason. */
	check(CTRunGetStatus(run) == kCTRunStatusNoStatus, "a run of this tier reports status %u",
	      (unsigned)CTRunGetStatus(run));
	matrix = CTRunGetTextMatrix(run);
	check(matrix.a == 1.0 && matrix.b == 0.0 && matrix.c == 0.0 && matrix.d == 1.0 && matrix.tx == 0.0 &&
	      matrix.ty == 0.0, "a run of this tier does not report the identity text matrix");

	/* AND IT DRAWS, OVER A RANGE — the same pixel check the line's case makes, because a draw door that marked
	 * nothing would pass a case that only called it. */
	{
		enum { W = 160, H = 40 };
		static unsigned char pixels[W * H * 4];
		CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();

		check(space != NULL, "the test could not make a colour space");
		if (space != NULL) {
			CGContextRef ctx = CGBitmapContextCreate(pixels, W, H, 8, W * 4, space,
				kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);

			check(ctx != NULL, "the test could not make a bitmap context for the run");
			if (ctx != NULL) {
				int whole, half, left_whole, left_half;

				CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 1.0);
				memset(pixels, 0, sizeof pixels);
				CTRunDraw(run, ctx, CFRangeMake(0, 0));
				whole = line_ink_count(pixels, W, H);
				left_whole = line_ink_left(pixels, W, H);
				check(whole > 0, "CTRunDraw marked NO pixels at all");
				memset(pixels, 0, sizeof pixels);
				CTRunDraw(run, ctx, CFRangeMake(0, 2));
				half = line_ink_count(pixels, W, H);
				left_half = line_ink_left(pixels, W, H);
				check(half > 0, "drawing HALF the run marked no pixels");
				check(half < whole, "half the run marked %d pixels and all of it marked %d", half, whole);
				check(left_half == left_whole,
				      "the first half of the run starts at %d and the whole run at %d", left_half,
				      left_whole);
			}
		}
	}

	/* NULL SAFETY: EVERY DOOR ANSWERS ITS DOCUMENTED FAILURE VALUE rather than crashing, which is what Apple's
	 * "@result ... or NULL" sentences require of an implementation. */
	check(CTRunGetGlyphCount(NULL) == 0, "a NULL run reports glyphs");
	check(CTRunGetAttributes(NULL) == NULL, "a NULL run answered attributes");
	check(CTRunGetGlyphsPtr(NULL) == NULL, "a NULL run answered a glyph pointer");
	check(CTRunGetPositionsPtr(NULL) == NULL, "a NULL run answered a position pointer");
	check(CTRunGetAdvancesPtr(NULL) == NULL, "a NULL run answered an advance pointer");
	check(CTRunGetStringIndicesPtr(NULL) == NULL, "a NULL run answered a string-index pointer");
	check(CTRunGetStringRange(NULL).length == 0, "a NULL run answered a string range");
	check(CTRunGetTypographicBounds(NULL, CFRangeMake(0, 0), NULL, NULL, NULL) == 0.0,
	      "a NULL run answered a typographic width");
	check(CTRunGetStringIndicesPtr(run) != NULL, "the run's own pointer went missing after the NULL checks");
	return g_fail - before;
}

/* ONE CHARACTER'S ADVANCE IN THE FACE UNDER TEST, so that every width below is expressed in the units the breaker
 * actually uses rather than in points guessed from a font's metrics. A monospace face is chosen for the same reason:
 * one advance is then every advance, and "seven advances fit seven characters" is a fact rather than a hope. */
static CGFloat typesetter_advance(CTFontRef font)
{
	UniChar c = (UniChar)'x';
	CGGlyph g = 0;
	CGSize size = CGSizeMake(0.0, 0.0);

	(void)CTFontGetGlyphsForCharacters(font, &c, &g, 1);
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, &g, &size, 1);
	return size.width;
}

/* THE TYPESETTER'S OWN SURFACE: ITS IDENTITY, ITS TWO CREATORS, ITS TWO OPTION CONSTANTS, AND THE RANGE RULE ITS LINE
 * DOOR IS BUILT ON. The breaking itself is the next case; this one holds the things that would be true of a
 * typesetter that broke nothing at all. */
static int case_typesetter_object(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr;
	CTTypesetterRef ts, ts_same;
	CTLineRef line;
	CFRange range;

	check(font != NULL, "no font, so there is nothing to typeset");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL, "the test could not make its dictionary");
	if (attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("one two three"), attrs);
	check(astr != NULL, "the test could not make its attributed string");
	if (astr == NULL) {
		return g_fail - before;
	}

	ts = CTTypesetterCreateWithAttributedString(astr);
	check(ts != NULL, "a typesetter could not be made from an attributed string WITH a font");
	if (ts == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)ts) == CTTypesetterGetTypeID(),
	      "CFGetTypeID(typesetter) is %lu and CTTypesetterGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)ts), (unsigned long)CTTypesetterGetTypeID());

	/* THE SECOND CREATOR IS THE FIRST WITH OPTIONS, and an EMPTY options dictionary must not change the answer. */
	ts_same = CTTypesetterCreateWithAttributedStringAndOptions(astr, attrs);
	check(ts_same != NULL, "the options-taking creator answered nothing for a dictionary of options");
	check(ts_same != NULL && CFEqual(ts, ts_same), "the two creators disagree about the same string and font");

	/* AND A STRING WITH NO FONT IS NOT TYPESETTABLE, for the same reason a line with no font is not a line. */
	check(CTTypesetterCreateWithAttributedString(NULL) == NULL, "a NULL attributed string was typeset");

	/* THE TWO OPTIONS ARE STRINGS, WHICH IS ALL A CALLER CAN CHECK OF A VALUE APPLE DOES NOT PUBLISH. */
	check(kCTTypesetterOptionDisableBidiProcessing != NULL &&
	      CFGetTypeID(kCTTypesetterOptionDisableBidiProcessing) == CFStringGetTypeID(),
	      "the bidi option key is not a CFString");
	check(kCTTypesetterOptionForcedEmbeddingLevel != NULL &&
	      CFGetTypeID(kCTTypesetterOptionForcedEmbeddingLevel) == CFStringGetTypeID(),
	      "the embedding-level option key is not a CFString");
	check(!CFEqual(kCTTypesetterOptionDisableBidiProcessing, kCTTypesetterOptionForcedEmbeddingLevel),
	      "the two option keys are the same string");

	/* A RANGE IN THE MIDDLE OF THE STRING MAKES A LINE THAT REPORTS THAT RANGE — the whole reason a typesetter needs
	 * a way into a line that no public header names (see CTLine_internal.h). */
	line = CTTypesetterCreateLine(ts, CFRangeMake(4, 3));
	check(line != NULL, "a line over characters 4..6 could not be made");
	if (line != NULL) {
		range = CTLineGetStringRange(line);
		check(range.location == 4 && range.length == 3,
		      "the line's string range is (%ld, %ld), want (4, 3) — a typeset line must report the string's "
		      "coordinates, not its own", (long)range.location, (long)range.length);
		check(CTLineGetGlyphCount(line) == 3, "the line over three characters holds %ld glyphs",
		      (long)CTLineGetGlyphCount(line));
	}

	/* A ZERO LENGTH MEANS THE REST OF THE STRING, in Apple's own words for the parameter. */
	line = CTTypesetterCreateLine(ts, CFRangeMake(4, 0));
	check(line != NULL, "a zero-length range at 4 answered nothing");
	if (line != NULL) {
		range = CTLineGetStringRange(line);
		check(range.location == 4 && range.length == 9,
		      "a zero-length range at 4 became (%ld, %ld), want (4, 9) — the rest of a 13-character string",
		      (long)range.location, (long)range.length);
	}

	/* AND A RANGE OUTSIDE THE STRING FAILS, which Apple states as a requirement of the caller and this tier answers
	 * by refusing rather than by clipping to fit. */
	check(CTTypesetterCreateLine(ts, CFRangeMake(0, 100)) == NULL, "a range past the end of the string made a line");
	check(CTTypesetterCreateLine(ts, CFRangeMake(-1, 2)) == NULL, "a negative range made a line");
	check(CTTypesetterCreateLine(NULL, CFRangeMake(0, 1)) == NULL, "a NULL typesetter made a line");

	/* NULL SAFETY, THE SAME CONTRACT EVERY OTHER DOOR IN THIS LIBRARY HAS. */
	check(CTTypesetterSuggestLineBreak(NULL, 0, 100.0) == 0, "a NULL typesetter suggested a break");
	check(CTTypesetterSuggestClusterBreak(NULL, 0, 100.0) == 0, "a NULL typesetter suggested a cluster break");
	check(CTTypesetterSuggestLineBreak(ts, -1, 100.0) == 0, "a negative start index suggested a break");
	check(CTTypesetterSuggestLineBreak(ts, 13, 100.0) == 0, "a start index at the end suggested a break");
	return g_fail - before;
}

/* THE BREAKER, WHICH IS THE ONE PLACE IN THIS LIBRARY THAT MAKES A DECISION RATHER THAN MEASURING ONE. Every
 * assertion below is either Apple's printed sentence (a hard break, or the width filled) or a PROPERTY the answers
 * must have together — that the breaks tile the string, that they never return zero, and that the two breaking doors
 * differ in exactly the way Apple names. */
static int case_typesetter_breaking(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs, attrs_styled;
	CFAttributedStringRef plain, styled, hard;
	CTParagraphStyleRef style;
	CTParagraphStyleSetting setting;
	CTTypesetterRef ts, ts_styled, ts_hard;
	CTLineRef first_line;
	CGFloat advance, seven;
	CTLineBreakMode mode = kCTLineBreakByCharWrapping;
	CFIndex from_line, from_cluster, from_styled, index, step, lines;
	CFRange range;

	check(font != NULL, "no font, so there is nothing to break");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL, "the test could not make its dictionary");
	if (attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	plain = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("one two three"), attrs);
	ts = (plain != NULL) ? CTTypesetterCreateWithAttributedString(plain) : NULL;
	check(ts != NULL, "the typesetter under test could not be made");
	if (ts == NULL) {
		return g_fail - before;
	}
	advance = typesetter_advance(font);
	check(advance > 0.0, "the face's advance is not positive, so no width below could be measured");
	seven = 7.0 * advance;

	/* THE TWO BREAKING DOORS DIFFER IN EXACTLY THE WAY APPLE NAMES, and this is the whole of the difference: Apple
	 * says the cluster door "is similar to a character break, except that it will not break apart linguistic clusters.
	 * NO OTHER CONTEXTUAL ANALYSIS WILL BE DONE", so it never backs off to a word boundary and the line door does.
	 * SEVEN ADVANCES OF "one two three" IS THE WIDTH THAT SHOWS IT: seven characters fit ("one two"), the last space
	 * that fits is at index 3, and the line door ends the line after it (`"one "`, four characters) while the cluster
	 * door ends it at the width (seven). */
	from_line = CTTypesetterSuggestLineBreak(ts, 0, seven);
	from_cluster = CTTypesetterSuggestClusterBreak(ts, 0, seven);
	check(from_line == 4, "seven advances of \"one two three\" broke as a LINE after %ld characters, want 4",
	      (long)from_line);
	check(from_cluster == 7, "the same width broke as a CLUSTER after %ld characters, want 7 — the width with no "
	      "backing off", (long)from_cluster);
	check(from_line < from_cluster, "the two breaking doors answered the same thing, and Apple says they differ");

	/* A HARD BREAK WINS WHEREVER IT IS, even with the whole width to spare — Apple's sentence puts the two triggers
	 * side by side, and the break character is part of the line. */
	hard = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("AB\nCD"), attrs);
	ts_hard = (hard != NULL) ? CTTypesetterCreateWithAttributedString(hard) : NULL;
	check(ts_hard != NULL, "the typesetter for the newline string could not be made");
	if (ts_hard != NULL) {
		check(CTTypesetterSuggestLineBreak(ts_hard, 0, 1000.0) == 3,
		      "a newline after two characters did not end the line at 3: %ld",
		      (long)CTTypesetterSuggestLineBreak(ts_hard, 0, 1000.0));
		check(CTTypesetterSuggestClusterBreak(ts_hard, 0, 1000.0) == 3,
		      "the cluster door did not honour the newline either");
		check(CTTypesetterSuggestLineBreak(ts_hard, 3, 1000.0) == 2,
		      "the second line of \"AB\\nCD\" is not the two characters after the newline");
	}

	/* AND THE PARAGRAPH STYLE'S LINE BREAK MODE IS READ, which is THE ONE PLACE IN THIS LIBRARY A RECORDED SETTING
	 * CHANGES WHAT THE LIBRARY DOES rather than being carried and answered back. The same string, the same width, one
	 * attribute added: char wrapping must break at the width where word wrapping backed off to the space. */
	setting.spec = kCTParagraphStyleSpecifierLineBreakMode;
	setting.valueSize = sizeof mode;
	setting.value = &mode;
	style = CTParagraphStyleCreate(&setting, 1);
	attrs_styled = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
						 &kCFTypeDictionaryValueCallBacks);
	check(style != NULL && attrs_styled != NULL, "the test could not make its paragraph style or dictionary");
	if (style != NULL && attrs_styled != NULL) {
		CFDictionarySetValue(attrs_styled, CFSTR("NSFontAttributeName"), font);
		/* THE KEY IS THE VALUE `CTTypesetter.c` WRITES, WHICH IS ITS OWN RECORDED BOUNDARY: the real constant
		 * `kCTParagraphStyleAttributeName` lives in `CTStringAttributes.h`, later in the documented include order. */
		CFDictionarySetValue(attrs_styled, CFSTR("NSParagraphStyleAttributeName"), style);
		styled = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("one two three"), attrs_styled);
		ts_styled = (styled != NULL) ? CTTypesetterCreateWithAttributedString(styled) : NULL;
		check(ts_styled != NULL, "the typesetter with a char-wrapping style could not be made");
		if (ts_styled != NULL) {
			from_styled = CTTypesetterSuggestLineBreak(ts_styled, 0, seven);
			check(from_styled == 7, "a CHAR-WRAPPING paragraph style broke after %ld characters, want 7 — the "
			      "mode was not read", (long)from_styled);
		}
	}

	/* THE BREAKS TILE THE STRING: from the first character to the last, every step at least one character, no gap
	 * and no overlap. THIS IS THE PROPERTY A CALLER'S LAYOUT LOOP LIVES ON — a zero would spin forever and a gap
	 * would drop text — and no single assertion above can tell it. */
	index = 0;
	lines = 0;
	while (index < 13 && lines < 64) {
		step = CTTypesetterSuggestLineBreak(ts, index, seven);
		check(step >= 1, "a break at %ld took %ld characters, and a caller's loop would never advance",
		      (long)index, (long)step);
		if (step < 1) {
			break;
		}
		index += step;
		lines++;
	}
	check(index == 13, "the breaks covered %ld of 13 characters", (long)index);
	check(lines > 1, "thirteen characters at seven advances broke into %ld line(s)", (long)lines);

	/* AND A SUGGESTED BREAK REALLY MAKES A LINE, with the range the break named — the two doors agreeing rather than
	 * being asserted separately. */
	first_line = CTTypesetterCreateLine(ts, CFRangeMake(0, from_line));
	check(first_line != NULL, "a line could not be made from the suggested break");
	if (first_line != NULL) {
		range = CTLineGetStringRange(first_line);
		check(range.location == 0 && range.length == from_line,
		      "the line's string range is (%ld, %ld) and the break suggested %ld characters",
		      (long)range.location, (long)range.length, (long)from_line);
		check(CTLineGetGlyphCount(first_line) == from_line, "the line from the break holds %ld glyphs",
		      (long)CTLineGetGlyphCount(first_line));
	}
	return g_fail - before;
}

/* THE FRAME AND THE FRAMESETTER, WHICH IS THIS WHOLE LAYER DOING ITS JOB: a string, a rectangle, and the lines that
 * fit. The assertions go outwards — the frame's ranges, its path and its attributes; then its LINES, which must TILE
 * the visible range and start where the frame says they do; then a path too SHORT to hold the text, which is the door's
 * reason for existing; then the size door and the typesetter underneath; and finally that DRAWING the frame marks
 * pixels, which no accessor can show. */
static int case_frame_object(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs, frame_attrs;
	CFAttributedStringRef astr;
	CTFramesetterRef fs;
	CTTypesetterRef under;
	CGPathRef path, short_path;
	CTFrameRef frame, clipped;
	CFArrayRef lines;
	CFRange range, visible, fit, line_range, previous;
	CGPoint origins[64];
	CGSize suggested;
	CGFloat advance, lineHeight;
	CFIndex i, count, covered;
	int descending;

	check(font != NULL, "no font, so there is nothing to lay out in a frame");
	if (font == NULL) {
		return g_fail - before;
	}
	advance = typesetter_advance(font);
	check(advance > 0.0, "the face's advance is not positive");
	if (advance <= 0.0) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	frame_attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
						&kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL && frame_attrs != NULL, "the test could not make its dictionaries");
	if (attrs == NULL || frame_attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, CFSTR("NSFontAttributeName"), font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault,
					CFSTR("one two three four five six seven eight nine ten"), attrs);
	fs = (astr != NULL) ? CTFramesetterCreateWithAttributedString(astr) : NULL;
	check(fs != NULL, "a framesetter could not be made");
	if (fs == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)fs) == CTFramesetterGetTypeID(),
	      "CFGetTypeID(framesetter) is %lu and CTFramesetterGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)fs), (unsigned long)CTFramesetterGetTypeID());

	/* THE TYPESETTER UNDER IT — the door that exists so a caller can break lines itself — and it must be usable: asked
	 * for a very wide line, it answers the WHOLE string, which says it is the same text. */
	under = CTFramesetterGetTypesetter(fs);
	check(under != NULL, "the framesetter handed back no typesetter");
	if (under != NULL) {
		check(CTTypesetterSuggestLineBreak(under, 0, 1.0e7) == 48,
		      "the typesetter under the framesetter answered %ld characters for the whole string, want 48",
		      (long)CTTypesetterSuggestLineBreak(under, 0, 1.0e7));
	}

	/* A BOX TWENTY ADVANCES WIDE AND TALL ENOUGH TO HOLD EVERYTHING — the frame that must show the LOT. AND THE
	 * RECTANGLE IS BUILT THE ERA'S WAY, because this tree's own Core Graphics header records that
	 * `CGPathCreateWithRect` is macOS 10.7 and was DELIBERATELY STRUCK: `CGPathCreateMutable` plus `CGPathAddRect` is
	 * the 10.6 spelling, and a test written with the later one would not build at all. */
	path = CGPathCreateMutable();
	check(path != NULL, "the test could not make a mutable path");
	if (path != NULL) {
		CGPathAddRect((CGMutablePathRef)path, NULL, CGRectMake(0.0, 0.0, 20.0 * advance, 400.0));
	}
	check(path != NULL, "the test could not make its path");
	if (path == NULL) {
		return g_fail - before;
	}
	frame = CTFramesetterCreateFrame(fs, CFRangeMake(0, 0), path, frame_attrs);
	check(frame != NULL, "the framesetter made no frame");
	if (frame == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)frame) == CTFrameGetTypeID(),
	      "CFGetTypeID(frame) is %lu and CTFrameGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)frame), (unsigned long)CTFrameGetTypeID());

	/* ITS RANGES, ITS PATH AND ITS ATTRIBUTES. A zero-length range means the rest of the string, as it does for every
	 * range-taking door here, and with room to spare the VISIBLE range is the whole of it — the two doors agreeing
	 * because the frame really did show everything. */
	range = CTFrameGetStringRange(frame);
	visible = CTFrameGetVisibleStringRange(frame);
	check(range.location == 0 && range.length == 48, "the frame's string range is (%ld, %ld), want (0, 48)",
	      (long)range.location, (long)range.length);
	check(visible.location == 0 && visible.length == 48,
	      "the frame showed (%ld, %ld) of a string it had room for", (long)visible.location,
	      (long)visible.length);
	check(CTFrameGetPath(frame) == path, "the frame did not hand back the path it was given");
	check(CTFrameGetFrameAttributes(frame) == frame_attrs,
	      "the frame did not hand back the attributes it was given");

	/* AND ITS LINES TILE THE VISIBLE RANGE: the first starts where the range does, each one starts where the last
	 * ended, and together they cover every character the frame says it showed. THAT PROPERTY IS THE LAYOUT — a gap
	 * drops text and an overlap draws it twice, and no single assertion above can tell either. */
	lines = CTFrameGetLines(frame);
	check(lines != NULL, "the frame answered no lines at all");
	if (lines == NULL) {
		return g_fail - before;
	}
	count = CFArrayGetCount(lines);
	check(count > 1, "a 48-character string in a 20-advance box became %ld line(s)", (long)count);
	covered = 0;
	previous = CFRangeMake(0, 0);
	for (i = 0; i < count; i++) {
		CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(lines, i);

		check(line != NULL && CFGetTypeID((CFTypeRef)line) == CTLineGetTypeID(),
		      "frame line %ld is not a CTLine", (long)i);
		if (line == NULL) {
			continue;
		}
		line_range = CTLineGetStringRange(line);
		if (i == 0) {
			check(line_range.location == visible.location,
			      "the first line starts at %ld and the visible range at %ld", (long)line_range.location,
			      (long)visible.location);
		} else {
			check(line_range.location == previous.location + previous.length,
			      "line %ld starts at %ld and the one before it ended at %ld", (long)i,
			      (long)line_range.location, (long)(previous.location + previous.length));
		}
		previous = line_range;
		covered += line_range.length;
	}
	check(covered == visible.length, "the lines cover %ld characters and the frame reports %ld visible",
	      (long)covered, (long)visible.length);

	/* AND THE ORIGINS DESCEND, from the top of the box downwards, all at the box's left edge — which is what makes a
	 * frame's origin its bottom-left and a caller's drawing loop an iteration over these points. */
	memset(origins, 0, sizeof origins);
	CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), origins);
	descending = 1;
	for (i = 0; i < count; i++) {
		check(origins[i].x == 0.0, "line %ld is placed at x = %g, and the box starts at 0",
		      (long)i, (double)origins[i].x);
		check(origins[i].y > 0.0 && origins[i].y < 400.0, "line %ld is placed at y = %g, outside the box",
		      (long)i, (double)origins[i].y);
		if (i > 0 && !(origins[i].y < origins[i - 1].y)) {
			descending = 0;
		}
	}
	check(descending, "the line origins do not descend, so the lines are not stacked");
	/* AND A RANGE OF ORIGINS WRITES ONLY ITS OWN, which is the framework's zero-length rule once more. (A `CGPoint` is
	 * a struct and has no `==` in C, so the two fields are compared.) */
	CTFrameGetLineOrigins(frame, CFRangeMake(1, 1), origins);
	check(origins[0].x == origins[1].x && origins[0].y == origins[1].y,
	      "a one-line range at 1 did not write that line's origin where it was asked: (%g, %g) against (%g, %g)",
	      (double)origins[0].x, (double)origins[0].y, (double)origins[1].x, (double)origins[1].y);

	/* NOW THE SAME TEXT IN A BOX TOO SHORT FOR IT: the frame must show a PREFIX and fewer lines, which is Apple's own
	 * sentence — filling stops "when it finds that text no longer fits". */
	lineHeight = 0.0;
	{
		CGFloat ascent = 0.0, descent = 0.0, leading = 0.0;

		(void)CTLineGetTypographicBounds((CTLineRef)CFArrayGetValueAtIndex(lines, 0), &ascent, &descent,
						 &leading);
		lineHeight = ascent + descent + leading;
	}
	short_path = CGPathCreateMutable();
	check(short_path != NULL, "the test could not make a mutable short path");
	if (short_path != NULL) {
		CGPathAddRect((CGMutablePathRef)short_path, NULL,
			      CGRectMake(0.0, 0.0, 20.0 * advance, lineHeight * 2.0));
	}
	check(short_path != NULL, "the test could not make its short path");
	if (short_path != NULL) {
		clipped = CTFramesetterCreateFrame(fs, CFRangeMake(0, 0), short_path, NULL);
		check(clipped != NULL, "the short path made no frame");
		if (clipped != NULL) {
			CFRange small = CTFrameGetVisibleStringRange(clipped);

			check(small.length < visible.length,
			      "a box two lines tall showed %ld of %ld characters", (long)small.length,
			      (long)visible.length);
			check(small.length > 0, "a box two lines tall showed nothing at all");
			check(small.location == 0, "the visible range does not start where the string does");
			check(CFArrayGetCount(CTFrameGetLines(clipped)) <= 2,
			      "a box two lines tall holds %ld lines", (long)CFArrayGetCount(CTFrameGetLines(clipped)));
		}
	}

	/* THE SIZE DOOR: WITH ROOM TO SPARE IT ANSWERS THE WHOLE TEXT, and with a height for two lines it answers less and
	 * says so through the fit range. */
	fit = CFRangeMake(0, 0);
	suggested = CTFramesetterSuggestFrameSizeWithConstraints(fs, CFRangeMake(0, 0), NULL,
								 CGSizeMake(20.0 * advance, 1.0e6), &fit);
	check(fit.location == 0 && fit.length == 48,
	      "an unconstrained suggestion fits (%ld, %ld) of 48 characters", (long)fit.location, (long)fit.length);
	check(suggested.width > 0.0 && suggested.height > 0.0,
	      "the suggested size is %g by %g", (double)suggested.width, (double)suggested.height);
	fit = CFRangeMake(0, 0);
	suggested = CTFramesetterSuggestFrameSizeWithConstraints(fs, CFRangeMake(0, 0), NULL,
								 CGSizeMake(20.0 * advance, lineHeight * 2.0), &fit);
	check(fit.length < 48 && fit.length > 0,
	      "a two-line-high constraint fits %ld of 48 characters", (long)fit.length);
	check(suggested.height <= lineHeight * 2.0 + 0.01,
	      "a two-line-high constraint suggested a height of %g", (double)suggested.height);

	/* AND IT DRAWS, WHICH NO ACCESSOR CAN SHOW. */
	{
		enum { W = 260, H = 440 };
		static unsigned char pixels[W * H * 4];
		CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();

		check(space != NULL, "the test could not make a colour space");
		if (space != NULL) {
			CGContextRef ctx = CGBitmapContextCreate(pixels, W, H, 8, W * 4, space,
				kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);

			check(ctx != NULL, "the test could not make a bitmap context for the frame");
			if (ctx != NULL) {
				CGContextSetRGBFillColor(ctx, 0.0, 0.0, 0.0, 1.0);
				memset(pixels, 0, sizeof pixels);
				CTFrameDraw(frame, ctx);
				check(line_ink_count(pixels, W, H) > 0, "CTFrameDraw marked NO pixels at all");
			}
		}
	}

	/* NULL SAFETY, THE SAME CONTRACT AS EVERYWHERE ELSE IN THIS LIBRARY. */
	check(CTFrameGetStringRange(NULL).length == 0, "a NULL frame answered a string range");
	check(CTFrameGetLines(NULL) == NULL, "a NULL frame answered lines");
	check(CTFrameGetPath(NULL) == NULL, "a NULL frame answered a path");
	CTFrameGetLineOrigins(NULL, CFRangeMake(0, 0), origins);
	CTFrameDraw(NULL, NULL);
	check(CTFramesetterCreateFrame(NULL, CFRangeMake(0, 0), path, NULL) == NULL,
	      "a NULL framesetter made a frame");
	check(CTFramesetterCreateFrame(fs, CFRangeMake(0, 100), path, NULL) == NULL,
	      "a range past the end of the string made a frame");
	check(CTFramesetterGetTypesetter(NULL) == NULL, "a NULL framesetter handed back a typesetter");
	check(CTFramesetterSuggestFrameSizeWithConstraints(NULL, CFRangeMake(0, 0), NULL, CGSizeMake(0.0, 0.0),
							   NULL).width == 0.0,
	      "a NULL framesetter suggested a size");
	return g_fail - before;
}

/* THE ATTRIBUTE VOCABULARY: FOURTEEN KEYS AND TWO ENUMS, held exactly as the font descriptor's keys are. TWELVE OF THE
 * FOURTEEN ARE PROVABLE NOW — Apple's `NSAttributedString.h` declares the same name with `NS` in place of `CT`, and a
 * dictionary crosses between the frameworks — and TWO have no counterpart there, so each key is named with the value
 * it must have and the COUNT OF PROVABLE ONES is asserted, because a key that moved between those two standings is a
 * deliberate act rather than a detail.
 *
 * AND TWO OF THESE KEYS ARE WHY THIS CASE HAS A PLACE IN THE SUITE RATHER THAN BESIDE THE OTHERS: `kCTFontAttributeName`
 * and `kCTParagraphStyleAttributeName` were carried as LOCAL LITERALS by `CTLine.c` and `CTTypesetter.c` until this
 * header landed, and the suite now reads them from the header like any other caller would. */
static int case_string_attribute_keys(void)
{
	int before = g_fail;
	/* AUTOMATIC, NEITHER static NOR const: these are `extern const CFStringRef` VARIABLES, so their values are not
	 * constant expressions. The last column is the standing each key has — 1 provable, 0 inferred — and its sum is
	 * asserted below. */
	struct { CFStringRef key; const char *want; int provable; } keys[] = {
		{ kCTFontAttributeName, "NSFontAttributeName", 1 },
		{ kCTForegroundColorFromContextAttributeName, "NSForegroundColorFromContextAttributeName", 0 },
		{ kCTKernAttributeName, "NSKernAttributeName", 1 },
		{ kCTLigatureAttributeName, "NSLigatureAttributeName", 1 },
		{ kCTForegroundColorAttributeName, "NSForegroundColorAttributeName", 1 },
		{ kCTParagraphStyleAttributeName, "NSParagraphStyleAttributeName", 1 },
		{ kCTStrokeWidthAttributeName, "NSStrokeWidthAttributeName", 1 },
		{ kCTStrokeColorAttributeName, "NSStrokeColorAttributeName", 1 },
		{ kCTUnderlineStyleAttributeName, "NSUnderlineStyleAttributeName", 1 },
		{ kCTSuperscriptAttributeName, "NSSuperscriptAttributeName", 1 },
		{ kCTUnderlineColorAttributeName, "NSUnderlineColorAttributeName", 1 },
		{ kCTVerticalFormsAttributeName, "NSVerticalFormsAttributeName", 0 },
		{ kCTGlyphInfoAttributeName, "NSGlyphInfoAttributeName", 1 },
		{ kCTCharacterShapeAttributeName, "NSCharacterShapeAttributeName", 1 },
	};
	size_t i, j;
	int provable = 0;

	for (i = 0; i < sizeof keys / sizeof keys[0]; i++) {
		char buf[96];
		size_t length;

		check(keys[i].key != NULL, "attribute key %zu is NULL", i);
		if (keys[i].key == NULL) {
			continue;
		}
		check(CFGetTypeID(keys[i].key) == CFStringGetTypeID(), "attribute key %zu is not a CFString", i);
		buf[0] = '\0';
		check(CFStringGetCString(keys[i].key, buf, (CFIndex)sizeof buf, kCFStringEncodingUTF8) &&
		      strcmp(buf, keys[i].want) == 0,
		      "an attribute key is \"%s\", want \"%s\"", buf, keys[i].want);
		/* AND EVERY VALUE TAKES THE APPKIT'S FORM, which is the rule the twelve proofs are instances of: it starts
		 * with `NS` and ends with `AttributeName`. */
		length = strlen(keys[i].want);
		check(length > 15 && strncmp(keys[i].want, "NS", 2) == 0 &&
		      strcmp(keys[i].want + length - 13, "AttributeName") == 0,
		      "an attribute key's value does not take the AppKit's form: \"%s\"", keys[i].want);
		provable += keys[i].provable;
		for (j = i + 1; j < sizeof keys / sizeof keys[0]; j++) {
			if (CFEqual(keys[i].key, keys[j].key)) {
				check(0, "attribute keys %zu and %zu are the same string", i, j);
			}
		}
	}
	check(provable == 12, "%d of the fourteen attribute keys are provable, want 12", provable);

	/* THE TWO UNDERLINE VOCABULARIES, VALUES AND ALL — INCLUDING THE GAP. The styles are 0x00, 0x01, 0x02 and 0x09, so
	 * a library that COUNTED the members of that enum would put Double at 3 and be wrong from there on; the gap is
	 * asserted as a gap rather than left implied by the values above. */
	check(kCTUnderlineStyleNone == 0x00 && kCTUnderlineStyleSingle == 0x01 && kCTUnderlineStyleThick == 0x02 &&
	      kCTUnderlineStyleDouble == 0x09,
	      "the underline styles are %d, %d, %d and %d; want 0, 1, 2 and 9",
	      (int)kCTUnderlineStyleNone, (int)kCTUnderlineStyleSingle, (int)kCTUnderlineStyleThick,
	      (int)kCTUnderlineStyleDouble);
	check(kCTUnderlineStyleDouble != 3, "the underline styles were renumbered: Double is 3");
	/* AND THE PATTERNS ARE IN THE HIGH BYTE, which is what makes a style and a pattern ONE NUMBER rather than two. */
	check(kCTUnderlinePatternSolid == 0x0000 && kCTUnderlinePatternDot == 0x0100 &&
	      kCTUnderlinePatternDash == 0x0200 && kCTUnderlinePatternDashDot == 0x0300 &&
	      kCTUnderlinePatternDashDotDot == 0x0400,
	      "the underline patterns are not in the high byte");
	check((kCTUnderlineStyleSingle | kCTUnderlinePatternDash) == 0x0201,
	      "a style and a pattern do not combine: 0x%04x",
	      (unsigned)(kCTUnderlineStyleSingle | kCTUnderlinePatternDash));
	check(sizeof(CTUnderlineStyle) == 4 && sizeof(CTUnderlineStyleModifiers) == 4,
	      "the underline types are %zu and %zu bytes, want 4 and 4", sizeof(CTUnderlineStyle),
	      sizeof(CTUnderlineStyleModifiers));
	return g_fail - before;
}

/* THE TAB TYPE ITSELF: the object the paragraph style's TabStops default is made of. Its five doors are a type id, a
 * creator and three getters, and the case holds the ROUND TRIP — what goes in comes out — the OPTIONS this tier keeps
 * without reading them, and the identity that comes from the runtime. */
static int case_text_tab(void)
{
	int before = g_fail;
	CFMutableDictionaryRef options;
	CTTextTabRef tab, again, other;
	CFDictionaryRef got_options;

	options = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	check(options != NULL, "the test could not make its options dictionary");
	if (options == NULL) {
		return g_fail - before;
	}
	tab = CTTextTabCreate(kCTCenterTextAlignment, 144.0, options);
	check(tab != NULL, "a tab could not be made");
	if (tab == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)tab) == CTTextTabGetTypeID(),
	      "CFGetTypeID(tab) is %lu and CTTextTabGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)tab), (unsigned long)CTTextTabGetTypeID());
	check(CTTextTabGetAlignment(tab) == kCTCenterTextAlignment, "the tab's alignment came back as %d, want %d",
	      (int)CTTextTabGetAlignment(tab), (int)kCTCenterTextAlignment);
	check(CTTextTabGetLocation(tab) == 144.0, "the tab's location came back as %g, want 144",
	      CTTextTabGetLocation(tab));
	/* THE OPTIONS ARE KEPT AND NOT READ — Apple names no key for them anywhere in the era's headers — so the door
	 * hands back the very dictionary it was given, and the caller's release below is the retain's proof. */
	got_options = CTTextTabGetOptions(tab);
	check(got_options == options, "the tab did not hand back the options it was given");
	CFRelease(options);
	check(CTTextTabGetOptions(tab) == got_options, "the tab's options did not survive the caller's release");

	/* EQUALITY IS BY ALIGNMENT AND LOCATION, so a tab made WITHOUT options equals one made with them — which is what
	 * the comparison below says, and the reason it is worth asserting is that the options are not part of it. */
	again = CTTextTabCreate(kCTCenterTextAlignment, 144.0, NULL);
	other = CTTextTabCreate(kCTLeftTextAlignment, 144.0, NULL);
	check(again != NULL && CFEqual(tab, again),
	      "two tabs with the same alignment and location are not equal");
	check(other != NULL && !CFEqual(tab, other), "two tabs with different alignments compare equal");

	/* NULL SAFETY, AND ONE ANSWER WORTH NAMING: an invalid tab reports `kCTNaturalTextAlignment` rather than a zero,
	 * which would read as LEFT and be a wrong answer rather than a missing one. */
	check(CTTextTabGetAlignment(NULL) == kCTNaturalTextAlignment,
	      "a NULL tab answered a left alignment rather than the natural one");
	check(CTTextTabGetLocation(NULL) == 0.0, "a NULL tab answered a location");
	check(CTTextTabGetOptions(NULL) == NULL, "a NULL tab answered options");
	return g_fail - before;
}

/* THE GLYPH SUBSTITUTION, WHICH IS THE VALUE OF AN ATTRIBUTE KEY `CTStringAttributes` ALREADY NAMES. Its three
 * creators name the same glyph three ways, so the case builds one each and holds what each CAN answer: an INDEX has no
 * name until somebody asks for one and the engine resolves it, a NAME keeps what it was given, and a CID keeps the
 * collection it belongs to and has NO GLYPH AT ALL — the one boundary here, asserted AS a boundary rather than left to
 * be discovered. */
static int case_glyph_info(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CTGlyphInfoRef by_glyph, again, by_name, by_cid;
	CFStringRef got;
	CGGlyph glyph = 0;

	check(font != NULL, "no font, so there is nothing to substitute");
	if (font == NULL) {
		return g_fail - before;
	}
	{
		UniChar c = (UniChar)'A';

		check(CTFontGetGlyphsForCharacters(font, &c, &glyph, 1) && glyph != 0,
		      "the face has no glyph for 'A'");
	}

	/* BY INDEX, WHOSE NAME IS NOT STORED AND IS FOUND ANYWAY — through the engine's own name door, which is the
	 * reason that door is at the seam. AND IT IS FOUND ONCE: the second call answers the same object. */
	by_glyph = CTGlyphInfoCreateWithGlyph(glyph, font, CFSTR("A"));
	check(by_glyph != NULL, "a glyph info could not be made from a glyph");
	if (by_glyph == NULL) {
		return g_fail - before;
	}
	check(CFGetTypeID((CFTypeRef)by_glyph) == CTGlyphInfoGetTypeID(),
	      "CFGetTypeID(glyphInfo) is %lu and CTGlyphInfoGetTypeID() is %lu",
	      (unsigned long)CFGetTypeID((CFTypeRef)by_glyph), (unsigned long)CTGlyphInfoGetTypeID());
	got = CTGlyphInfoGetGlyphName(by_glyph);
	check(got != NULL, "a glyph made from an index resolved NO name, and the engine has one for it");
	check(got != NULL && CFGetTypeID((CFTypeRef)got) == CFStringGetTypeID(),
	      "the resolved glyph name is not a CFString");
	check(CTGlyphInfoGetGlyphName(by_glyph) == got,
	      "the lazily resolved name was not the same object on the second call");
	check(CTGlyphInfoGetCharacterIdentifier(by_glyph) == 0 &&
	      CTGlyphInfoGetCharacterCollection(by_glyph) == kCTIdentityMappingCharacterCollection,
	      "an index-made glyph info claims a CID");

	/* EQUALITY IS BY GLYPH AND FACE, and the case builds the second one the same way rather than by name: whether a
	 * NAME resolves to the same glyph is the face's business, and asserting it here would be testing the face. */
	again = CTGlyphInfoCreateWithGlyph(glyph, font, NULL);
	check(again != NULL && CFEqual(by_glyph, again),
	      "two glyph infos for the same glyph in the same face are not equal");

	/* AND BY NAME, WHICH KEEPS WHAT IT WAS GIVEN even when the face cannot resolve it. */
	by_name = CTGlyphInfoCreateWithGlyphName(CFSTR("A"), font, CFSTR("A"));
	check(by_name != NULL, "a glyph info could not be made from a name");
	if (by_name != NULL) {
		got = CTGlyphInfoGetGlyphName(by_name);
		check(got != NULL && CFEqual(got, CFSTR("A")),
		      "the name-made glyph info did not answer the name it was given");
	}

	/* AND BY CID, WHICH IS THE BOUNDARY: the collection comes back and the glyph does not, because a CID is a key
	 * into a collection's CMap and this tier has no CMap. */
	by_cid = CTGlyphInfoCreateWithCharacterIdentifier((CGFontIndex)1234, kCTAdobeJapan1CharacterCollection,
							  CFSTR("A"));
	check(by_cid != NULL, "a glyph info could not be made from a CID");
	if (by_cid != NULL) {
		check(CTGlyphInfoGetCharacterIdentifier(by_cid) == 1234, "the CID came back as %u, want 1234",
		      (unsigned)CTGlyphInfoGetCharacterIdentifier(by_cid));
		check(CTGlyphInfoGetCharacterCollection(by_cid) == kCTAdobeJapan1CharacterCollection,
		      "the collection did not come back");
		check(CTGlyphInfoGetGlyphName(by_cid) == NULL,
		      "a CID-made glyph info answered a glyph name, and it has no face to resolve one in");
		check(!CFEqual(by_cid, by_glyph), "a CID-made glyph info equals an index-made one");
	}

	/* THE COLLECTION VOCABULARY, WHOSE VALUES HAVE NO GAPS — the opposite shape to the underline one the attribute
	 * case holds. */
	check(kCTIdentityMappingCharacterCollection == 0 && kCTAdobeCNS1CharacterCollection == 1 &&
	      kCTAdobeGB1CharacterCollection == 2 && kCTAdobeJapan1CharacterCollection == 3 &&
	      kCTAdobeJapan2CharacterCollection == 4 && kCTAdobeKorea1CharacterCollection == 5,
	      "the character collections are not 0 through 5 in Apple's order");
	check(sizeof(CTCharacterCollection) == 2, "a character collection is %zu bytes, want 2",
	      sizeof(CTCharacterCollection));

	/* NULL SAFETY. */
	check(CTGlyphInfoCreateWithGlyphName(NULL, font, NULL) == NULL, "a NULL glyph name made a glyph info");
	check(CTGlyphInfoCreateWithGlyph(glyph, NULL, NULL) == NULL, "a NULL font made a glyph info");
	check(CTGlyphInfoGetGlyphName(NULL) == NULL, "a NULL glyph info answered a name");
	check(CTGlyphInfoGetCharacterIdentifier(NULL) == 0, "a NULL glyph info answered a CID");
	return g_fail - before;
}

/* THE UMBRELLA, WHICH IS THE LAST HEADER AND THE ONE WITH A DOOR NO OTHER HEADER COULD DECLARE. A version number is a
 * fact about the FRAMEWORK, so it belongs to the file that MEANS the framework — and the case calls it, because a door
 * declared and never called is a declaration rather than a fact. */
static int case_umbrella_version(void)
{
	int before = g_fail;
	uint32_t version = CTGetCoreTextVersion();

	/* THE SIX PUBLISHED NUMBERS, as Apple writes them — as MACROS, so they are compared here without linking. */
	check(kCTVersionNumber10_5 == 0x00020000 && kCTVersionNumber10_5_2 == 0x00020001 &&
	      kCTVersionNumber10_5_3 == 0x00020002 && kCTVersionNumber10_5_5 == 0x00020003 &&
	      kCTVersionNumber10_6 == 0x00030000 && kCTVersionNumber10_6_7 == 0x00030007,
	      "the version constants are not the values Apple publishes");
	/* AND THE DOOR ANSWERS THE 10.6 NUMBER, which is the SURFACE this library implements — every header written from
	 * the era's, every value the era's. It is NOT a claim about the platform underneath: see CoreText.c. */
	check(version == kCTVersionNumber10_6, "the framework answered version 0x%08x, want 0x%08x (10.6)",
	      (unsigned)version, (unsigned)kCTVersionNumber10_6);
	/* AND APPLE'S OWN RECOMMENDED USE OF THIS DOOR IS THAT A CALLER CAN MAKE IT, which is the shape its header prints
	 * as an example: the pointer is non-NULL and the number is at least the one they need. */
	check(&CTGetCoreTextVersion != NULL && CTGetCoreTextVersion() >= kCTVersionNumber10_5,
	      "the version door is not callable, or answers less than 10.5");
	/* AND THE UMBRELLA REACHES EVERY OBJECT THIS FRAMEWORK HAS: a type id from each of the eleven headers that declare
	 * one, asked FOR THROUGH THE UMBRELLA rather than through the header it lives in. THAT IS WHAT ONE LINE HAS TO
	 * MEAN, and it is the assertion that would fail if the include list above ever lost a header. */
	check(CTFontGetTypeID() != 0 && CTFontCollectionGetTypeID() != 0 && CTFontDescriptorGetTypeID() != 0 &&
	      CTFrameGetTypeID() != 0 && CTFramesetterGetTypeID() != 0 && CTGlyphInfoGetTypeID() != 0 &&
	      CTLineGetTypeID() != 0 && CTParagraphStyleGetTypeID() != 0 && CTRunGetTypeID() != 0 &&
	      CTTextTabGetTypeID() != 0 && CTTypesetterGetTypeID() != 0,
	      "a type id reachable through the umbrella answered zero");
	return g_fail - before;
}

/* THE TRUNCATION ENGINE, AND APPLE'S PRINTED CONTRACT IS THE WHOLE OF IT. Four sentences from its header are asserted
 * here: a line that FITS comes back whole; the token is added where the truncation took place; a NULL token "simply
 * cut[s] the line off"; and a token wider than the width answers NULL. THE THREE TYPES ARE APPLE'S OWN DESCRIPTIONS OF
 * WHICH END STAYS VISIBLE, and the case MEASURES that rather than trusting it — an END truncation keeps the line's
 * FIRST characters, a START truncation keeps its LAST, and a MIDDLE one spans both ends.
 *
 * AND ONE REFUSAL IS HELD, because it is the one thing this tier cannot do: a token drawn in ANOTHER FACE. A line here
 * holds one font and the token's glyph INDICES mean nothing in the other, so the door answers NULL rather than putting
 * garbage where an ellipsis belongs. The case reaches that refusal the cheapest way there is — a token at the same
 * face and a DIFFERENT SIZE, which is a different font by this library's own equality. */
static int case_line_truncation(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs, attrs_big;
	CFAttributedStringRef astr, token_astr, big_astr;
	CTLineRef line, token, big_token, cut;
	double line_width;
	CFRange full;

	check(font != NULL, "no font, so there is no line to truncate");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	attrs_big = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					      &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL && attrs_big != NULL, "the test could not make its dictionaries");
	if (attrs == NULL || attrs_big == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("ABCDEFGHIJ"), attrs);
	token_astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("..."), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	token = (token_astr != NULL) ? CTLineCreateWithAttributedString(token_astr) : NULL;
	check(line != NULL && token != NULL, "the line or its token could not be made");
	if (line == NULL || token == NULL) {
		return g_fail - before;
	}
	line_width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
	full = CTLineGetStringRange(line);
	check(line_width > 0.0 && full.length == 10, "the line to truncate is %g wide and %ld characters",
	      line_width, (long)full.length);

	/* A LINE THAT FITS IS RETURNED WHOLE — Apple's own condition, "will be truncated if its width is GREATER than the
	 * width passed" — so nothing is cut and the range is the same one. */
	cut = CTLineCreateTruncatedLine(line, line_width * 2.0, kCTLineTruncationEnd, token);
	check(cut != NULL, "a line that fits was not returned at all");
	if (cut != NULL) {
		check(CTLineGetGlyphCount(cut) == CTLineGetGlyphCount(line),
		      "a line that fits came back with %ld of %ld glyphs", (long)CTLineGetGlyphCount(cut),
		      (long)CTLineGetGlyphCount(line));
		check(CTLineGetStringRange(cut).location == full.location &&
		      CTLineGetStringRange(cut).length == full.length,
		      "a line that fits came back over a different range");
	}

	/* AN END TRUNCATION KEEPS THE START: fewer glyphs, a range that begins where the line's does, and a width no
	 * greater than the one it was cut for. */
	cut = CTLineCreateTruncatedLine(line, line_width / 2.0, kCTLineTruncationEnd, token);
	check(cut != NULL, "an END truncation answered nothing");
	if (cut != NULL) {
		check(CTLineGetGlyphCount(cut) < CTLineGetGlyphCount(line),
		      "an END truncation kept every glyph");
		check(CTLineGetGlyphCount(cut) > 0, "an END truncation kept NO glyphs");
		check(CTLineGetStringRange(cut).location == full.location,
		      "an END truncation starts at %ld and the line at %ld",
		      (long)CTLineGetStringRange(cut).location, (long)full.location);
		check(CTLineGetTypographicBounds(cut, NULL, NULL, NULL) <= line_width / 2.0 + 0.001,
		      "the truncated line is %g wide, wider than the %g it was cut for",
		      CTLineGetTypographicBounds(cut, NULL, NULL, NULL), line_width / 2.0);
	}

	/* A START TRUNCATION KEEPS THE END, so its range begins LATER and still ENDS where the line does. */
	cut = CTLineCreateTruncatedLine(line, line_width / 2.0, kCTLineTruncationStart, token);
	check(cut != NULL, "a START truncation answered nothing");
	if (cut != NULL) {
		CFRange r = CTLineGetStringRange(cut);

		check(r.location > full.location, "a START truncation starts at %ld and kept the front",
		      (long)r.location);
		check(r.location + r.length == full.location + full.length,
		      "a START truncation ends at %ld and the line at %ld", (long)(r.location + r.length),
		      (long)(full.location + full.length));
	}

	/* AND A MIDDLE TRUNCATION SPANS BOTH ENDS, which is why its range starts where the line's does and is SHORTER —
	 * it covers characters the line does not draw, and that is recorded rather than hidden. */
	cut = CTLineCreateTruncatedLine(line, line_width / 2.0, kCTLineTruncationMiddle, token);
	check(cut != NULL, "a MIDDLE truncation answered nothing");
	if (cut != NULL) {
		CFRange r = CTLineGetStringRange(cut);

		check(r.location == full.location, "a MIDDLE truncation does not start where the line starts");
		check(r.length < full.length, "a MIDDLE truncation covers the whole line: %ld", (long)r.length);
		check(r.length > 0, "a MIDDLE truncation covers nothing");
	}

	/* A NULL TOKEN CUTS THE LINE OFF, in Apple's own words, and there is no ellipsis to see. */
	cut = CTLineCreateTruncatedLine(line, line_width / 2.0, kCTLineTruncationEnd, NULL);
	check(cut != NULL, "a NULL token answered nothing, and Apple says the line is simply cut off");
	if (cut != NULL) {
		check(CTLineGetGlyphCount(cut) < CTLineGetGlyphCount(line),
		      "a NULL-token truncation kept every glyph");
	}

	/* AND A TOKEN TOO WIDE ANSWERS NULL — Apple's sentence, held. */
	check(CTLineCreateTruncatedLine(line, line_width / 4.0, kCTLineTruncationEnd, token) == NULL,
	      "a token wider than the width was accepted, and Apple says this function returns NULL");

	/* AND THE REFUSAL: a token in ANOTHER FONT. Same face, DIFFERENT SIZE — which this library's font equality calls a
	 * different font — and the door refuses rather than drawing its glyph indices in the line's face. */
	{
		CTFontRef big = CTFontCreateWithName(CFSTR("DejaVuSansMono-Bold"), 24.0, NULL);

		check(big != NULL, "the test could not make its larger font");
		if (big != NULL) {
			CFDictionarySetValue(attrs_big, kCTFontAttributeName, big);
			big_astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("..."), attrs_big);
			big_token = (big_astr != NULL) ? CTLineCreateWithAttributedString(big_astr) : NULL;
			check(big_token != NULL, "the larger token line could not be made");
			if (big_token != NULL) {
				/* AND THE TOKEN IS NOT CONSULTED WHEN NOTHING IS TRUNCATED: a line that
				 * FITS comes back whole EVEN WITH a token this tier would refuse, because
				 * Apple's condition is about the LINE's width and the token is only reached
				 * when there is cutting to do. THE FIRST RUN OF THIS CASE asserted the
				 * opposite — that the refusal happened anyway — which would have made a door
				 * that refuses lines it never had to touch. */
				check(CTLineCreateTruncatedLine(line, line_width * 2.0, kCTLineTruncationEnd,
								big_token) != NULL,
				      "a line that fits was refused because of a token it never used");
				check(CTLineCreateTruncatedLine(line, line_width * 0.8, kCTLineTruncationEnd,
								big_token) == NULL,
				      "a token drawn in ANOTHER FONT was accepted, and its glyph indices mean "
				      "nothing in the line's font");
			}
		}
	}

	/* NULL SAFETY. */
	check(CTLineCreateTruncatedLine(NULL, 100.0, kCTLineTruncationEnd, NULL) == NULL,
	      "a NULL line made a truncated line");
	return g_fail - before;
}

/* THE JUSTIFICATION ENGINE, WHOSE HEADER PRINTS LESS THAN THE TRUNCATION ONE'S — so its TWO UNPRINTED DECISIONS are
 * asserted as this tier's and nothing else's: the slack goes to the SPACES (widening the gaps between words), and the
 * FACTOR is the fraction of the slack taken up. Apple's three printed sentences are held too: a factor of 0 or less
 * justifies NOTHING, full justification reaches the width it was asked for, and a NARROWER width SQUISHES the line
 * rather than refusing it.
 *
 * AND THE CASE READS THE GLYPH POSITIONS THROUGH THE RUN, because they are what justification CHANGES: the glyphs are
 * the same ones and their ORIGINS have moved. */
static int case_line_justification(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr, word_astr;
	CTLineRef line, justified, half, squished, word, wide_word;
	CFArrayRef runs;
	CTRunRef run;
	const CGPoint *before_xy, *after_xy;
	double width, wide, got;

	check(font != NULL, "no font, so there is no line to justify");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL, "the test could not make its dictionary");
	if (attrs == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("one two three"), attrs);
	word_astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("word"), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	word = (word_astr != NULL) ? CTLineCreateWithAttributedString(word_astr) : NULL;
	check(line != NULL && word != NULL, "the lines under test could not be made");
	if (line == NULL || word == NULL) {
		return g_fail - before;
	}
	width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
	wide = width + 20.0;
	check(width > 0.0, "the line to justify has no width");

	/* A FACTOR OF ZERO — AND OF LESS — JUSTIFIES NOTHING, which is Apple's own sentence. */
	justified = CTLineCreateJustifiedLine(line, 0.0, wide);
	check(justified != NULL, "a factor of 0 answered nothing");
	if (justified != NULL) {
		got = CTLineGetTypographicBounds(justified, NULL, NULL, NULL);
		check(got - width < 0.001 && width - got < 0.001,
		      "a factor of 0 changed the line: %g against %g", got, width);
	}
	check(CTLineCreateJustifiedLine(line, -1.0, wide) != NULL, "a negative factor answered nothing");

	/* FULL JUSTIFICATION REACHES THE WIDTH ASKED FOR, keeping every glyph and the string range. */
	justified = CTLineCreateJustifiedLine(line, 1.0, wide);
	check(justified != NULL, "full justification answered nothing");
	if (justified != NULL) {
		got = CTLineGetTypographicBounds(justified, NULL, NULL, NULL);
		check(got > width, "full justification did not widen the line: %g against %g", got, width);
		check(got - wide < 0.001 && wide - got < 0.001,
		      "full justification reached %g, and the width asked for was %g", got, wide);
		check(CTLineGetGlyphCount(justified) == CTLineGetGlyphCount(line),
		      "justification changed the glyph count: %ld against %ld",
		      (long)CTLineGetGlyphCount(justified), (long)CTLineGetGlyphCount(line));
		check(CTLineGetStringRange(justified).location == CTLineGetStringRange(line).location &&
		      CTLineGetStringRange(justified).length == CTLineGetStringRange(line).length,
		      "justification changed the string range");

		/* AND THE GAPS MOVED, WHICH IS THE WHOLE OF IT: the FIRST glyph stays where it was and the first glyph AFTER
		 * the first space does not. The positions come from the runs, because that is where a line keeps them. */
		runs = CTLineGetGlyphRuns(line);
		run = (runs != NULL) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
		before_xy = (run != NULL) ? CTRunGetPositionsPtr(run) : NULL;
		runs = CTLineGetGlyphRuns(justified);
		run = (runs != NULL) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
		after_xy = (run != NULL) ? CTRunGetPositionsPtr(run) : NULL;
		check(before_xy != NULL && after_xy != NULL, "the positions of one of the two lines are unreadable");
		if (before_xy != NULL && after_xy != NULL) {
			check(after_xy[0].x - before_xy[0].x < 0.001 && before_xy[0].x - after_xy[0].x < 0.001,
			      "justification moved the FIRST glyph, and the first word is where it always was");
			check(after_xy[4].x > before_xy[4].x + 0.001,
			      "justification did not move the word after the first space: %g against %g",
			      (double)after_xy[4].x, (double)before_xy[4].x);
		}
	}

	/* AND HALF THE FACTOR IS HALF THE SLACK, which is this tier's LINEAR reading of "varying degrees of partial
	 * justification" — the one thing Apple prints no curve for. */
	half = CTLineCreateJustifiedLine(line, 0.5, wide);
	check(half != NULL, "partial justification answered nothing");
	if (half != NULL) {
		got = CTLineGetTypographicBounds(half, NULL, NULL, NULL);
		check(got > width && got < wide,
		      "half the slack landed OUTSIDE the line and the width: %g, against %g and %g", got, width, wide);
	}

	/* AND A NARROWER WIDTH SQUISHES IT — Apple's own word for negative justification — rather than refusing. */
	squished = CTLineCreateJustifiedLine(line, 1.0, width - 4.0);
	check(squished != NULL, "a narrower width answered nothing, and Apple says the line is squished");
	if (squished != NULL) {
		got = CTLineGetTypographicBounds(squished, NULL, NULL, NULL);
		check(got < width, "a narrower width did not narrow the line: %g against %g", got, width);
	}

	/* AND A LINE WITH NO SPACE HAS NO GAP TO WIDEN, so it comes back unchanged — the recorded case, and the one a
	 * single long word or an overflowed line hits. */
	wide_word = CTLineCreateJustifiedLine(word, 1.0, 500.0);
	check(wide_word != NULL, "a line with no space answered nothing");
	if (wide_word != NULL) {
		double word_width = CTLineGetTypographicBounds(word, NULL, NULL, NULL);

		got = CTLineGetTypographicBounds(wide_word, NULL, NULL, NULL);
		check(got - word_width < 0.001 && word_width - got < 0.001,
		      "a line with no space was stretched anyway: %g against %g", got, word_width);
	}

	/* NULL SAFETY. */
	check(CTLineCreateJustifiedLine(NULL, 1.0, 100.0) == NULL, "a NULL line was justified");
	return g_fail - before;
}

/* A VALID ORDERING FUNCTION FOR THE COLLECTION'S SORTED DOOR — by PostScript NAME, ascending — because that door's
 * whole point is that the CALLER gives the order. It is a real comparator and not a stub: an inconsistent one would be
 * undefined behaviour in the sort, which is the kind of "test" this tree refuses to write. */
static CFComparisonResult matching_by_name(CTFontDescriptorRef first, CTFontDescriptorRef second, void *context)
{
	/* THE SIGNATURE IS APPLE'S TYPEDEF EXACTLY, and not a `const void *` pair that happens to be compatible: the
	 * first draft took the generic comparator's types, which clang REJECTS as an incompatible function pointer, and
	 * the rejection is right — a callback whose parameter types differ is a different callback. */
	CFStringRef a = (CFStringRef)CTFontDescriptorCopyAttribute(first, kCTFontNameAttribute);
	CFStringRef b = (CFStringRef)CTFontDescriptorCopyAttribute(second, kCTFontNameAttribute);
	CFComparisonResult result = kCFCompareEqualTo;

	(void)context;
	if (a != NULL && b != NULL) {
		result = CFStringCompare(a, b, kCFCompareCaseInsensitive);
	}
	if (a != NULL) {
		CFRelease(a);
	}
	if (b != NULL) {
		CFRelease(b);
	}
	return result;
}

/* THE MATCHING TIER, WHICH IS WHAT FOUR DOORS WERE WAITING FOR. The descriptor's two and the collection's two are the
 * SAME QUESTION one level apart, so the case asks it both ways and holds the three things the tier decides: THE SET (a
 * query's attributes must all be present and equal; a query with none describes everything), THE ORDER by family name,
 * and THE MANDATORY SET — the descriptor door's second parameter, which FILTERS the result rather than shaping the
 * query.
 *
 * AND IT HOLDS THE BOUNDARY THE RETURNED DESCRIPTORS CARRY: three attributes this tier can state as facts about a face,
 * so a caller who DEMANDS one it does not build gets an EMPTY ARRAY rather than an approximate face. */
static int case_font_matching(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);	/* this REGISTERS the face, which is what makes it matchable */
	CFMutableDictionaryRef query_attrs;
	CFMutableSetRef mandatory;
	CFMutableArrayRef ask;
	CTFontDescriptorRef query, single;
	CFArrayRef matches, again, filtered, plain, all, every, asked, sorted;
	CTFontCollectionRef everything, from_query;
	CTFontDescriptorRef first;
	CFStringRef name;
	CFIndex i, count;
	int named = 1;

	check(font != NULL, "no font, so there is nothing to match against");
	if (font == NULL) {
		return g_fail - before;
	}
	query_attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
						&kCFTypeDictionaryValueCallBacks);
	mandatory = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
	check(query_attrs != NULL && mandatory != NULL, "the test could not make its query dictionary or set");
	if (query_attrs == NULL || mandatory == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(query_attrs, kCTFontNameAttribute, CFSTR("DejaVuSansMono-Bold"));
	query = CTFontDescriptorCreateWithAttributes(query_attrs);
	check(query != NULL, "the query descriptor could not be made");
	if (query == NULL) {
		return g_fail - before;
	}

	/* THE SET: the face named is registered, so the query describes at least it, and EVERY match carries the name that
	 * was asked for — which is the rule the tier states. */
	matches = CTFontDescriptorCreateMatchingFontDescriptors(query, NULL);
	check(matches != NULL, "the matching door answered nothing at all");
	if (matches == NULL) {
		return g_fail - before;
	}
	count = CFArrayGetCount(matches);
	check(count >= 1, "the query matched %ld faces, and the one it names is registered", (long)count);
	for (i = 0; i < count; i++) {
		CTFontDescriptorRef match = (CTFontDescriptorRef)CFArrayGetValueAtIndex(matches, i);

		name = (CFStringRef)CTFontDescriptorCopyAttribute(match, kCTFontNameAttribute);
		if (name == NULL || !CFEqual(name, CFSTR("DejaVuSansMono-Bold"))) {
			named = 0;
		}
		if (name != NULL) {
			CFRelease(name);
		}
	}
	check(named, "a matched descriptor does not carry the name the query asked for");

	/* THE ORDER IS STABLE — the whole reason it is a decision rather than an accident — and the SINGULAR door is the
	 * FIRST of the same set. */
	again = CTFontDescriptorCreateMatchingFontDescriptors(query, NULL);
	check(again != NULL && CFEqual(matches, again),
	      "two identical queries answered different arrays, so the order is not stable");
	single = CTFontDescriptorCreateMatchingFontDescriptor(query, NULL);
	first = (CTFontDescriptorRef)CFArrayGetValueAtIndex(matches, 0);
	check(single != NULL && CFEqual(single, first),
	      "the singular door did not answer the first descriptor of the plural one's array");

	/* THE MANDATORY SET FILTERS THE RESULT: demanding an attribute this tier does not build leaves NOTHING, and
	 * demanding one it does leaves the match. */
	CFSetAddValue(mandatory, kCTFontFeatureSettingsAttribute);
	filtered = CTFontDescriptorCreateMatchingFontDescriptors(query, mandatory);
	check(filtered != NULL && CFArrayGetCount(filtered) == 0,
	      "%ld descriptors matched a mandatory attribute this tier cannot build",
	      (long)((filtered != NULL) ? CFArrayGetCount(filtered) : -1));
	if (filtered != NULL) {
		CFRelease(filtered);
	}
	{
		CFMutableSetRef names = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);

		check(names != NULL, "the test could not make its second set");
		if (names != NULL) {
			CFSetAddValue(names, kCTFontNameAttribute);
			filtered = CTFontDescriptorCreateMatchingFontDescriptors(query, names);
			check(filtered != NULL && CFArrayGetCount(filtered) >= 1,
			      "demanding the NAME, which this tier's descriptors carry, left nothing");
			if (filtered != NULL) {
				CFRelease(filtered);
			}
			CFRelease(names);
		}
	}

	/* AND THE COLLECTION'S TWO DOORS ARE THE SAME QUESTION ONE LEVEL UP. A collection that asks for everything describes
	 * every face; one built from the query describes what the query does; and the SORTED door hands the order to the
	 * caller. */
	everything = CTFontCollectionCreateFromAvailableFonts(NULL);
	every = (everything != NULL) ? CTFontCollectionCreateMatchingFontDescriptors(everything) : NULL;
	check(every != NULL && CFArrayGetCount(every) >= count,
	      "a collection asking for everything matched fewer faces (%ld) than one query did (%ld)",
	      (long)((every != NULL) ? CFArrayGetCount(every) : -1), (long)count);
	ask = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	check(ask != NULL, "the test could not make its array of queries");
	if (ask != NULL) {
		CFArrayAppendValue(ask, query);
		from_query = CTFontCollectionCreateWithFontDescriptors(ask, NULL);
		plain = (from_query != NULL) ? CTFontCollectionCreateMatchingFontDescriptors(from_query) : NULL;
		check(plain != NULL && CFArrayGetCount(plain) == count,
		      "the collection's answer (%ld) differs from the descriptor's (%ld) for the same question",
		      (long)((plain != NULL) ? CFArrayGetCount(plain) : -1), (long)count);
		/* AND THE CALLER'S OWN ORDER IS TAKEN. */
		sorted = (from_query != NULL)
				 ? CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(from_query,
												   matching_by_name, NULL)
				 : NULL;
		check(sorted != NULL && CFArrayGetCount(sorted) == count,
		      "the sorted door answered a different set from the unsorted one");
		/* AND NO CALLBACK IS THIS TIER'S OWN ORDER RATHER THAN A REFUSAL, which is recorded. */
		asked = (from_query != NULL)
				? CTFontCollectionCreateMatchingFontDescriptorsSortedWithCallback(from_query, NULL, NULL)
				: NULL;
		check(asked != NULL && plain != NULL && CFEqual(asked, plain),
		      "a NULL callback did not answer this tier's own order");
	}

	/* NULL SAFETY. */
	check(CTFontDescriptorCreateMatchingFontDescriptors(NULL, NULL) == NULL, "a NULL descriptor matched");
	check(CTFontDescriptorCreateMatchingFontDescriptor(NULL, NULL) == NULL, "a NULL descriptor matched singularly");
	check(CTFontCollectionCreateMatchingFontDescriptors(NULL) == NULL, "a NULL collection matched");
	return g_fail - before;
}

/* THE VISIBILITY DOOR: ENABLING AND DISABLING FONTS, WHICH IS THIS LIBRARY'S ONE PIECE OF GLOBAL STATE. Three things
 * about it are checkable and all three are held here: a DISABLED face LEAVES EVERY MATCHING ANSWER, RE-ENABLING BRINGS
 * IT BACK, and the state is keyed on the descriptor's ATTRIBUTES rather than on an object — a caller may rebuild its
 * query from scratch and still re-enable the font it named, which is the only reading that makes the door usable at all.
 *
 * AND IT PUTS THE STATE BACK THE WAY IT FOUND IT, at the end of the case: the disabled set is process-global, so a case
 * that left a face disabled would be a case that changed the ones after it. */
static int case_font_visibility(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);	/* registers the face, so it is matchable before anything is disabled */
	CFMutableDictionaryRef attrs;
	CFMutableArrayRef handed;
	CTFontDescriptorRef query, rebuilt;
	CFArrayRef matches, after, back, every;
	CTFontCollectionRef everything;
	CFIndex i, enabled = -1, disabled = -1, named = 0;

	check(font != NULL, "no font, so there is nothing to disable");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	handed = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	check(attrs != NULL && handed != NULL, "the test could not make its dictionary or array");
	if (attrs == NULL || handed == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, kCTFontNameAttribute, CFSTR("DejaVuSansMono-Bold"));
	query = CTFontDescriptorCreateWithAttributes(attrs);
	CFArrayAppendValue(handed, query);
	matches = CTFontDescriptorCreateMatchingFontDescriptors(query, NULL);
	enabled = (matches != NULL) ? CFArrayGetCount(matches) : -1;
	check(enabled >= 1, "the face does not match before it is disabled (%ld), so nothing can be shown to change",
	      (long)enabled);

	/* DISABLE IT: the face leaves the answer, and the COLLECTION sees the same thing — both go through the one walk. */
	CTFontManagerEnableFontDescriptors(handed, false);
	after = CTFontDescriptorCreateMatchingFontDescriptors(query, NULL);
	disabled = (after != NULL) ? CFArrayGetCount(after) : -1;
	check(after != NULL && disabled == 0, "a DISABLED face still matched (%ld descriptors)", (long)disabled);
	everything = CTFontCollectionCreateFromAvailableFonts(NULL);
	every = (everything != NULL) ? CTFontCollectionCreateMatchingFontDescriptors(everything) : NULL;
	check(every != NULL, "the collection answered nothing at all after a face was disabled");
	if (every != NULL) {
		for (i = 0; i < CFArrayGetCount(every); i++) {
			CFStringRef name = (CFStringRef)CTFontDescriptorCopyAttribute(
				(CTFontDescriptorRef)CFArrayGetValueAtIndex(every, i), kCTFontNameAttribute);

			if (name != NULL) {
				if (CFEqual(name, CFSTR("DejaVuSansMono-Bold"))) {
					named++;
				}
				CFRelease(name);
			}
		}
		check(named == 0, "the collection still offers a disabled face (%ld times)", (long)named);
		CFRelease(every);
	}

	/* RE-ENABLE FROM A REBUILT DESCRIPTOR: the state is keyed on ATTRIBUTES, not on the object the caller passed. */
	rebuilt = CTFontDescriptorCreateWithAttributes(attrs);
	check(rebuilt != NULL, "the test could not rebuild its query");
	if (rebuilt != NULL) {
		CFArrayRef single = CFArrayCreate(kCFAllocatorDefault, (const void **)&rebuilt, 1, &kCFTypeArrayCallBacks);

		check(single != NULL, "the test could not make its one-element array");
		if (single != NULL) {
			CTFontManagerEnableFontDescriptors(single, true);
			back = CTFontDescriptorCreateMatchingFontDescriptors(query, NULL);
			check(back != NULL && CFArrayGetCount(back) == enabled,
			      "re-enabling from a REBUILT descriptor did not bring the face back (%ld, was %ld)",
			      (long)((back != NULL) ? CFArrayGetCount(back) : -1), (long)enabled);
			if (back != NULL) {
				CFRelease(back);
			}
			CFRelease(single);
		}
	}

	/* NULL AND EMPTY: a door whose argument is a list must survive a list it cannot use. */
	CTFontManagerEnableFontDescriptors(NULL, false);
	check(1, "the door survived a NULL array -- this line can only be reached");
	CTFontManagerEnableFontDescriptors(handed, true);	/* AND THE STATE IS PUT BACK as the case found it. */
	CFRelease(handed);
	CFRelease(attrs);
	return g_fail - before;
}

/* THE WIRE, WHICH IS THIS LIBRARY'S HALF OF THE FONT SERVICE — the conversation as bytes, with no socket, no port, no
 * run loop and no daemon anywhere in it. Both ends are here, so a drift between what one side writes and what the other
 * reads cannot happen without this case failing.
 *
 * AND IT HOLDS THE DISTINCTION THE FORMAT RESTS ON: BYTES THIS SERVICE CANNOT READ ARE REFUSED, WHILE A REQUEST IT
 * UNDERSTANDS THAT DESCRIBES NO INSTALLED FACE IS ANSWERED, WITH AN EMPTY ARRAY. Collapsing those two would leave a
 * client unable to tell a broken conversation from an empty one.
 *
 * AND IT HOLDS THE PROPERTY THE ENCODING RESTS ON: A MATCH'S ATTRIBUTES ARE THE MATCH — a descriptor here holds exactly
 * what it was built with, so a reply of attribute dictionaries loses nothing. */
static int case_font_service_wire(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);	/* registers the face, so the conversation has something to say */
	CFMutableDictionaryRef attrs, nobody;
	CFDataRef request, reply, empty_reply, bogus;
	CFPropertyListRef decoded;
	CFTypeRef version, matches, first;
	CFIndex count = -1, nothing = -1;

	check(font != NULL, "no font, so there is nothing for the service to answer with");
	if (font == NULL) {
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	nobody = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					   &kCFTypeDictionaryValueCallBacks);
	check(attrs != NULL && nobody != NULL, "the test could not make its dictionaries");
	if (attrs == NULL || nobody == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(attrs, kCTFontNameAttribute, CFSTR("DejaVuSansMono-Bold"));
	CFDictionarySetValue(nobody, kCTFontNameAttribute, CFSTR("NoSuchFace-At-All"));

	/* A REQUESTER ASKS AND A RESPONDER ANSWERS, THROUGH THE TWO DOORS A DAEMON AND A PROCESS WOULD USE. */
	request = CTFontServiceCopyRequestData(attrs);
	check(request != NULL, "the requester's half produced nothing");
	if (request == NULL) {
		return g_fail - before;
	}
	reply = CTFontServiceCopyReplyData(request);
	check(reply != NULL, "the responder could not answer a request it was handed");
	CFRelease(request);
	if (reply == NULL) {
		return g_fail - before;
	}
	decoded = CFPropertyListCreateWithData(kCFAllocatorDefault, reply, 0, NULL, NULL);
	check(decoded != NULL && CFGetTypeID(decoded) == CFDictionaryGetTypeID(), "the reply is not a dictionary");
	if (decoded != NULL) {
		/* THE VERSION TRAVELS IN BOTH DIRECTIONS, so a client that speaks something else can tell. */
		version = CFDictionaryGetValue((CFDictionaryRef)decoded, CFSTR("version"));
		check(version != NULL && CFGetTypeID(version) == CFStringGetTypeID()
			      && CFEqual(version, kCTFontServiceWireVersionString),
		      "the reply does not carry the version spoken here");
		matches = CFDictionaryGetValue((CFDictionaryRef)decoded, CFSTR("matches"));
		check(matches != NULL && CFGetTypeID(matches) == CFArrayGetTypeID(), "the reply carries no matches array");
		if (matches != NULL && CFGetTypeID(matches) == CFArrayGetTypeID()) {
			count = CFArrayGetCount((CFArrayRef)matches);
			check(count >= 1, "a registered face came back as %ld matches", (long)count);
			if (count >= 1) {
				first = CFArrayGetValueAtIndex((CFArrayRef)matches, 0);
				check(CFGetTypeID(first) == CFDictionaryGetTypeID(),
				      "a match is not an attribute dictionary, so the encoding is not lossless");
			}
		}
		CFRelease(decoded);
	}
	CFRelease(reply);

	/* AND A QUESTION WITH NO ANSWER IS ANSWERED: a face nobody has comes back EMPTY, not refused. */
	request = CTFontServiceCopyRequestData(nobody);
	empty_reply = (request != NULL) ? CTFontServiceCopyReplyData(request) : NULL;
	check(empty_reply != NULL, "a request describing no installed face was REFUSED, and it should be answered empty");
	if (request != NULL) {
		CFRelease(request);
	}
	if (empty_reply != NULL) {
		decoded = CFPropertyListCreateWithData(kCFAllocatorDefault, empty_reply, 0, NULL, NULL);
		if (decoded != NULL && CFGetTypeID(decoded) == CFDictionaryGetTypeID()) {
			matches = CFDictionaryGetValue((CFDictionaryRef)decoded, CFSTR("matches"));
			nothing = (matches != NULL && CFGetTypeID(matches) == CFArrayGetTypeID())
					  ? CFArrayGetCount((CFArrayRef)matches)
					  : -1;
			CFRelease(decoded);
		}
		check(nothing == 0, "a question with no answer carried %ld matches rather than none", (long)nothing);
		CFRelease(empty_reply);
	}

	/* AND WHAT CANNOT BE READ IS REFUSED AS SUCH. */
	bogus = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)"not a property list at all", 26);
	check(bogus != NULL, "the test could not make its garbage bytes");
	if (bogus != NULL) {
		check(CTFontServiceCopyReplyData(bogus) == NULL, "the service answered bytes that are not a request");
		CFRelease(bogus);
	}
	check(CTFontServiceCopyReplyData(NULL) == NULL, "the service answered a NULL request");
	check(CTFontServiceCopyRequestData(NULL) == NULL, "the requester's half answered NULL attributes");
	CFRelease(nobody);
	CFRelease(attrs);
	return g_fail - before;
}

/* THE FACE'S CHARACTER SET, WHICH IS THE INVERSE OF THE GLYPH STEP THIS LIBRARY ALREADY HAD: a code unit is in the set
 * when the face maps it, which is the same fact `CTFontGetGlyphsForCharacters` reports, asked exhaustively.
 *
 * AND THAT EQUIVALENCE IS WHAT THIS CASE HOLDS, because it is the whole contract: MEMBERSHIP AND THE MAPPING DOOR MUST
 * AGREE, code unit by code unit, over a sample chosen to include characters a monospace face has (letters, digits), one
 * it may or may not have (a CJK ideograph), and one that is not a character at all (U+FFFE). The case never assumes
 * WHICH of them the face covers -- it asserts only that two doors agree about it, which is a fact either way. */
static int case_font_character_set(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFCharacterSetRef set;
	UniChar sample[6];
	CGGlyph glyphs[6];
	bool mapped[6];
	CFIndex i, members = 0;
	uint32_t code;

	check(font != NULL, "no font, so there is no character set to copy");
	if (font == NULL) {
		return g_fail - before;
	}
	set = CTFontCopyCharacterSet(font);
	check(set != NULL, "the face's character set came back NULL");
	if (set == NULL) {
		return g_fail - before;
	}
	sample[0] = (UniChar)'A';
	sample[1] = (UniChar)'0';
	sample[2] = (UniChar)' ';
	sample[3] = (UniChar)0x00E9;	/* e-acute */
	sample[4] = (UniChar)0x4E2D;	/* a CJK ideograph */
	sample[5] = (UniChar)0xFFFE;	/* not a character */
	for (i = 0; i < 6; i++) {
		mapped[i] = CTFontGetGlyphsForCharacters(font, &sample[i], &glyphs[i], 1);
	}
	for (i = 0; i < 6; i++) {
		check(CFCharacterSetIsCharacterMember(set, sample[i]) == mapped[i],
		      "the set and the mapping door disagree about U+%04X", (unsigned)sample[i]);
	}

	/* AND IT IS NEITHER EVERYTHING NOR NOTHING, which is what the scan has to get right. The count is taken by asking
	 * the set itself, so the case measures the answer rather than trusting it. */
	for (code = 0; code <= 0xFFFF; code++) {
		if (CFCharacterSetIsCharacterMember(set, (UniChar)code)) {
			members++;
		}
	}
	check(members > 0, "the face maps nothing at all, which cannot be true of a face that draws");
	check(members < 0x10000, "the set is every code unit, so the scan discriminates nothing");

	check(CTFontCopyCharacterSet(NULL) == NULL, "a NULL font produced a character set");
	CFRelease(set);
	return g_fail - before;
}

/* THE RAW TABLE DOORS, WHICH ARE THE PAIR OF THE OWED FAMILY THAT NEEDED NO PARSING: Apple's answer for a table IS the
 * file's bytes, so there is nothing to interpret and the door is a copy.
 *
 * AND WHAT THE CASE HOLDS IS THE PART THAT IS NOT NOTHING: THE DIRECTORY AND THE TABLE DOOR MUST AGREE. Every tag the
 * face lists must be one the table door can actually produce, a tag no face carries must answer NULL rather than invent
 * a table, and the bytes that come back must be THAT TABLE'S -- `head` begins with its own version, 0x00010000 for
 * TrueType, which is a fact about the format rather than about this library. */
static int case_font_tables(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFArrayRef tables;
	CFDataRef head, absent;
	CFIndex i, count = -1;
	int listed_head = 0, readable = 1, numbered = 1;

	check(font != NULL, "no font, so there are no tables to copy");
	if (font == NULL) {
		return g_fail - before;
	}
	tables = CTFontCopyAvailableTables(font, kCTFontTableOptionNoOptions);
	check(tables != NULL, "the face's table directory came back NULL");
	if (tables == NULL) {
		return g_fail - before;
	}
	count = CFArrayGetCount(tables);
	check(count > 0, "the face lists %ld tables, and a TrueType face cannot list none", (long)count);
	for (i = 0; i < count; i++) {
		CFTypeRef element = CFArrayGetValueAtIndex(tables, i);
		int tag = 0;
		CFDataRef table;

		if (element == NULL || CFGetTypeID(element) != CFNumberGetTypeID()
		    || !CFNumberGetValue((CFNumberRef)element, kCFNumberSInt32Type, &tag)) {
			numbered = 0;
			continue;
		}
		if ((uint32_t)tag == (uint32_t)kCTFontTableHead) {
			listed_head = 1;
		}
		table = CTFontCopyTable(font, (CTFontTableTag)tag, kCTFontTableOptionNoOptions);
		if (table == NULL || CFDataGetLength(table) == 0) {
			readable = 0;
		}
		if (table != NULL) {
			CFRelease(table);
		}
	}
	check(numbered, "a tag in the directory is not a number, so a caller cannot read it");
	check(listed_head, "the directory does not list `head`");
	check(readable, "the directory lists a table the table door cannot produce");

	/* AND THE BYTES ARE THAT TABLE'S. */
	head = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableHead, kCTFontTableOptionNoOptions);
	check(head != NULL, "the `head` table could not be copied");
	if (head != NULL) {
		const UInt8 *bytes = CFDataGetBytePtr(head);

		check(CFDataGetLength(head) >= 4, "`head` came back as %ld bytes", (long)CFDataGetLength(head));
		check(bytes != NULL && bytes[0] == 0x00 && bytes[1] == 0x01 && bytes[2] == 0x00 && bytes[3] == 0x00,
		      "`head` does not begin with version 1.0, so these are not that table's bytes");
		CFRelease(head);
	}
	/* AND A TABLE NO FACE HAS IS NOT INVENTED. */
	absent = CTFontCopyTable(font, (CTFontTableTag)0x7A7A7A7Au, kCTFontTableOptionNoOptions);
	check(absent == NULL, "a tag no face carries produced a table");
	if (absent != NULL) {
		CFRelease(absent);
	}
	check(CTFontCopyTable(NULL, (CTFontTableTag)kCTFontTableHead, 0) == NULL, "a NULL font produced a table");
	check(CTFontCopyAvailableTables(NULL, 0) == NULL, "a NULL font produced a directory");
	CFRelease(tables);
	return g_fail - before;
}

/* THE `post` METRICS, WHICH ARE THE FIRST OWED DOORS TO READ A TABLE AND INTERPRET IT.
 *
 * AND THE CASE DOES THE INTERPRETING ITSELF, WHICH IS THE ONLY WAY TO HOLD A PARSER TO ANYTHING: it reads the `post`
 * table through the table door, takes the two values at their fixed offsets BOTH BIG-ENDIAN, scales them by the point
 * size itself, and requires the two metric doors to agree. A parser checked against its own output checks nothing. */
static int case_font_post_metrics(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CFDataRef post;
	const UInt8 *bytes;
	CGFloat size, scale, position, thickness;
	int raw_position, raw_thickness;
	CFIndex length;

	check(font != NULL, "no font, so there are no underline metrics");
	if (font == NULL) {
		return g_fail - before;
	}
	post = CTFontCopyTable(font, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	check(post != NULL, "the `post` table could not be copied");
	if (post == NULL) {
		return g_fail - before;
	}
	length = CFDataGetLength(post);
	bytes = (const UInt8 *)CFDataGetBytePtr(post);
	check(bytes != NULL && length >= 12,
	      "`post` came back as %ld bytes, and its first four fields need twelve", (long)length);
	if (bytes == NULL || length < 12) {
		CFRelease(post);
		return g_fail - before;
	}
	/* THE TWO FIELDS AS THE FORMAT WRITES THEM: signed, and big-endian. */
	raw_position = (int)(((unsigned)bytes[8] << 8) | (unsigned)bytes[9]);
	raw_position = (raw_position >= 0x8000) ? raw_position - 0x10000 : raw_position;
	raw_thickness = (int)(((unsigned)bytes[10] << 8) | (unsigned)bytes[11]);
	raw_thickness = (raw_thickness >= 0x8000) ? raw_thickness - 0x10000 : raw_thickness;
	CFRelease(post);

	check(CTFontGetUnitsPerEm(font) > 0, "the face reports %u units per em", CTFontGetUnitsPerEm(font));
	check(CTFontGetUnitsPerEm(font) > 0, "no units per em, so nothing can be scaled");
	if (CTFontGetUnitsPerEm(font) == 0) {
		return g_fail - before;
	}
	size = CTFontGetSize(font);
	scale = size / (CGFloat)CTFontGetUnitsPerEm(font);
	position = CTFontGetUnderlinePosition(font);
	thickness = CTFontGetUnderlineThickness(font);

	/* THE DOORS AGREE WITH THE TABLE, to the precision a scaled metric can be compared at. */
	check(fabs((double)position - (double)((CGFloat)raw_position * scale)) < 0.0001,
	      "the position door answered %g where the table's %d scaled to %g points is %g",
	      (double)position, raw_position, (double)size, (double)((CGFloat)raw_position * scale));
	check(fabs((double)thickness - (double)((CGFloat)raw_thickness * scale)) < 0.0001,
	      "the thickness door answered %g where the table's %d scaled to %g points is %g",
	      (double)thickness, raw_thickness, (double)size, (double)((CGFloat)raw_thickness * scale));

	/* AND THEY ARE SCALED AND NOT RAW: a font unit is a fraction of a point at any real size, so a metric in the TENS
	 * would mean the scale was never applied -- which is the failure this line exists to catch. */
	check(fabs((double)position) < (double)size && fabs((double)thickness) < (double)size,
	      "the metrics are not scaled to the point size (%g and %g at %g points)",
	      (double)position, (double)thickness, (double)size);
	check(thickness > 0.0, "the face states an underline thickness of %g, and a face that draws one states a positive size",
	      (double)thickness);

	check(CTFontGetUnderlinePosition(NULL) == 0.0, "a NULL font produced an underline position");
	check(CTFontGetUnderlineThickness(NULL) == 0.0, "a NULL font produced an underline thickness");
	return g_fail - before;
}

/* THE SYMBOLIC TRAITS, WHICH THIS LIBRARY USED TO REFUSE TO GUESS AT AND CAN NOW READ.
 *
 * THE CASE HOLDS THREE THINGS AND THE THIRD IS THE ONE THAT KEEPS THE OTHER TWO HONEST: the face at hand is
 * `DejaVuSansMono-Bold`, so BOLD must be set and it must be set for a MONOSPACED face too; AND THE DOOR MUST AGREE WITH
 * THE TABLES IT READS, which the case computes for itself out of the raw `OS/2` and `head` bytes; AND THE TABLES MUST
 * ACTUALLY STATE IT, because a check that passes with both sides silent proves nothing at all. */
static int case_font_symbolic_traits(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	CTFontSymbolicTraits traits;
	CFDataRef os2, head;
	unsigned fs_selection = 0, mac_style = 0;
	int states_bold;

	check(font != NULL, "no font, so there are no traits to read");
	if (font == NULL) {
		return g_fail - before;
	}
	traits = CTFontGetSymbolicTraits(font);
	check((traits & kCTFontBoldTrait) != 0,
	      "the face this suite loads is named `-Bold` and does not report bold (mask 0x%x)", (unsigned)traits);
	check((traits & kCTFontMonoSpaceTrait) != 0,
	      "a fixed-pitch face does not report monospace (mask 0x%x)", (unsigned)traits);

	/* THE DOOR AGAINST THE TABLES, computed here from the bytes the table door hands back. */
	os2 = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableOS2, kCTFontTableOptionNoOptions);
	if (os2 != NULL) {
		const UInt8 *bytes = (const UInt8 *)CFDataGetBytePtr(os2);

		if (bytes != NULL && CFDataGetLength(os2) >= 64) {
			fs_selection = ((unsigned)bytes[62] << 8) | (unsigned)bytes[63];
		}
		CFRelease(os2);
	}
	head = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableHead, kCTFontTableOptionNoOptions);
	if (head != NULL) {
		const UInt8 *bytes = (const UInt8 *)CFDataGetBytePtr(head);

		if (bytes != NULL && CFDataGetLength(head) >= 46) {
			mac_style = ((unsigned)bytes[44] << 8) | (unsigned)bytes[45];
		}
		CFRelease(head);
	}
	states_bold = ((fs_selection & 0x20u) != 0) || ((mac_style & 0x0001u) != 0);
	check(states_bold, "neither `OS/2` nor `head` marks this face bold, so the agreement below would prove nothing");
	check(((traits & kCTFontBoldTrait) != 0) == states_bold,
	      "the bold trait disagrees with the tables (fsSelection 0x%x, macStyle 0x%x, mask 0x%x)",
	      fs_selection, mac_style, (unsigned)traits);

	check(CTFontGetSymbolicTraits(NULL) == 0, "a NULL font produced traits");
	return g_fail - before;
}

/* THE CURRENT VARIATION INSTANCE, WHICH IS A CHOICE RATHER THAN A CAPABILITY: it needs no `fvar` table, because it is
 * the variation dictionary the font was ASKED FOR, and the font has been carrying its descriptor all along.
 *
 * AND THE CASE HOLDS BOTH DIRECTIONS OF THAT, because either one alone is passable by accident: a font asked for NOTHING
 * must report nothing, and a font asked for an instance must hand back THE SAME DICTIONARY -- compared with CFEqual
 * against the one that went in, so a door that invented a dictionary would fail rather than merely differ. */
static int case_font_variation(void)
{
	int before = g_fail;
	CTFontRef plain = test_font(12.0);
	CFMutableDictionaryRef attrs, variation;
	CFNumberRef axis, weight;
	CTFontDescriptorRef descriptor;
	CTFontRef asked;
	CFDictionaryRef back;
	int tag = 0x77676874;	/* 'wght', the four-character axis code Apple's keys hold */
	int value = 700;
	int answered_nothing;

	check(plain != NULL, "no font, so there is no instance to ask about");
	if (plain == NULL) {
		return g_fail - before;
	}
	answered_nothing = (CTFontCopyVariation(plain) == NULL);
	check(answered_nothing, "a font asked for no instance reported one anyway");

	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	variation = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					      &kCFTypeDictionaryValueCallBacks);
	axis = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &tag);
	weight = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
	check(attrs != NULL && variation != NULL && axis != NULL && weight != NULL,
	      "the test could not make its instance");
	if (attrs == NULL || variation == NULL || axis == NULL || weight == NULL) {
		return g_fail - before;
	}
	CFDictionarySetValue(variation, axis, weight);
	CFRelease(axis);
	CFRelease(weight);
	CFDictionarySetValue(attrs, kCTFontNameAttribute, CFSTR("DejaVuSansMono-Bold"));
	CFDictionarySetValue(attrs, kCTFontVariationAttribute, variation);
	descriptor = CTFontDescriptorCreateWithAttributes(attrs);
	asked = (descriptor != NULL) ? CTFontCreateWithFontDescriptor(descriptor, 12.0, NULL) : NULL;
	check(asked != NULL, "a descriptor with a name and an instance could not make a font");
	if (asked != NULL) {
		back = CTFontCopyVariation(asked);
		check(back != NULL, "a font asked for an instance reported none");
		check(back != NULL && CFEqual(back, variation),
		      "the instance that came back is not the one that went in");
		if (back != NULL) {
			CFRelease(back);
		}
		CFRelease(asked);
	}
	check(CTFontCopyVariation(NULL) == NULL, "a NULL font produced an instance");
	if (descriptor != NULL) {
		CFRelease(descriptor);
	}
	CFRelease(variation);
	CFRelease(attrs);
	return g_fail - before;
}

/* TWO READERS THE CASES BELOW NEED AND THE LIBRARY KEEPS TO ITSELF: the cases decode the tables THEY ask the door
 * to copy, which is this tree's rule for holding a parser -- a parser checked against its own output checks
 * nothing. Written here rather than shared, because a case that used the library's arithmetic would agree with the
 * library's mistakes. */
static unsigned case_be16(const UInt8 *bytes)
{
	return ((unsigned)bytes[0] << 8) | (unsigned)bytes[1];
}

static double case_fixed(const UInt8 *bytes)
{
	unsigned raw = ((unsigned)bytes[0] << 24) | ((unsigned)bytes[1] << 16)
		     | ((unsigned)bytes[2] << 8) | (unsigned)bytes[3];
	double whole = (raw >= 0x80000000u) ? ((double)raw - 4294967296.0) : (double)raw;

	return whole / 65536.0;
}

/* THE VARIATION AXES, AND THE CASE READS `fvar` ITSELF THROUGH THE PUBLIC TABLE DOOR AND RECOMPUTES EVERY NUMBER
 * THE AXES DOOR REPORTS. Apple's own sentence is what the shape is held against -- "this function returns an array
 * of variation axis dictionaries. Each variation axis dictionary contains THE FIVE variation axis keys above" --
 * so the count of keys is asserted as well as the values, and the values are asserted twice: against what the table
 * SAYS, and against each other (min <= default <= max).
 *
 * AND THE OWED NOTE THIS DOOR CARRIED IS NOW FALSE RATHER THAN STALE, which is why the door is taken tonight: it
 * read "no variable font is installed on this machine, so a case could only exercise the NO-`fvar` path". A
 * variable face IS installed, the note was written without looking for one, and this case is what falsified it.
 * THE FACE IS PRINTED, because which face answered is a fact about this machine rather than about the library. */
static int case_font_variation_axes(void)
{
	int before = g_fail;
	const char *path = "/usr/share/fonts/opentype/cantarell/Cantarell-VF.otf";
	CGDataProviderRef provider;
	CGFontRef face;
	CTFontRef font, plain;
	CFArrayRef axes;
	CFDataRef fvar;
	const UInt8 *bytes;
	CFIndex length, count, i;
	unsigned axis_count, axis_size, offset_to_data;

	provider = CGDataProviderCreateWithFilename(path);
	check(provider != NULL, "the suite could not open the variable face %s", path);
	if (provider == NULL) {
		return g_fail - before;
	}
	face = CGFontCreateWithDataProvider(provider);
	check(face != NULL, "the engine would not load the variable face %s", path);
	if (face == NULL) {
		return g_fail - before;
	}
	font = CTFontCreateWithGraphicsFont(face, 12.0, NULL, NULL);
	check(font != NULL, "no font could be made from the variable face");
	if (font == NULL) {
		return g_fail - before;
	}
	printf("  (note: the variable face answering the `fvar` path is %s)\n", path);

	fvar = CTFontCopyTable(font, (CTFontTableTag)kCTFontTableFvar, kCTFontTableOptionNoOptions);
	check(fvar != NULL, "the variable face reports no `fvar` table, so nothing can be held against the door");
	if (fvar == NULL) {
		return g_fail - before;
	}
	length = CFDataGetLength(fvar);
	bytes = (const UInt8 *)CFDataGetBytePtr(fvar);
	check(bytes != NULL && length >= 16, "`fvar` came back as %ld bytes, and its header needs sixteen", (long)length);
	if (bytes == NULL || length < 16) {
		CFRelease(fvar);
		return g_fail - before;
	}
	offset_to_data = case_be16(bytes + 4);
	axis_count = case_be16(bytes + 8);
	axis_size = case_be16(bytes + 10);
	check(axis_count > 0, "the variable face states no axis, so an empty array would hold nothing");
	check(axis_size >= 20, "the face states an axis record of %u bytes, and the format's is at least twenty",
	      axis_size);
	check((CFIndex)(offset_to_data + axis_count * axis_size) <= length,
	      "the axis records run past the table: %u at %u in %ld bytes", axis_count, offset_to_data, (long)length);

	axes = CTFontCopyVariationAxes(font);
	check(axes != NULL, "the axes door answered NULL for a face that carries `fvar`");
	if (axes == NULL) {
		CFRelease(fvar);
		return g_fail - before;
	}
	count = CFArrayGetCount(axes);
	check(count == (CFIndex)axis_count, "the door answered %ld axes where the table states %u",
	      (long)count, axis_count);
	for (i = 0; i < count && i < (CFIndex)axis_count; i++) {
		CFDictionaryRef axis = (CFDictionaryRef)CFArrayGetValueAtIndex(axes, i);
		const UInt8 *record = bytes + (CFIndex)offset_to_data + i * (CFIndex)axis_size;
		CFNumberRef number;
		CFTypeRef name;
		double minimum, maximum, deflt, got;
		int tag, got_tag = 0;

		check(CFGetTypeID(axis) == CFDictionaryGetTypeID(), "axis %ld is not a dictionary", (long)i);
		/* APPLE'S OWN COUNT: "each variation axis dictionary contains the five variation axis keys above". */
		check(CFDictionaryGetCount(axis) == 5,
		      "axis %ld carries %ld keys where Apple's sentence says five", (long)i,
		      (long)CFDictionaryGetCount(axis));
		minimum = case_fixed(record + 4);
		deflt = case_fixed(record + 8);
		maximum = case_fixed(record + 12);
		tag = (int)((unsigned)record[0] << 24 | (unsigned)record[1] << 16 |
			    (unsigned)record[2] << 8 | (unsigned)record[3]);
		check(minimum <= deflt && deflt <= maximum,
		      "the table's own bounds are not ordered: %g, %g, %g", minimum, deflt, maximum);
		printf("  (note: axis %ld is '%c%c%c%c', %g to %g, default %g)\n", (long)i,
		       (char)record[0], (char)record[1], (char)record[2], (char)record[3],
		       minimum, maximum, deflt);

		number = (CFNumberRef)CFDictionaryGetValue(axis, kCTFontVariationAxisIdentifierKey);
		check(number != NULL && CFGetTypeID(number) == CFNumberGetTypeID(),
		      "axis %ld carries no numeric identifier", (long)i);
		if (number != NULL && CFGetTypeID(number) == CFNumberGetTypeID()) {
			check(CFNumberGetValue(number, kCFNumberSInt32Type, &got_tag),
			      "axis %ld's identifier is not an integer", (long)i);
			check(got_tag == tag,
			      "axis %ld identifies itself as 0x%08X where the table writes the four-character code 0x%08X",
			      (long)i, (unsigned)got_tag, (unsigned)tag);
		}
		number = (CFNumberRef)CFDictionaryGetValue(axis, kCTFontVariationAxisMinimumValueKey);
		got = 0.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - minimum) < 0.0001,
		      "axis %ld's minimum is %g where the table writes %g", (long)i, got, minimum);
		number = (CFNumberRef)CFDictionaryGetValue(axis, kCTFontVariationAxisMaximumValueKey);
		got = 0.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - maximum) < 0.0001,
		      "axis %ld's maximum is %g where the table writes %g", (long)i, got, maximum);
		number = (CFNumberRef)CFDictionaryGetValue(axis, kCTFontVariationAxisDefaultValueKey);
		got = 0.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - deflt) < 0.0001,
		      "axis %ld's default is %g where the table writes %g", (long)i, got, deflt);
		name = CFDictionaryGetValue(axis, kCTFontVariationAxisNameKey);
		check(name != NULL && CFGetTypeID(name) == CFStringGetTypeID(),
		      "axis %ld has no name, and the face's `name` table has an entry for the index it states", (long)i);
	}
	CFRelease(axes);
	CFRelease(fvar);

	/* AND A FACE WITH NO `fvar` ANSWERS AN EMPTY ARRAY RATHER THAN NULL, which is this door's recorded choice: "this
	 * face has no variation axes" is an ANSWER, and NULL is kept for the door that could not be asked. */
	plain = test_font(12.0);
	if (plain != NULL) {
		CFArrayRef none = CTFontCopyVariationAxes(plain);

		check(none != NULL, "a face with no `fvar` answered NULL, and \"no axes\" is this door's empty array");
		if (none != NULL) {
			check(CFArrayGetCount(none) == 0, "a face with no `fvar` answered %ld axes",
			      (long)CFArrayGetCount(none));
			CFRelease(none);
		}
	}
	check(CTFontCopyVariationAxes(NULL) == NULL, "a NULL font produced axes");
	return g_fail - before;
}

/* A FEATURE SETTING IS A TUPLE, AND TOGGLING ONE KEEPS THE OTHERS -- WHICH IS A BUG THIS CASE CAUGHT RATHER THAN A
 * PROPERTY IT DECORATES. Apple names `kCTFontFeatureSettingsAttribute` among the keys a descriptor copy MERGES, and
 * its value is an array of setting dictionaries; the copy door merged only DICTIONARIES and replaced everything
 * else, so the second setting silently took the first one's place. IT WAS UNREACHABLE BEFORE THIS DOOR: there was
 * no supported way to build a settings array with two entries in it, which is why the bug waited for the door and
 * why the case was written WITH the door rather than after it.
 *
 * THE FACE IS THE SUITE'S OWN AND NO FEATURE IS APPLIED BY IT: this door is about what a font was ASKED FOR, so
 * nothing here needs a `feat` table -- and no face installed here carries one, which is why CTFontCopyFeatures is
 * still owed while this door is not. */
static int case_font_feature_settings(void)
{
	int before = g_fail;
	int type_value = 1, first_value = 1, second_value = 2;
	CFNumberRef type, first, second;
	CTFontDescriptorRef plain, one, two, again;
	CFTypeRef settings, tuple;
	CTFontRef plain_font, font;
	CFArrayRef back;

	type = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &type_value);
	first = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &first_value);
	second = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &second_value);
	/* THE DESCRIPTOR COMES FROM A FONT RATHER THAN FROM A NAME, AND THAT IS NECESSARY RATHER THAN TIDY: a case runs in
	 * its own CHILD PROCESS, so the face has to be REGISTERED here before a name can resolve to it -- which is what
	 * `test_font` does, and why its registration check is the suite's first act in every case that needs a face. A
	 * descriptor built from the name alone finds nothing the first time and makes this case fail for a reason that has
	 * nothing to do with the door. */
	font = test_font(12.0);
	plain_font = font;
	check(plain_font != NULL, "no font, so there is no descriptor to copy features onto");
	if (plain_font == NULL) {
		return g_fail - before;
	}
	plain = CTFontCopyFontDescriptor(font);
	check(plain != NULL && CTFontDescriptorCopyAttribute(plain, kCTFontNameAttribute) != NULL,
	      "the descriptor taken from the font does not carry the name, so no font can be made from a copy of it");
	if (plain == NULL) {
		return g_fail - before;
	}

	/* THE FIRST SETTING IS A TUPLE, and it carries the SETTING and not merely the pair: a tuple that named a
	 * selector without saying what state it is in would say nothing about the thing it exists to carry. */
	one = CTFontDescriptorCreateCopyWithFeature(plain, type, first);
	check(one != NULL, "the feature copy answered nothing");
	if (one == NULL) {
		return g_fail - before;
	}
	settings = CTFontDescriptorCopyAttribute(one, kCTFontFeatureSettingsAttribute);
	check(settings != NULL && CFGetTypeID(settings) == CFArrayGetTypeID(),
	      "the copy carries no feature-settings array");
	if (settings != NULL && CFGetTypeID(settings) == CFArrayGetTypeID()) {
		check(CFArrayGetCount((CFArrayRef)settings) == 1,
		      "one setting produced %ld of them", (long)CFArrayGetCount((CFArrayRef)settings));
		tuple = CFArrayGetValueAtIndex((CFArrayRef)settings, 0);
		check(tuple != NULL && CFGetTypeID(tuple) == CFDictionaryGetTypeID(), "the setting is not a dictionary");
		if (tuple != NULL && CFGetTypeID(tuple) == CFDictionaryGetTypeID()) {
			check(CFEqual(CFDictionaryGetValue((CFDictionaryRef)tuple,
							   kCTFontFeatureTypeIdentifierKey), type),
			      "the setting does not carry the type identifier it was given");
			check(CFEqual(CFDictionaryGetValue((CFDictionaryRef)tuple,
							   kCTFontFeatureSelectorIdentifierKey), first),
			      "the setting does not carry the selector identifier it was given");
			check(CFDictionaryGetValue((CFDictionaryRef)tuple,
						   kCTFontFeatureSelectorSettingKey) == kCFBooleanTrue,
			      "the setting does not say ON, and a door handed a selector to toggle is asked for it enabled");
		}
		CFRelease(settings);
	}

	/* AND THE SECOND SETTING IS WHERE THE BUG WAS: the array must GROW rather than be replaced. */
	two = CTFontDescriptorCreateCopyWithFeature(one, type, second);
	check(two != NULL, "the second feature copy answered nothing");
	if (two != NULL) {
		settings = CTFontDescriptorCopyAttribute(two, kCTFontFeatureSettingsAttribute);
		check(settings != NULL && CFGetTypeID(settings) == CFArrayGetTypeID(),
		      "the second copy carries no feature-settings array");
		if (settings != NULL && CFGetTypeID(settings) == CFArrayGetTypeID()) {
			CFIndex i, found_first = -1, found_second = -1;

			check(CFArrayGetCount((CFArrayRef)settings) == 2,
			      "toggling a second feature left %ld settings, so the copy REPLACED the array it was told to MERGE",
			      (long)CFArrayGetCount((CFArrayRef)settings));
			for (i = 0; i < CFArrayGetCount((CFArrayRef)settings); i++) {
				CFDictionaryRef each = (CFDictionaryRef)CFArrayGetValueAtIndex((CFArrayRef)settings, i);

				if (CFEqual(CFDictionaryGetValue(each, kCTFontFeatureSelectorIdentifierKey), first)) {
					found_first = i;
				}
				if (CFEqual(CFDictionaryGetValue(each, kCTFontFeatureSelectorIdentifierKey), second)) {
					found_second = i;
				}
			}
			check(found_first >= 0, "the first setting was dropped by the second copy");
			check(found_second >= 0, "the second setting never arrived");
		}
		CFRelease(settings);
	}

	/* AND THE SAME PAIR AGAIN REPLACES RATHER THAN APPENDS, which is the other half of "merge by (type, selector)". */
	again = (two != NULL) ? CTFontDescriptorCreateCopyWithFeature(two, type, first) : NULL;
	if (again != NULL) {
		settings = CTFontDescriptorCopyAttribute(again, kCTFontFeatureSettingsAttribute);
		check(settings != NULL && CFGetTypeID(settings) == CFArrayGetTypeID() &&
		      CFArrayGetCount((CFArrayRef)settings) == 2,
		      "re-sending a setting that is already there changed the number of settings");
		if (settings != NULL) {
			CFRelease(settings);
		}
	}

	/* AND THE FONT MADE FROM SUCH A DESCRIPTOR ANSWERS THEM, WHICH IS `CTFontCopyFeatureSettings`' WHOLE DOOR. */
	font = (two != NULL) ? CTFontCreateWithFontDescriptor(two, 12.0, NULL) : NULL;
	check(font != NULL, "no font could be made from a descriptor carrying feature settings");
	if (font != NULL) {
		back = CTFontCopyFeatureSettings(font);
		check(back != NULL, "a font asked for two settings answered none");
		if (back != NULL) {
			CFTypeRef asked = CTFontDescriptorCopyAttribute(two, kCTFontFeatureSettingsAttribute);

			check(CFArrayGetCount(back) == 2, "the font answered %ld settings, and it was asked for two",
			      (long)CFArrayGetCount(back));
			check(asked != NULL && CFEqual(back, asked),
			      "the settings that came back are not the ones that went in");
			if (asked != NULL) {
				CFRelease(asked);
			}
			CFRelease(back);
		}
		CFRelease(font);
	}

	/* AND A FONT ASKED FOR NOTHING ANSWERS NULL, WHICH IS APPLE'S OWN SECOND HALF: "or NULL if the default
	 * settings should be used". */
	font = CTFontCreateWithFontDescriptor(plain, 12.0, NULL);
	if (font != NULL) {
		check(CTFontCopyFeatureSettings(font) == NULL,
		      "a font asked for no setting answered something other than NULL");
		CFRelease(font);
	}
	check(CTFontCopyFeatureSettings(NULL) == NULL, "a NULL font produced settings");
	check(CTFontDescriptorCreateCopyWithFeature(NULL, type, first) == NULL,
	      "a NULL descriptor produced a copy");
	check(CTFontDescriptorCreateCopyWithFeature(plain, NULL, first) == NULL,
	      "a copy was made with no type identifier");
	check(CTFontDescriptorCreateCopyWithFeature(plain, type, NULL) == NULL,
	      "a copy was made with no selector identifier");

	CFRelease(plain);
	if (plain_font != NULL) {
		CFRelease(plain_font);
	}
	if (one != NULL) {
		CFRelease(one);
	}
	if (two != NULL) {
		CFRelease(two);
	}
	if (again != NULL) {
		CFRelease(again);
	}
	CFRelease(type);
	CFRelease(first);
	CFRelease(second);
	return g_fail - before;
}

/* TWO HELPERS THE REGISTRY CREATORS' CASES NEED, AND BOTH SAY SOMETHING THE CASES DEPEND ON. The registry here is
 * THE PROCESS'S -- MEASURED: a fresh process sees ZERO faces through `cg_font_enumerate`, because this machine's
 * engine is configured with no font directory that holds any -- so a case must PUT the faces there that it asks
 * about, and PRINT that it did. Which faces answered is then a fact about the case rather than about the machine. */
static bool test_register_path(const char *path)
{
	if (access(path, R_OK) != 0) {
		return false;
	}
	return cg_font_register(path) != 0 || cg_font_is_registered(path);
}

/* A FONT MADE FROM A FILE, WHICH IS HOW A CASE NAMES A FACE THE REGISTRY DOES NOT HAVE YET. */
static CTFontRef test_font_from_path(const char *path)
{
	CGDataProviderRef provider = CGDataProviderCreateWithFilename(path);
	CGFontRef face = (provider != NULL) ? CGFontCreateWithDataProvider(provider) : NULL;
	CTFontRef font = NULL;

	if (face != NULL) {
		font = CTFontCreateWithGraphicsFont(face, 12.0, NULL, NULL);
		CGFontRelease(face);
	}
	return font;
}

/* THE TWO CREATORS THAT SEARCH FOR A FACE BY ITS FAMILY AND ITS TRAITS, AND EACH IS HELD AGAINST A FACE THE CASE PUT
 * IN THE REGISTRY ITSELF. Everything asserted here is a fact about `DejaVu Sans Mono` and `DejaVu Sans`, which are
 * on this machine and are printed.
 *
 * AND THE ASSERTION THAT MATTERS MOST IS THE FAMILY DOOR'S LAST ONE. The original is BOLD AND MONOSPACED and the
 * family asked for is one whose faces are NEITHER monospaced nor, in the face that answers, absent: demanding
 * monospace of another family would ask for something no face in it can be, so the door compares under the STYLE
 * mask it records -- and the case asserts the mono bit is NOT carried across, which is that decision measured
 * rather than described. */
static int case_font_family_and_traits(void)
{
	int before = g_fail;
	const char *mono_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
	const char *sans_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
	CTFontRef bold_mono, regular_mono, cleared, bolded, unchanged, unsatisfiable, other_family, missing;
	CFStringRef mono_family = NULL, sans_family = CFSTR("DejaVu Sans");

	check(test_register_path(mono_path), "the registry refused %s", mono_path);
	check(test_register_path(sans_path), "the registry refused %s", sans_path);
	printf("  (note: the case registered %s and %s; this process's registry is what the doors see)\n",
	       mono_path, sans_path);

	/* `test_font` REGISTERS ITS OWN FACE TOO (DejaVuSansMono-Bold) and answers it at the size given, which is how the
	 * size-preservation half is held: 18 points in, 18 points out. */
	bold_mono = test_font(18.0);
	check(bold_mono != NULL, "no font, so there is nothing to copy");
	if (bold_mono == NULL) {
		return g_fail - before;
	}
	check((CTFontGetSymbolicTraits(bold_mono) & kCTFontBoldTrait) != 0,
	      "the face under test is not bold, so a request to clear bold would prove nothing");
	mono_family = CTFontCopyFamilyName(bold_mono);
	check(mono_family != NULL, "the face under test has no family name");

	/* CLEARING BOLD, WHICH ONLY WORKS IF THE REQUEST IS COMPUTED FROM THE FONT'S OWN TRAITS. */
	cleared = CTFontCreateCopyWithSymbolicTraits(bold_mono, 0.0, NULL, 0, kCTFontBoldTrait);
	check(cleared != NULL, "no face in the family satisfied a request to clear bold");
	if (cleared != NULL) {
		CFStringRef got = CTFontCopyFamilyName(cleared);

		check((CTFontGetSymbolicTraits(cleared) & kCTFontBoldTrait) == 0,
		      "the copy kept bold after being asked to clear it");
		check(got != NULL && CFEqual(got, mono_family), "the copy left the family it was asked to stay in");
		check(!CFEqual(cleared, bold_mono), "clearing bold answered the very same face");
		check(CTFontGetSize(cleared) == 18.0, "a size of 0.0 did not preserve the original's size (%g)",
		      (double)CTFontGetSize(cleared));
		if (got != NULL) {
			CFRelease(got);
		}
		CFRelease(cleared);
	}

	/* AND SETTING IT, FROM THE FACE THAT HAS NONE -- a different question with a different answer. */
	regular_mono = CTFontCreateWithName(CFSTR("DejaVuSansMono"), 12.0, NULL);
	check(regular_mono != NULL, "the plain face of the family could not be made by name");
	if (regular_mono != NULL) {
		check((CTFontGetSymbolicTraits(regular_mono) & kCTFontBoldTrait) == 0,
		      "the face named for the plain one is bold, so setting bold would prove nothing");
		bolded = CTFontCreateCopyWithSymbolicTraits(regular_mono, 12.0, NULL, kCTFontBoldTrait, kCTFontBoldTrait);
		check(bolded != NULL, "no face in the family satisfied a request for bold");
		if (bolded != NULL) {
			check((CTFontGetSymbolicTraits(bolded) & kCTFontBoldTrait) != 0,
			      "the copy is not bold after being asked for bold");
			check(!CFEqual(bolded, regular_mono), "asking for bold answered the face that is not bold");
			CFRelease(bolded);
		}
		/* AN EMPTY MASK IS NOT A SEARCH: it asks for no change, so the font's own face is the answer. */
		unchanged = CTFontCreateCopyWithSymbolicTraits(regular_mono, 0.0, NULL, 0, 0);
		check(unchanged != NULL && CFEqual(unchanged, regular_mono),
		      "an empty trait mask did not answer the face it was given");
		if (unchanged != NULL) {
			CFRelease(unchanged);
		}
	}

	/* AND A REQUEST NOTHING CAN MEET ANSWERS NULL RATHER THAN A NEAR MISS, which is what "or NULL if none found in
	 * the system" is for: no face of this family is EXPANDED. */
	unsatisfiable = CTFontCreateCopyWithSymbolicTraits(bold_mono, 0.0, NULL, kCTFontExpandedTrait,
							   kCTFontExpandedTrait);
	check(unsatisfiable == NULL, "an expanded DejaVu Sans Mono was found, and no such face exists");
	if (unsatisfiable != NULL) {
		CFRelease(unsatisfiable);
	}

	/* THE FAMILY DOOR, AND THE MONO BIT IS THE MEASUREMENT: it must NOT come across. */
	other_family = CTFontCreateCopyWithFamily(bold_mono, 0.0, NULL, sans_family);
	check(other_family != NULL, "no face of \"DejaVu Sans\" answered the family door");
	if (other_family != NULL) {
		CFStringRef got = CTFontCopyFamilyName(other_family);

		check(got != NULL && CFEqual(got, sans_family), "the family door answered another family");
		check((CTFontGetSymbolicTraits(other_family) & kCTFontBoldTrait) != 0,
		      "the family door lost the original's bold, which is a style any family can honour");
		check((CTFontGetSymbolicTraits(other_family) & kCTFontMonoSpaceTrait) == 0,
		      "the family door demanded MONOSPACE of a family that has none -- the style mask is what makes this door answer");
		check(CTFontGetSize(other_family) == 18.0, "a size of 0.0 did not preserve the size (%g)",
		      (double)CTFontGetSize(other_family));
		if (got != NULL) {
			CFRelease(got);
		}
		CFRelease(other_family);
	}

	/* AND THE SAME FAMILY ANSWERS THE FACE IN IT THAT CARRIES THE ORIGINAL'S STYLE. */
	if (regular_mono != NULL && mono_family != NULL) {
		CTFontRef same = CTFontCreateCopyWithFamily(regular_mono, 0.0, NULL, mono_family);

		check(same != NULL, "the family door could not find the family the font is already in");
		if (same != NULL) {
			check((CTFontGetSymbolicTraits(same) & kCTFontMonoSpaceTrait) != 0,
			      "the answer left a monospaced family, and it was asked for that family");
			check((CTFontGetSymbolicTraits(same) & kCTFontBoldTrait) == 0,
			      "the family door answered a BOLD face for a request in the original's plain style");
			CFRelease(same);
		}
	}

	/* A FAMILY NOBODY HAS ANSWERS NULL, AND SO DOES A MISSING ARGUMENT. */
	missing = CTFontCreateCopyWithFamily(bold_mono, 0.0, NULL, CFSTR("No Such Family At All"));
	check(missing == NULL, "a family that does not exist produced a font");
	check(CTFontCreateCopyWithFamily(NULL, 0.0, NULL, sans_family) == NULL, "a NULL font produced a font");
	check(CTFontCreateCopyWithFamily(bold_mono, 0.0, NULL, NULL) == NULL, "a NULL family produced a font");
	check(CTFontCreateCopyWithSymbolicTraits(NULL, 0.0, NULL, 0, kCTFontBoldTrait) == NULL,
	      "a NULL font produced a font");

	if (mono_family != NULL) {
		CFRelease(mono_family);
	}
	if (regular_mono != NULL) {
		CFRelease(regular_mono);
	}
	CFRelease(bold_mono);
	return g_fail - before;
}

/* THE CASCADE SEARCH AND THE ROLES, WHICH ARE THE TWO CREATORS LEFT THAT ASK A QUESTION ABOUT A FACE RATHER THAN
 * FOR ONE. Both are held against a registry the CASE fills, and both have a half that Apple does not spell out:
 * for the cascade it is WHERE THE LIST COMES FROM, and for a role it is WHICH FACE PLAYS IT.
 *
 * AND THE CASCADE HALF IS HELD BY MAKING THE TWO POSSIBLE ANSWERS DIFFERENT. Two registered faces cover the
 * character used here (Hebrew alef, which the suite's own monospaced face does NOT cover): `DejaVu Sans` and
 * `FreeSerif` -- and the tier's order puts `DejaVu Sans` first, so a font with NO cascade list must answer
 * DejaVu Sans and the same font WITH a cascade list naming FreeSerif must answer FreeSerif. A case that only asked
 * "did something answer" would pass whichever half was broken. */
static int case_font_cascade_and_roles(void)
{
	int before = g_fail;
	const char *sans_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
	const char *serif_path = "/usr/share/fonts/truetype/freefont/FreeSerif.ttf";
	CTFontRef mono, from_serif, cascade_font, kept, found, role, again;
	CTFontDescriptorRef mono_descriptor, serif_descriptor, with_cascade;
	CFMutableDictionaryRef entry, additions;
	CFMutableArrayRef cascade;
	CFStringRef serif_name = NULL, system_family = NULL;
	CFArrayRef candidates;
	UniChar alef[1];

	mono = test_font(12.0);	/* registers DejaVuSansMono-Bold, and NOTHING ELSE YET: the registrations come below,
				 * after the measurement that needs an empty registry */
	check(mono != NULL, "no font, so there is nothing to cascade from");
	if (mono == NULL) {
		return g_fail - before;
	}

	/* THE CHARACTER IS THE POINT: the monospaced face has no Hebrew, which the case asserts rather than assumes. */
	alef[0] = 0x05D0;
	{
		CGGlyph glyphs[1] = { 0 };

		check(!CTFontGetGlyphsForCharacters(mono, alef, glyphs, 1),
		      "the face under test maps Hebrew alef, so it could not show a cascade answering anything");
	}

	/* APPLE'S FIRST HALF: a string the CURRENT FONT can encode is answered BY that font, retained. */
	kept = CTFontCreateForString(mono, CFSTR("A"), CFRangeMake(0, 1));
	check(kept == mono, "\"if the current font is capable of encoding the string range then it will be retained and returned\" -- it was not");
	if (kept != NULL) {
		CFRelease(kept);
	}

	/* **AND THE CASCADE EITHER ANSWERS NOTHING OR ANSWERS A FACE THAT REALLY COVERS HEBREW — WHICH IS THE
	 * INVARIANT. "NOTHING" WAS ONLY EVER AN ACCIDENT OF THIS HOST.**
	 *
	 * This used to assert `found == NULL`, reading "no face covers this" before the two registrations below, and
	 * that held for exactly as long as the font registry found NO FONTS AT ALL — which it did, because it was
	 * looking in `/System/Library/Fonts`. Once it was made to look where this operating system keeps its faces,
	 * the host's own DejaVu answered for Hebrew and this failed: THE CASE HAD ENCODED A CONSEQUENCE OF THE BUG.
	 *
	 * What it should assert is the thing a cascade must never do — return a face that cannot show the character —
	 * and what it may now also do is answer a coverer, if the host has one. The measurement is still taken HERE,
	 * before the registrations, for the reason the old comment gives: "no face covers this" and "no face is
	 * registered" are different measurements and this one has to be taken first or it is neither. */
	found = CTFontCreateForString(mono, CFSTR("\u05D0"), CFRangeMake(0, 1));
	if (found != NULL) {
		CGGlyph cover[1] = { 0 };

		check(CTFontGetGlyphsForCharacters(found, alef, cover, 1),
		      "the cascade answered a face that does NOT cover Hebrew, which is worse than answering nothing");
		CFRelease(found);
	} else {
		printf("  (note: nothing on this host covers Hebrew yet, so the cascade answered nothing)\n");
	}

	check(test_register_path(sans_path), "the registry refused %s", sans_path);
	check(test_register_path(serif_path), "the registry refused %s", serif_path);
	printf("  (note: the case registered %s and %s beside the suite's own face, AFTER measuring the empty answer)\n",
	       sans_path, serif_path);

	/* AND THE CASCADE LIST IS BUILT BY THE CASE, OUT OF A DESCIPTOR THAT NAMES THE FACE TO USE. */
	from_serif = test_font_from_path(serif_path);
	check(from_serif != NULL, "the face at %s could not be loaded from its file", serif_path);
	if (from_serif != NULL) {
		serif_name = CTFontCopyPostScriptName(from_serif);
	}
	mono_descriptor = CTFontCopyFontDescriptor(mono);
	check(serif_name != NULL && mono_descriptor != NULL, "the case could not build its cascade inputs");
	if (serif_name != NULL && mono_descriptor != NULL) {
		entry = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
						  &kCFTypeDictionaryValueCallBacks);
		CFDictionarySetValue(entry, kCTFontNameAttribute, serif_name);
		serif_descriptor = CTFontDescriptorCreateWithAttributes(entry);
		CFRelease(entry);
		cascade = CFArrayCreateMutable(kCFAllocatorDefault, 1, &kCFTypeArrayCallBacks);
		CFArrayAppendValue(cascade, serif_descriptor);
		additions = CFDictionaryCreateMutable(kCFAllocatorDefault, 1, &kCFTypeDictionaryKeyCallBacks,
						      &kCFTypeDictionaryValueCallBacks);
		CFDictionarySetValue(additions, kCTFontCascadeListAttribute, cascade);
		with_cascade = CTFontDescriptorCreateCopyWithAttributes(mono_descriptor, additions);
		CFRelease(additions);
		CFRelease(cascade);
		CFRelease(serif_descriptor);
		check(with_cascade != NULL, "the descriptor would not take a cascade list");
		if (with_cascade != NULL) {
			cascade_font = CTFontCreateWithFontDescriptor(with_cascade, 12.0, NULL);
			check(cascade_font != NULL, "no font could be made from a descriptor carrying a cascade list");
			if (cascade_font != NULL) {
				CFStringRef got;

				found = CTFontCreateForString(cascade_font, CFSTR("\u05D0"), CFRangeMake(0, 1));
				check(found != NULL, "the cascade list named a face and the door did not use it");
				if (found != NULL) {
					got = CTFontCopyFamilyName(found);
					check(got != NULL && CFEqual(got, CFSTR("FreeSerif")),
					      "the door answered from somewhere other than the cascade list it was given");
					if (got != NULL) {
						CFRelease(got);
					}
					CFRelease(found);
				}
				CFRelease(cascade_font);
			}
			CFRelease(with_cascade);
		}
	}
	if (serif_name != NULL) {
		CFRelease(serif_name);
	}
	if (mono_descriptor != NULL) {
		CFRelease(mono_descriptor);
	}
	if (from_serif != NULL) {
		CFRelease(from_serif);
	}

	/* AND WITHOUT A CASCADE LIST THE TIER'S OWN ORDER ANSWERS, which is `DejaVu Sans` before `FreeSerif`. */
	found = CTFontCreateForString(mono, CFSTR("\u05D0"), CFRangeMake(0, 1));
	check(found != NULL, "no registered face covers Hebrew, and two were registered");
	if (found != NULL) {
		CFStringRef got = CTFontCopyFamilyName(found);

		check(got != NULL && CFEqual(got, CFSTR("DejaVu Sans")),
		      "the registry walk answered a family other than the first in the tier's order");
		if (got != NULL) {
			CFRelease(got);
		}
		CFRelease(found);
	}

	/* AND THE RANGE RULE: a length of 0 runs to the end of the string, and ONE face must cover the WHOLE range. */
	{
		CTFontRef first_two = CTFontCreateForString(mono, CFSTR("AB\u05D0"), CFRangeMake(0, 2));
		CTFontRef tail = CTFontCreateForString(mono, CFSTR("AB\u05D0"), CFRangeMake(2, 0));

		check(first_two == mono, "a range the current font can encode did not answer with the current font");
		if (first_two != NULL) {
			CFRelease(first_two);
		}
		check(tail != NULL && tail != mono, "a length of 0 did not run to the end of the string");
		if (tail != NULL) {
			CFRelease(tail);
		}
	}
	check(CTFontCreateForString(NULL, CFSTR("A"), CFRangeMake(0, 1)) == NULL, "a NULL font produced a font");
	check(CTFontCreateForString(mono, NULL, CFRangeMake(0, 1)) == NULL, "a NULL string produced a font");
	/* AND AN EMPTY RANGE IS A RANGE AT THE END OF THE STRING OR AN EMPTY STRING -- NOT A LENGTH OF 0, which means
	 * "to the end" and is measured two blocks above. Getting this wrong is how the case first failed: it asked for a
	 * length of 0 and got the font back, which is the range rule working. */
	check(CTFontCreateForString(mono, CFSTR("A"), CFRangeMake(1, 0)) == NULL,
	      "a range starting past the last character produced a font");
	check(CTFontCreateForString(mono, CFSTR(""), CFRangeMake(0, 0)) == NULL,
	      "an empty string produced a font, and no range asks for nothing");

	/* **THE ROLES, AND THE PREFERENCE THAT ANSWERS THEM.**
	 *
	 * This block used to assert "the first face in the tier's order" — WHICH WAS THE ABSENCE OF A THEME RATHER THAN
	 * A RULE. It held while this tier had no opinion about which family a user interface uses, and what that
	 * produced was a bold italic, and then a URW serif, as the face a whole desktop is drawn in.
	 *
	 * THE OPINION IS THE USER'S NOW, IT IS A LIST, AND THE LIST IS WALKED IN ORDER. So the thing to assert is the
	 * PREFERENCE, and `AGORA_UI_FONT` is what makes that decidable on any machine instead of a fact about this one:
	 * naming an installed family must answer THAT family, whatever sorts first in the registry. DejaVu is used
	 * because this suite already depends on it — the registration cases above register its Bold face by path. */
	/* **AND THE WALK IS READ THROUGH THE SAME SEAM THE DOOR USES**, which is why the assignment is here: the
	 * emphasised role's case below reads it too, and a block that only CHECKED it left the rest of the case
	 * holding whatever `candidates` had been — MEASURED as a signal 11 in the child, from a pointer nobody had
	 * set. */
	candidates = CTFontMatchingCopyAvailableDescriptors();
	check(candidates != NULL && CFArrayGetCount(candidates) > 0,
	      "the registry is empty, so no role can be answered");
	setenv("AGORA_UI_FONT", "DejaVu Sans", 1);
	role = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
	check(role != NULL, "the system role was not answered");
	if (role != NULL) {
		CFStringRef got = CTFontCopyFamilyName(role);

		check(got != NULL && CFStringCompare(got, CFSTR("DejaVu Sans"), 0) == kCFCompareEqualTo,
		      "a role asked for DejaVu Sans by preference did not answer it");
		if (got != NULL) {
			CFRelease(got);
		}
		CFRelease(role);
	}
	unsetenv("AGORA_UI_FONT");

	/* AND WITH NOTHING NAMED, THE ANSWER IS THIS TIER'S OWN LIST — whose contents are a matter of taste and whose
	 * FIRST PROPERTY is not: it is a REGULAR face. That is what the list exists to guarantee, and it is what the
	 * tier did not guarantee before it had one. */
	role = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
	check(role != NULL, "the system role was not answered");
	if (role != NULL) {
		CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(role);

		check((traits & (kCTFontBoldTrait | kCTFontItalicTrait)) == 0,
		      "the system role answered a BOLD or ITALIC face, which a plain role must never do");
		system_family = CTFontCopyFamilyName(role);
		if (system_family != NULL) {
			char buffer[256];

			if (CFStringGetCString(system_family, buffer, (CFIndex)sizeof buffer, kCFStringEncodingUTF8)) {
				printf("  (note: the system role is answered by \"%s\")\n", buffer);
			}
		}
		check(CTFontGetSize(role) == 12.0, "the system role answered %g points where 12 were asked",
		      (double)CTFontGetSize(role));
		CFRelease(role);
	}

	/* THE FIXED-PITCH ROLE ASKS FOR A FACT RATHER THAN A PLACE, and the registry holds non-monospaced faces for it to
	 * reject: whatever it answers must be monospaced. The size of 0.0 is the other half of this block -- Apple prints
	 * no per-role size, so the documented 12.0 is what it becomes. */
	role = CTFontCreateUIFontForLanguage(kCTFontUserFixedPitchFontType, 0.0, NULL);
	check(role != NULL, "the fixed-pitch role was not answered");
	if (role != NULL) {
		check((CTFontGetSymbolicTraits(role) & kCTFontMonoSpaceTrait) != 0,
		      "the fixed-pitch role answered a face that is not fixed-pitch");
		check(CTFontGetSize(role) == 12.0, "a size of 0.0 did not become the default 12.0 (%g)",
		      (double)CTFontGetSize(role));
		CFRelease(role);
	}

	/* THE EMPHASIZED ROLE ASKS FOR THE BASE FACE BOLD, THROUGH THE TRAIT COPY DOOR -- so what holds it is a RELATION
	 * rather than a name: the same family as the plain role, bold set, and a DIFFERENT face when the base was not
	 * bold. A case that asserted a name here would be asserting which face this machine's registry holds first. */
	role = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
	again = CTFontCreateUIFontForLanguage(kCTFontEmphasizedSystemFontType, 12.0, NULL);
	check(again != NULL, "the emphasized system role was not answered");
	if (role != NULL && again != NULL) {
		CFStringRef base_family = CTFontCopyFamilyName(role);
		CFStringRef bold_family = CTFontCopyFamilyName(again);

		check(base_family != NULL && bold_family != NULL && CFEqual(base_family, bold_family),
		      "the emphasized role left the family the plain role answered from");
		check((CTFontGetSymbolicTraits(again) & kCTFontBoldTrait) != 0,
		      "the emphasized role answered a face that is not bold");
		if ((CTFontGetSymbolicTraits(role) & kCTFontBoldTrait) == 0) {
			check(!CFEqual(again, role),
			      "the base face is not bold, so the emphasized role must be a different face");
		}
		if (base_family != NULL) {
			CFRelease(base_family);
		}
		if (bold_family != NULL) {
			CFRelease(bold_family);
		}
	}
	if (role != NULL) {
		CFRelease(role);
	}
	if (again != NULL) {
		CFRelease(again);
	}

	/* AND THE LANGUAGE IS ACCEPTED AND CHANGES NOTHING, which is this door's recorded boundary: choosing a face for a
	 * localization needs the languages a face supports, and that door is still owed. */
	role = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, CFSTR("en"));
	again = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
	check(role != NULL && again != NULL, "the system role was not answered with a language and without one");
	if (role != NULL && again != NULL) {
		CFStringRef with_language = CTFontCopyFamilyName(role);
		CFStringRef without = CTFontCopyFamilyName(again);

		check(with_language != NULL && without != NULL && CFEqual(with_language, without),
		      "the language changed which face plays the role, and this door records that it cannot");
		if (with_language != NULL) {
			CFRelease(with_language);
		}
		if (without != NULL) {
			CFRelease(without);
		}
	}
	if (role != NULL) {
		CFRelease(role);
	}
	if (again != NULL) {
		CFRelease(again);
	}

	/* AND WHAT IS NOT A ROLE ANSWERS NOTHING: Apple's own "none", and a value past the end of its enum. */
	check(CTFontCreateUIFontForLanguage(kCTFontNoFontType, 12.0, NULL) == NULL, "\"no font type\" produced a font");
	check(CTFontCreateUIFontForLanguage((CTFontUIFontType)27, 12.0, NULL) == NULL,
	      "27 produced a font, and Apple's roles stop at 26");
	check(CTFontCreateUIFontForLanguage((CTFontUIFontType)9999, 12.0, NULL) == NULL,
	      "a value past the end of the enum produced a font");

	if (system_family != NULL) {
		CFRelease(system_family);
	}
	if (candidates != NULL) {
		CFRelease(candidates);
	}
	CFRelease(mono);
	return g_fail - before;
}

/* THE TRAITS DICTIONARY: FOUR KEYS, THREE RECORDED CURVES AND ONE ANCHOR EACH.
 *
 * AND THE CASE DOES TWO DIFFERENT THINGS, BECAUSE THE DOOR DOES. It RECOMPUTES the three floats from the raw `OS/2`
 * and `post` bytes with its own arithmetic -- which holds the door to the MAPPING ITSELF, the recorded deviation,
 * rather than to its own output -- and it asserts APPLE'S ANCHORS on real faces: a regular face is 0.0, an oblique
 * face is positive and a condensed one negative, and all three sit inside the closed range the era's header states.
 * A case that only did the first would pass if the mapping were wrong in the same way in both places; a case that
 * only did the second would pass on a door that rounded everything to a sign.
 *
 * AND THE FACES ARE LOADED FROM THEIR FILES RATHER THAN BY NAME: this door reads TABLES and needs no descriptor, so
 * naming them would only add a registry dependency to a case about arithmetic. */
static int case_font_traits_dictionary(void)
{
	int before = g_fail;
	const char *plain_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
	const char *bold_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf";
	const char *oblique_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Oblique.ttf";
	const char *condensed_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf";
	const char *light_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans-ExtraLight.ttf";
	CTFontRef bold, plain, oblique, condensed, light;
	CFDictionaryRef traits;
	CFNumberRef number;
	CFDataRef os2, post;
	const UInt8 *bytes;
	CFIndex length;
	double want_weight, want_width, want_slant, got;
	unsigned weight_class = 400, width_class = 5;

	bold = test_font_from_path(bold_path);
	check(bold != NULL, "the face at %s could not be loaded", bold_path);
	if (bold == NULL) {
		return g_fail - before;
	}
	traits = CTFontCopyTraits(bold);
	check(traits != NULL, "the traits dictionary came back empty");
	if (traits == NULL) {
		CFRelease(bold);
		return g_fail - before;
	}
	/* APPLE'S OWN COUNT OF THE KEYS, which is what its documentation describes: the symbolic value and the three
	 * normalised floats. A dictionary missing three of its four entries would be worse than a link error, which is
	 * what the owed reason said before this door was taken. */
	check(CFDictionaryGetCount(traits) == 4, "the dictionary carries %ld keys where four are documented",
	      (long)CFDictionaryGetCount(traits));
	number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontWeightTrait);
	check(number != NULL, "the weight entry is missing");
	number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontWidthTrait);
	check(number != NULL, "the width entry is missing");
	number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontSlantTrait);
	check(number != NULL, "the slant entry is missing");

	/* THE SYMBOLIC ENTRY IS THE LIBRARY'S OWN DOOR, WHICH IS APPLE'S OWN SENTENCE ABOUT IT. */
	{
		CFNumberRef symbolic = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontSymbolicTrait);
		int64_t value = 0;

		check(symbolic != NULL && CFNumberGetValue(symbolic, kCFNumberSInt64Type, &value),
		      "the symbolic entry is missing or is not an integer");
		check((CTFontSymbolicTraits)value == CTFontGetSymbolicTraits(bold),
		      "the symbolic entry is 0x%llX where the traits door answers 0x%X",
		      (unsigned long long)value, (unsigned)CTFontGetSymbolicTraits(bold));
	}

	/* AND THE THREE FLOATS, RECOMPUTED FROM THE RAW TABLES BY THE CASE. */
	os2 = CTFontCopyTable(bold, (CTFontTableTag)kCTFontTableOS2, kCTFontTableOptionNoOptions);
	post = CTFontCopyTable(bold, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	check(os2 != NULL && post != NULL, "the face's `OS/2` or `post` table could not be read");
	if (os2 != NULL && post != NULL) {
		bytes = (const UInt8 *)CFDataGetBytePtr(os2);
		length = CFDataGetLength(os2);
		check(bytes != NULL && length >= 8, "`OS/2` came back as %ld bytes, and its first fields need eight",
		      (long)length);
		if (bytes != NULL && length >= 8) {
			weight_class = case_be16(bytes + 4);
			width_class = case_be16(bytes + 6);
		}
		bytes = (const UInt8 *)CFDataGetBytePtr(post);
		length = CFDataGetLength(post);
		check(bytes != NULL && length >= 8, "`post` came back as %ld bytes, and its first fields need eight",
		      (long)length);
		/* THE MAPPING, WRITTEN OUT IN THE CASE AS THE DEVIATION IT IS: the format's own scale, its own regular
		 * value, and Apple's stated anchors at 0.0. */
		want_weight = (weight_class > 400) ? ((double)weight_class - 400.0) / 500.0
						   : -((400.0 - (double)weight_class) / 300.0);
		want_width = ((double)width_class - 5.0) / 4.0;
		want_slant = 0.0;
		if (bytes != NULL && length >= 8) {
			want_slant = -case_fixed(bytes + 4) / 30.0;
			if (want_slant == 0.0) {
				want_slant = 0.0;	/* the same negative-zero rule the door carries, so the note
							 * below prints the door's value and not the case's */
			}
			if (want_slant > 1.0) {
				want_slant = 1.0;
			} else if (want_slant < -1.0) {
				want_slant = -1.0;
			}
		}
		number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontWeightTrait);
		got = 99.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - want_weight) < 0.0001,
		      "the weight is %g where the table's usWeightClass %u maps to %g", got, weight_class, want_weight);
		number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontWidthTrait);
		got = 99.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - want_width) < 0.0001,
		      "the width is %g where the table's usWidthClass %u maps to %g", got, width_class, want_width);
		number = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontSlantTrait);
		got = 99.0;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) &&
		      fabs(got - want_slant) < 0.0001,
		      "the slant is %g where the `post` italic angle maps to %g", got, want_slant);
		printf("  (note: the bold face is usWeightClass %u, usWidthClass %u, and its traits are %g / %g / %g)\n",
		       weight_class, width_class, want_weight, want_width, want_slant);
		CFRelease(post);
		CFRelease(os2);
	}
	CFRelease(traits);

	/* AND APPLE'S ANCHORS ON FACES THAT STATE EACH ONE, which the last four checks are for: 0.0 IS REGULAR, AN
	 * OBLIQUE FACE IS POSITIVE AND A CONDENSED ONE NEGATIVE, AND EVERY VALUE IS INSIDE THE CLOSED RANGE THE ERA'S
	 * HEADER STATES. */
	plain = test_font_from_path(plain_path);
	oblique = test_font_from_path(oblique_path);
	condensed = test_font_from_path(condensed_path);
	light = test_font_from_path(light_path);
	check(plain != NULL && oblique != NULL && condensed != NULL && light != NULL,
	      "one of the four faces the anchors are measured on could not be loaded");

	/* A HELPER WOULD HIDE THE THREE ASSERTIONS, SO THEY ARE WRITTEN OUT: each face is asked for one entry. */
	if (plain != NULL) {
		CFDictionaryRef t = CTFontCopyTraits(plain);

		got = 99.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontWeightTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got == 0.0,
		      "a REGULAR face's weight is %g, and Apple's anchor says 0.0 is regular or medium", got);
		got = 99.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontWidthTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got == 0.0,
		      "a face at the format's NORMAL width answers %g, and Apple's anchor says 0.0 is regular", got);
		got = 99.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontSlantTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got == 0.0,
		      "an upright face's slant is %g, and Apple's anchor says 0.0 is no rotation", got);
		if (t != NULL) {
			CFRelease(t);
		}
	}
	if (oblique != NULL) {
		CFDictionaryRef t = CTFontCopyTraits(oblique);

		got = 0.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontSlantTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got > 0.0,
		      "an OBLIQUE face's slant is %g, and the format writes its italic angle as a negative number", got);
		check((CTFontGetSymbolicTraits(oblique) & kCTFontItalicTrait) != 0,
		      "the oblique face has no italic trait, so the sign here would prove nothing");
		if (t != NULL) {
			printf("  (note: the oblique face's slant trait is %g, and its angle is %g degrees)\n",
			       got, (double)CTFontGetSlantAngle(oblique));
			CFRelease(t);
		}
	}
	if (condensed != NULL) {
		CFDictionaryRef t = CTFontCopyTraits(condensed);

		got = 0.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontWidthTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got < 0.0,
		      "a CONDENSED face's width is %g, and Apple's anchor says negative values are condensed", got);
		if (t != NULL) {
			CFRelease(t);
		}
	}
	if (light != NULL) {
		CFDictionaryRef t = CTFontCopyTraits(light);

		got = 0.0;
		number = (t != NULL) ? (CFNumberRef)CFDictionaryGetValue(t, kCTFontWeightTrait) : NULL;
		check(number != NULL && CFNumberGetValue(number, kCFNumberDoubleType, &got) && got < 0.0,
		      "a LIGHT face's weight is %g, and a face lighter than regular must be negative", got);
		if (t != NULL) {
			CFRelease(t);
		}
	}
	if (plain != NULL) {
		CFRelease(plain);
	}
	if (oblique != NULL) {
		CFRelease(oblique);
	}
	if (condensed != NULL) {
		CFRelease(condensed);
	}
	if (light != NULL) {
		CFRelease(light);
	}
	CFRelease(bold);
	check(CTFontCopyTraits(NULL) == NULL, "a NULL font produced a traits dictionary");
	return g_fail - before;
}

/* THE SLANT ANGLE, WHICH IS TWO INGREDIENTS ADDED AND A UNIT APPLE NEVER STATES.
 *
 * THE CASE MEASURES ALL THREE THINGS RATHER THAN ONE. The face's own angle is held against the `post` table the case
 * reads itself; THE MATRIX'S SKEW IS HELD AGAINST A MATRIX THE CASE BUILDS OUT OF A TANGENT IT KNOWS -- tan(10
 * degrees) is a ten-degree lean, and no arithmetic in the door can make it something else; AND THE TRANSFORM'S
 * CONVENTION IS MEASURED WITH THE PUBLIC DOOR rather than trusted, since "the image of the vertical is (c, d)" is
 * the sentence the whole skew formula rests on. AND THE SUM IS ASSERTED, on a face that has BOTH ingredients, which
 * is the only way to tell a door that adds them from one that reports whichever it finds.
 *
 * AND THE UNIT IS CROSS-CHECKED AGAINST THE OTHER DOOR THAT SPEAKS OF SLANT: `kCTFontSlantTrait` says 1.0 IS THIRTY
 * DEGREES, so this door's answer must be thirty times that entry -- two doors, one fact, and a factor of fifty-seven
 * would show up here as a failure rather than in a caller's layout. */
static int case_font_slant_angle(void)
{
	int before = g_fail;
	const char *upright_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf";
	const char *oblique_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Oblique.ttf";
	CTFontRef upright, oblique, skewed, skewed_oblique;
	CGAffineTransform lean;
	CFDataRef post;
	const UInt8 *bytes;
	double italic, want;
	CGFloat got;
	CGPoint vertical;

	post = NULL;
	lean = CGAffineTransformIdentity;
	/* A TEN-DEGREE LEAN, EXPRESSED AS THE SHEAR THE TRANSFORM'S OWN `c` FIELD IS: x grows with y by its tangent. */
	lean.c = (CGFloat)0.176326980708465;	/* tan(10 degrees) */

	upright = test_font_from_path(upright_path);
	oblique = test_font_from_path(oblique_path);
	check(upright != NULL && oblique != NULL, "the two faces the slant is measured on could not be loaded");
	if (upright == NULL || oblique == NULL) {
		return g_fail - before;
	}

	/* THE TRANSFORM'S CONVENTION, MEASURED: the vertical direction's image IS (c, d), which is what makes
	 * atan2(c, d) an angle anyone can check. */
	vertical = CGPointApplyAffineTransform(CGPointMake(0.0, 1.0), lean);
	check(fabs((double)vertical.x - (double)lean.c) < 0.000001 && fabs((double)vertical.y - (double)lean.d) < 0.000001,
	      "the transform puts the vertical at (%g, %g) where the shear says (%g, %g)",
	      (double)vertical.x, (double)vertical.y, (double)lean.c, (double)lean.d);

	/* THE FACE'S OWN ANGLE, READ FROM THE TABLE BY THE CASE. */
	post = CTFontCopyTable(oblique, (CTFontTableTag)kCTFontTablePost, kCTFontTableOptionNoOptions);
	check(post != NULL, "the oblique face's `post` table could not be read");
	if (post != NULL) {
		bytes = (const UInt8 *)CFDataGetBytePtr(post);
		check(bytes != NULL && CFDataGetLength(post) >= 8, "`post` is too short for its italic angle");
		if (bytes != NULL && CFDataGetLength(post) >= 8) {
			italic = case_fixed(bytes + 4);
			check(italic != 0.0, "the oblique face's italic angle is 0, so the sign check would prove nothing");
			want = -italic;	/* the table is counter-clockwise, this door is clockwise */
			got = CTFontGetSlantAngle(oblique);
			check(fabs((double)got - want) < 0.0001,
			      "the oblique face's angle is %g where its `post` italic angle %g means %g",
			      (double)got, italic, want);
			printf("  (note: the oblique face's `post` italic angle is %g, and the door answers %g degrees)\n",
			       italic, (double)got);
		}
		CFRelease(post);
	}

	/* AND AN UPRIGHT FACE WITH AN IDENTITY MATRIX IS ZERO -- Apple's own anchor, on the other side. */
	check(CTFontGetSlantAngle(upright) == 0.0, "an upright face under the identity answers %g",
	      (double)CTFontGetSlantAngle(upright));

	/* THE MATRIX'S SKEW ALONE: ten degrees of shear on an upright face. */
	skewed = CTFontCreateCopyWithSymbolicTraits(upright, 0.0, &lean, 0, 0);
	check(skewed != NULL, "the copy under a leaning matrix could not be made");
	if (skewed != NULL) {
		got = CTFontGetSlantAngle(skewed);
		check(fabs((double)got - 10.0) < 0.001, "a ten-degree shear answered %g degrees", (double)got);
		CFRelease(skewed);
	}

	/* AND THE SUM, WHICH IS THE ASSERTION THE WHOLE CASE EXISTS FOR: the oblique face under the same shear carries
	 * BOTH ingredients, and a door that reported one of them would fail here and pass everywhere above. */
	skewed_oblique = CTFontCreateCopyWithSymbolicTraits(oblique, 0.0, &lean, 0, 0);
	check(skewed_oblique != NULL, "the oblique face under a leaning matrix could not be copied");
	if (skewed_oblique != NULL) {
		double face_angle = (double)CTFontGetSlantAngle(oblique);

		got = CTFontGetSlantAngle(skewed_oblique);
		check(fabs((double)got - (face_angle + 10.0)) < 0.001,
		      "the sum is %g where the face's %g plus the matrix's ten is %g",
		      (double)got, face_angle, face_angle + 10.0);
		CFRelease(skewed_oblique);
	}

	/* AND THE TWO DOORS THAT SPEAK OF SLANT AGREE: `kCTFontSlantTrait` states that 1.0 IS thirty degrees. */
	{
		CFDictionaryRef traits = CTFontCopyTraits(oblique);

		if (traits != NULL) {
			CFNumberRef entry = (CFNumberRef)CFDictionaryGetValue(traits, kCTFontSlantTrait);
			double trait = 0.0;

			check(entry != NULL && CFNumberGetValue(entry, kCFNumberDoubleType, &trait),
			      "the oblique face has no slant trait to cross-check against");
			check(fabs(trait * 30.0 - (double)CTFontGetSlantAngle(oblique)) < 0.0001,
			      "the trait says %g (thirty to the degree) where the angle door says %g",
			      trait, (double)CTFontGetSlantAngle(oblique));
			CFRelease(traits);
		}
	}

	check(CTFontGetSlantAngle(NULL) == 0.0, "a NULL font produced an angle");
	CFRelease(upright);
	CFRelease(oblique);
	return g_fail - before;
}

/* THE VERTICAL TRANSLATIONS, WHICH ARE A COMPOSITION OF THREE DOORS AND ARE HELD AS ONE.
 *
 * EVERY INGREDIENT IS ASSERTED SEPARATELY AND THE COMPOSITION IS ASSERTED ON TOP, because a composition that agrees
 * with its parts is the only kind worth having: the height IS the ascent (measured against the ascent door, not
 * against a number), and the width IS half the advance minus the bearing -- both taken from the doors the answer is
 * built from. AND ONE CASE CARRIES NO ARITHMETIC AT ALL: A SPACE HAS NO OUTLINE, so its bounding box has no left
 * side bearing and its offset must be EXACTLY half its advance, which a door that confused the two origins would
 * not manage.
 *
 * AND THE ARRAY IS WRITTEN ONLY WHEN IT IS ASKED FOR: a count of zero, or a NULL array, must leave the caller's
 * memory alone -- asserted by putting a sentinel in and reading it back, which is the only way to see it. */
static int case_font_vertical_origin(void)
{
	int before = g_fail;
	CTFontRef font = test_font(12.0);
	UniChar characters[3];
	CGGlyph glyphs[3];
	CGSize translations[3];
	CGSize advances[3];
	CGRect boxes[3];
	CGFloat ascent;
	int i;

	check(font != NULL, "no font, so there is no vertical origin");
	if (font == NULL) {
		return g_fail - before;
	}
	characters[0] = 'A';
	characters[1] = 'B';
	characters[2] = ' ';
	check(CTFontGetGlyphsForCharacters(font, characters, glyphs, 3),
	      "the face under test does not map A, B and a space, so the offsets cannot be read");
	ascent = CTFontGetAscent(font);
	check(ascent > 0.0, "the face reports an ascent of %g, and a positive one is what the door puts in the height",
	      (double)ascent);

	translations[0] = CGSizeMake(-1.0, -1.0);
	translations[1] = CGSizeMake(-1.0, -1.0);
	translations[2] = CGSizeMake(-1.0, -1.0);
	CTFontGetVerticalTranslationsForGlyphs(font, glyphs, translations, 3);
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, glyphs, advances, 3);
	(void)CTFontGetBoundingRectsForGlyphs(font, kCTFontHorizontalOrientation, glyphs, boxes, 3);

	for (i = 0; i < 3; i++) {
		double want = (double)advances[i].width / 2.0 - (double)boxes[i].origin.x;

		check(fabs((double)translations[i].height - (double)ascent) < 0.0001,
		      "glyph %d's height is %g where the ascent door says %g",
		      i, (double)translations[i].height, (double)ascent);
		check(fabs((double)translations[i].width - want) < 0.0001,
		      "glyph %d's offset is %g where half its advance minus its bearing is %g",
		      i, (double)translations[i].width, want);
		check((double)translations[i].width > 0.0 && (double)translations[i].width <= (double)advances[i].width,
		      "glyph %d's offset (%g) is not inside its own advance (%g)",
		      i, (double)translations[i].width, (double)advances[i].width);
	}
	check(advances[2].width > 0.0, "the space has no advance, so the bearing half cannot be checked");
	check(fabs((double)translations[2].width - (double)advances[2].width / 2.0) < 0.0001,
	      "a SPACE has no outline and therefore no bearing, so its offset must be exactly half its advance: it is %g where half of %g is %g",
	      (double)translations[2].width, (double)advances[2].width, (double)advances[2].width / 2.0);
	printf("  (note: at 12 points the ascent is %g and 'A' sits %g points right of the horizontal origin)\n",
	       (double)ascent, (double)translations[0].width);

	/* AND ONE GLYPH AT A TIME AGREES WITH THREE AT ONCE, which is the range rule this door's neighbours all hold. */
	{
		CGSize one = CGSizeMake(-1.0, -1.0);

		CTFontGetVerticalTranslationsForGlyphs(font, &glyphs[0], &one, 1);
		check(one.width == translations[0].width && one.height == translations[0].height,
		      "asking for one glyph answered (%g, %g) where asking for three answered (%g, %g)",
		      (double)one.width, (double)one.height, (double)translations[0].width,
		      (double)translations[0].height);
	}

	/* AND NOTHING IS WRITTEN WHEN THERE IS NOTHING TO WRITE. */
	translations[0] = CGSizeMake(-7.5, -7.5);
	CTFontGetVerticalTranslationsForGlyphs(font, glyphs, translations, 0);
	check(translations[0].width == -7.5 && translations[0].height == -7.5,
	      "a count of zero wrote to the caller's array");
	CTFontGetVerticalTranslationsForGlyphs(font, glyphs, translations, -1);
	check(translations[0].width == -7.5 && translations[0].height == -7.5,
	      "a NEGATIVE count wrote to the caller's array");
	CTFontGetVerticalTranslationsForGlyphs(NULL, glyphs, translations, 3);
	check(translations[0].width == -7.5 && translations[0].height == -7.5,
	      "a NULL font wrote to the caller's array");
	CTFontGetVerticalTranslationsForGlyphs(font, NULL, translations, 3);
	check(translations[0].width == -7.5 && translations[0].height == -7.5,
	      "a NULL glyph array wrote to the caller's array");
	CTFontGetVerticalTranslationsForGlyphs(font, glyphs, NULL, 3);
	check(1, "a NULL output array is a no-op rather than a crash");

	CFRelease(font);
	return g_fail - before;
}


/* ============================================================================================ *
 * A LINE IS KERNED, WHICH IS THE DIFFERENCE BETWEEN SPACING AND TYPESETTING.                     *
 *                                                                                                *
 * An ADVANCE is a glyph's own opinion of how far the pen moves; a KERN is the font's opinion      *
 * about a PAIR — that an `o` belongs closer under a `T` than the two advances put it — and a line *
 * that sums advances and stops has thrown that opinion away. MEASURED, DejaVu Sans asks for `To`  *
 * to close by 348 units of 2048, an em's sixth: TWO PIXELS of daylight at 13 points, the width of *
 * a stem.                                                                                         *
 *                                                                                                *
 * THE TWO FACES ARE THE TWO ANSWERS. `test_font` is DejaVu Sans MONO Bold, which has NO `kern`    *
 * table at all — so it holds the case where the adjustment must be zero and the layout must be    *
 * the plain sum — and a proportional face holds the case where it is not. A case with only the    *
 * second would pass on a face with no kerning; one with only the first would pass on a line that  *
 * never kerns.                                                                                    *
 * ============================================================================================ */
static int case_line_kerning(void)
{
	int before = g_fail;
	CTFontRef mono = test_font(13.0);
	CTFontRef prop = CTFontCreateWithName(CFSTR("DejaVu Sans"), 13.0, NULL);
	UniChar t = 'T', o = 'o';
	CGGlyph tg[2], og[2];
	CGSize tadv[2];
	double kern, mono_kern, tadvance;
	int defined;

	check(mono != NULL, "no monospace face, so the no-kerning half cannot be asked");
	if (mono == NULL) {
		return g_fail - before;
	}
	memset(tg, 0, sizeof tg);
	memset(og, 0, sizeof og);
	defined = CTFontGetGlyphsForCharacters(mono, &t, tg, 1) && CTFontGetGlyphsForCharacters(mono, &o, og, 1);
	check(defined, "the monospace face has no glyph for 'T' or 'o', so nothing can be laid out");
	if (!defined) {
		return g_fail - before;
	}

	/* **THE FACE WITH NO KERN TABLE, WHICH IS A RESULT AND NOT AN ABSENCE**: the door answers zero for every pair
	 * rather than reaching the engine for a table that is not there. */
	mono_kern = CTFontGetKerningBetweenGlyphs(mono, tg[0], og[0]);
	check(mono_kern == 0.0, "a face with no kern table answered %g for 'To'", mono_kern);

	check(prop != NULL, "no proportional face resolved, so the kerned half cannot be asked");
	if (prop == NULL) {
		return g_fail - before;
	}
	memset(tg, 0, sizeof tg);
	memset(og, 0, sizeof og);
	defined = CTFontGetGlyphsForCharacters(prop, &t, tg, 1)
		  && CTFontGetGlyphsForCharacters(prop, &o, og, 1);
	check(defined, "the proportional face has no glyph for 'T' or 'o'");
	if (!defined) {
		CFRelease(prop);
		return g_fail - before;
	}
	kern = CTFontGetKerningBetweenGlyphs(prop, tg[0], og[0]);
	tadvance = CTFontGetAdvancesForGlyphs(prop, kCTFontHorizontalOrientation, tg, tadv, 1);
	printf("  (DejaVu Sans asks for 'To' to close by %.3f points at 13; 'T' advances %.3f)\n",
	       kern, tadvance);
	check(kern < 0.0, "this face does not kern 'To' (%g), so the layout cannot be judged", kern);

	{
		CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
									 &kCFTypeDictionaryKeyCallBacks,
									 &kCFTypeDictionaryValueCallBacks);
		CFAttributedStringRef astr;
		CTLineRef line;

		if (attrs != NULL) {
			CFDictionarySetValue(attrs, kCTFontAttributeName, prop);
		}
		astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("To"), attrs);
		line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
		check(line != NULL, "the line 'To' could not be made");
		if (line != NULL) {
			CFArrayRef runs = CTLineGetGlyphRuns(line);
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;

			check(run != NULL && CTRunGetGlyphCount(run) == 2, "the run is not two glyphs");
			if (run != NULL && CTRunGetGlyphCount(run) == 2) {
				CGPoint pos[2];
				double width;

				pos[0] = CGPointMake(0.0, 0.0);
				pos[1] = CGPointMake(0.0, 0.0);
				CTRunGetPositions(run, CFRangeMake(0, 2), pos);
				check(fabs(pos[1].x - (tadvance + kern)) < 0.01,
				      "'o' is placed %.3f from the pen, want %.3f — the advance %.3f plus the kern %.3f",
				      (double)pos[1].x, (double)(tadvance + kern), tadvance, kern);
				check(pos[1].x < tadvance - 0.5,
				      "'o' is %.3f from the pen, the ADVANCE alone (%.3f): the kern was not applied",
				      (double)pos[1].x, tadvance);
				width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
				check(fabs(width - (tadvance + kern
						    + CTFontGetAdvancesForGlyphs(prop, kCTFontHorizontalOrientation, og,
										 tadv, 1))) < 0.01,
				      "the line measures %.3f, so the kern did not reach its WIDTH", width);
			}
			CFRelease(line);
		}
		if (astr != NULL) {
			CFRelease(astr);
		}
		if (attrs != NULL) {
			CFRelease(attrs);
		}
	}

	/* **AND A PAIR THE FACE DOES NOT ADJUST**, so the check above is a measurement of the KERN and not of a layout
	 * that always shortens the line. */
	{
		UniChar h = 'h';
		CGGlyph hg[2];

		memset(hg, 0, sizeof hg);
		if (CTFontGetGlyphsForCharacters(prop, &h, hg, 1)) {
			double hkern = CTFontGetKerningBetweenGlyphs(prop, tg[0], hg[0]);

			if (hkern == 0.0) {
				CFMutableDictionaryRef a2 = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
										      &kCFTypeDictionaryKeyCallBacks,
										      &kCFTypeDictionaryValueCallBacks);
				CFAttributedStringRef s2;
				CTLineRef l2;

				if (a2 != NULL) {
					CFDictionarySetValue(a2, kCTFontAttributeName, prop);
				}
				s2 = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("Th"), a2);
				l2 = (s2 != NULL) ? CTLineCreateWithAttributedString(s2) : NULL;
				if (l2 != NULL) {
					CFArrayRef r2 = CTLineGetGlyphRuns(l2);

					if (r2 != NULL && CFArrayGetCount(r2) > 0) {
						CGPoint p2[2];

						p2[0] = CGPointMake(0.0, 0.0);
						p2[1] = CGPointMake(0.0, 0.0);
						CTRunGetPositions((CTRunRef)CFArrayGetValueAtIndex(r2, 0),
								  CFRangeMake(0, 2), p2);
						check(fabs(p2[1].x - tadvance) < 0.01,
						      "'h' after 'T' is placed %.3f from the pen, want the advance alone (%.3f)",
						      (double)p2[1].x, tadvance);
					}
					CFRelease(l2);
				}
				if (s2 != NULL) {
					CFRelease(s2);
				}
				if (a2 != NULL) {
					CFRelease(a2);
				}
			} else {
				printf("  (note: 'Th' is kerned by %g too, so the unkerned pair was not asked)\n", hkern);
			}
		}
	}
	CFRelease(prop);
	return g_fail - before;
}


/* **AND THE FRAMESETTER PATH, WHICH IS WHERE THE APPKIT'S TEXT GOES.** `-drawInRect:withAttributes:` makes a FRAME,
 * not a line — that is what gives it wrapping — and a frame's lines come from the typesetter, which makes them
 * through a door no public header names. So there are two ways into a line and only one of them is public, and a
 * kerning change that reached `CTLineCreateWithAttributedString` and not this one would leave every label, button
 * and menu item in the AppKit unkerned while a direct CTLine was kerned. That split is what this asks about.
 */
static int case_frame_kerning(void)
{
	int before = g_fail;
	CTFontRef prop = CTFontCreateWithName(CFSTR("DejaVu Sans"), 13.0, NULL);
	UniChar t = 'T', o = 'o';
	CGGlyph tg[2], og[2];
	CGSize tadv[2];
	double kern, advance;
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr;
	CTFramesetterRef framesetter;
	CGMutablePathRef path;
	CTFrameRef frame;
	CFArrayRef lines;

	check(prop != NULL, "no proportional face, so the framesetter path cannot be asked");
	if (prop == NULL) {
		return g_fail - before;
	}
	memset(tg, 0, sizeof tg);
	memset(og, 0, sizeof og);
	if (!CTFontGetGlyphsForCharacters(prop, &t, tg, 1)
	    || !CTFontGetGlyphsForCharacters(prop, &o, og, 1)) {
		CFRelease(prop);
		return g_fail - before;
	}
	kern = CTFontGetKerningBetweenGlyphs(prop, tg[0], og[0]);
	advance = CTFontGetAdvancesForGlyphs(prop, kCTFontHorizontalOrientation, tg, tadv, 1);

	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	if (attrs != NULL) {
		CFDictionarySetValue(attrs, kCTFontAttributeName, prop);
	}
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("Toggle the Checkbox"), attrs);
	framesetter = (astr != NULL) ? CTFramesetterCreateWithAttributedString(astr) : NULL;
	check(framesetter != NULL, "the framesetter would not take the string");
	if (framesetter == NULL) {
		CFRelease(prop);
		return g_fail - before;
	}
	path = CGPathCreateMutable();
	CGPathAddRect(path, NULL, CGRectMake(0.0, 0.0, 400.0, 20.0));
	frame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, 0), path, NULL);
	lines = (frame != NULL) ? CTFrameGetLines(frame) : NULL;
	check(lines != NULL && CFArrayGetCount(lines) > 0, "the frame has no lines");
	if (lines != NULL && CFArrayGetCount(lines) > 0) {
		CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(lines, 0);
		CFArrayRef runs = CTLineGetGlyphRuns(line);

		check(runs != NULL && CFArrayGetCount(runs) > 0, "the line has no runs");
		if (runs != NULL && CFArrayGetCount(runs) > 0) {
			CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, 0);
			CGPoint pos[2];

			pos[0] = CGPointMake(0.0, 0.0);
			pos[1] = CGPointMake(0.0, 0.0);
			CTRunGetPositions(run, CFRangeMake(0, 2), pos);
			check(fabs(pos[1].x - (advance + kern)) < 0.01,
			      "the FRAME path places 'o' at %.3f, want %.3f — the advance %.3f plus the kern %.3f, so "
			      "the AppKit's own text is not kerned", (double)pos[1].x, (double)(advance + kern),
			      advance, kern);
		}
	}
	CGPathRelease(path);
	CFRelease(framesetter);
	if (astr != NULL) {
		CFRelease(astr);
	}
	if (attrs != NULL) {
		CFRelease(attrs);
	}
	CFRelease(prop);
	return g_fail - before;
}

/* **A RUN DELEGATE IS A BOX: A CHARACTER THAT OCCUPIES WIDTH AND DRAWS NO INK.** The three callbacks answer for a
 * reference constant, the delegate is set as the value of `kCTRunDelegateAttributeName` over the character, and the
 * line reserves the delegate's width instead of the placeholder glyph's advance and grows to cover its ascent and
 * descent. So the probe sets one over the middle of `abXc` and reads the offsets back: the box's own glyph count, the
 * distance from the box to the next character (the delegate's width, not the glyph's), and the two heights. */
struct fn_box { CGFloat width, ascent, descent; };
static CGFloat fn_box_width(void *refCon) { return ((const struct fn_box *)refCon)->width; }
static CGFloat fn_box_ascent(void *refCon) { return ((const struct fn_box *)refCon)->ascent; }
static CGFloat fn_box_descent(void *refCon) { return ((const struct fn_box *)refCon)->descent; }

static int case_run_delegate(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	struct fn_box box = { 30.0, 12.0, 3.0 };
	CTRunDelegateCallbacks callbacks = { 0, fn_box_ascent, fn_box_descent, fn_box_width };
	CTRunDelegateRef delegate;
	UniChar chars[4];
	CFStringRef text;
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef base;
	CFMutableAttributedStringRef astr;
	CTLineRef line;

	check(font != NULL, "no face, so the box cannot be judged against one");
	if (font == NULL) {
		return g_fail - before;
	}
	delegate = CTRunDelegateCreate(&callbacks, &box);
	check(delegate != NULL && CTRunDelegateGetRefCon(delegate) == &box,
	      "`CTRunDelegateCreate` makes one and keeps the reference constant");

	chars[0] = 'a'; chars[1] = 'b'; chars[2] = 0xFFFC; chars[3] = 'c';
	text = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 4);
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	base = CFAttributedStringCreate(kCFAllocatorDefault, text, attrs);
	astr = CFAttributedStringCreateMutableCopy(kCFAllocatorDefault, 0, base);
	CFAttributedStringSetAttribute(astr, CFRangeMake(2, 1), kCTRunDelegateAttributeName, delegate);
	line = CTLineCreateWithAttributedString(astr);
	check(line != NULL, "the line with a box could not be made");

	if (line != NULL) {
		CGFloat boxX = CTLineGetOffsetForStringIndex(line, 2, NULL);
		CGFloat afterX = CTLineGetOffsetForStringIndex(line, 3, NULL);
		CGFloat ascent = 0.0, descent = 0.0;
		CFArrayRef runs = CTLineGetGlyphRuns(line);
		CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
			? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;

		(void)CTLineGetTypographicBounds(line, &ascent, &descent, NULL);
		printf("  (box at x=%.3f, next at x=%.3f, ascent %.3f, descent %.3f)\n",
		       (double)boxX, (double)afterX, (double)ascent, (double)descent);
		check(CTLineGetGlyphCount(line) == 4, "the line has one entry per character, box included");
		check(fabs((afterX - boxX) - 30.0) < 0.01,
		      "the character after the box is the DELEGATE's width past it, not the glyph's advance");
		check(ascent >= 12.0 - 0.01, "the line's ascent grew to cover the box's");
		check(descent >= 3.0 - 0.01, "and its descent too");
		check(run != NULL && CTRunGetGlyphCount(run) == 4, "the run agrees: one glyph per character");
	}

	/* **AND THE TYPESETTER MEASURES THE BOX TOO**, so a line breaks where the box actually ends rather than where its
	 * placeholder glyph did. At 40 points, `a` and `b` fit but the box's THIRTY does not, so the fit stops before it —
	 * where a placeholder advance would have left room for the box AND `c`. */
	{
		CTTypesetterRef ts = CTTypesetterCreateWithAttributedString(astr);
		CFIndex count = (ts != NULL) ? CTTypesetterSuggestLineBreak(ts, 0, 40.0) : 0;

		printf("  (a 40-point width broke after %ld character(s))\n", (long)count);
		check(ts != NULL && count == 2,
		      "the typesetter measured the box's 30 points and stopped before it");
		if (ts != NULL) {
			CFRelease(ts);
		}
	}

	if (line != NULL) {
		CFRelease(line);
	}
	CFRelease(astr);
	CFRelease(base);
	CFRelease(text);
	CFRelease(attrs);
	CFRelease(delegate);
	return g_fail - before;
}

/* **A TAB LANDS AT ITS STOP, WHICH IS A PLACE RATHER THAN A WIDTH.** The paragraph style carries `CTTextTab`s, and a
 * `\t` character advances to the first stop past the pen — no ink, like a run-delegate box, but the width is decided
 * while the positions are walked. So the probe sets stops at 50 and 100 over `a\tb\tc` and reads the offsets back: the
 * character after a tab begins AT the stop, and the fit breaks where the tab actually takes the pen. */
static int case_text_tab_stops(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	CFMutableArrayRef tabs = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	CTTextTabRef t50 = CTTextTabCreate(kCTLeftTextAlignment, 50.0, NULL);
	CTTextTabRef t100 = CTTextTabCreate(kCTLeftTextAlignment, 100.0, NULL);
	CTParagraphStyleSetting setting;
	CTParagraphStyleRef style;
	CFMutableDictionaryRef attrs;
	CFAttributedStringRef astr;
	CTLineRef line;

	check(font != NULL && tabs != NULL && t50 != NULL && t100 != NULL, "the tab stops could not be made");
	if (font == NULL || tabs == NULL || t50 == NULL || t100 == NULL) {
		return g_fail - before;
	}
	CFArrayAppendValue(tabs, t50);
	CFArrayAppendValue(tabs, t100);
	setting.spec = kCTParagraphStyleSpecifierTabStops;
	setting.valueSize = sizeof(CFArrayRef);
	setting.value = &tabs;
	style = CTParagraphStyleCreate(&setting, 1);
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	CFDictionarySetValue(attrs, kCTParagraphStyleAttributeName, style);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("a\tb\tc"), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	check(line != NULL, "the line with tabs could not be made");
	if (line != NULL) {
		CGFloat bX = CTLineGetOffsetForStringIndex(line, 2, NULL);
		CGFloat cX = CTLineGetOffsetForStringIndex(line, 4, NULL);
		CGFloat tabX = CTLineGetOffsetForStringIndex(line, 1, NULL);

		printf("  (tab at x=%.3f, `b` at x=%.3f, `c` at x=%.3f)\n", (double)tabX, (double)bX, (double)cX);
		check(CTLineGetGlyphCount(line) == 5, "the line keeps one entry per character, tabs included");
		check(fabs(bX - 50.0) < 0.01, "the character after the first tab begins AT the 50-point stop");
		check(fabs(cX - 100.0) < 0.01, "and the one after the second at the 100-point stop");
	}
	{
		CTTypesetterRef ts = (astr != NULL) ? CTTypesetterCreateWithAttributedString(astr) : NULL;
		CFIndex count = (ts != NULL) ? CTTypesetterSuggestLineBreak(ts, 0, 60.0) : 0;

		/* THE FIT REACHED THE FIRST TAB AND THE WORD-WRAP BACKOFF STOPPED THERE, which is the answer the stops drive:
		 * a tab is whitespace, so word wrapping may break at it — and WITHOUT the stops the fit would have run on to
		 * the last tab and answered 4. */
		printf("  (a 60-point width broke after %ld character(s))\n", (long)count);
		check(ts != NULL && count == 2,
		      "the fit followed the first tab to its stop and the word backoff stopped there");
		if (ts != NULL) {
			CFRelease(ts);
		}
	}
	if (line != NULL) {
		CFRelease(line);
	}
	CFRelease(astr);
	CFRelease(attrs);
	CFRelease(style);
	CFRelease(t100);
	CFRelease(t50);
	CFRelease(tabs);
	return g_fail - before;
}

/* **AND THE BASE WRITING DIRECTION, WHICH THE RUN REPORTS.** A paragraph's direction is read at the line's first
 * character and handed to the run, so a right-to-left paragraph makes a run that says so — the FIRST piece of the
 * bidi milestone, with the REORDERING (the Unicode Bidirectional Algorithm) named as the next slice. */
/* **THE GLYPH A RUN PUTS A CHARACTER IN, WHICH IS NOW A QUESTION FOR THE RUN'S STRING INDICES RATHER THAN AN
 * INDEX.** A shaped run's glyphs are in the SHAPER's order — a right-to-left run comes back in VISUAL order, so its
 * last glyph is the first character's — and reading `glyphs[i]` for character `i` was only right while a glyph WAS a
 * character. The run's own string indices are the map, so this asks them; a character that shares a glyph with the one
 * before it (a ligature, a mark) answers with that glyph, which is the same rule the caret doors use. */
static CGGlyph glyph_for_character(CTRunRef run, CFIndex character)
{
	const CFIndex *indices = (run != NULL) ? CTRunGetStringIndicesPtr(run) : NULL;
	const CGGlyph *glyphs = (run != NULL) ? CTRunGetGlyphsPtr(run) : NULL;
	CFIndex i, count = (run != NULL) ? CTRunGetGlyphCount(run) : 0;

	if (indices == NULL || glyphs == NULL) {
		return 0;
	}
	for (i = 0; i < count; i++) {
		if (indices[i] == character) {
			return glyphs[i];
		}
	}
	return 0;
}

static int case_base_writing_direction(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	CTParagraphStyleSetting setting;
	CTWritingDirection value = kCTWritingDirectionRightToLeft;
	CTParagraphStyleRef rtl = NULL;
	CTParagraphStyleRef ltr = NULL;
	CFMutableDictionaryRef attrs = NULL;
	CFAttributedStringRef astr = NULL;
	CTLineRef line = NULL;
	CFArrayRef runs;
	CTRunRef run;
	CTRunStatus status;

	setting.spec = kCTParagraphStyleSpecifierBaseWritingDirection;
	setting.valueSize = sizeof value;
	setting.value = &value;
	rtl = CTParagraphStyleCreate(&setting, 1);
	value = kCTWritingDirectionLeftToRight;
	setting.value = &value;
	ltr = CTParagraphStyleCreate(&setting, 1);
	check(font != NULL && rtl != NULL && ltr != NULL, "the writing-direction styles could not be made");
	if (font == NULL || rtl == NULL || ltr == NULL) {
		if (ltr != NULL) CFRelease(ltr);
		if (rtl != NULL) CFRelease(rtl);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);

	/* A RIGHT-TO-LEFT PARAGRAPH: the run reports the flag, and NOTHING has moved yet. */
	CFDictionarySetValue(attrs, kCTParagraphStyleAttributeName, rtl);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("abc"), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
	run = (runs != NULL && CFArrayGetCount(runs) > 0) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
	status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;
	printf("  (a right-to-left paragraph's run reports status %u)\n", (unsigned)status);
	check(run != NULL && status == kCTRunStatusRightToLeft,
	      "a right-to-left paragraph's run reports kCTRunStatusRightToLeft");
	check((status & kCTRunStatusNonMonotonic) == 0,
	      "and nothing was REORDERED yet, so the non-monotonic flag stays clear");
	if (line != NULL) CFRelease(line);
	if (astr != NULL) CFRelease(astr);

	/* A LEFT-TO-RIGHT PARAGRAPH: the tier's ordinary answer, no status. */
	CFDictionarySetValue(attrs, kCTParagraphStyleAttributeName, ltr);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("abc"), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
	run = (runs != NULL && CFArrayGetCount(runs) > 0) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
	check(run != NULL && CTRunGetStatus(run) == kCTRunStatusNoStatus,
	      "a left-to-right paragraph's run reports no status");
	if (line != NULL) CFRelease(line);
	if (astr != NULL) CFRelease(astr);

	/* AND A NATURAL DIRECTION IS THE TIER'S DEFAULT — LEFT TO RIGHT — until the algorithm is here. */
	CFDictionaryRemoveValue(attrs, kCTParagraphStyleAttributeName);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("abc"), attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
	run = (runs != NULL && CFArrayGetCount(runs) > 0) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
	check(run != NULL && CTRunGetStatus(run) == kCTRunStatusNoStatus,
	      "and a NATURAL direction is the tier's default — left to right");
	if (line != NULL) CFRelease(line);
	if (astr != NULL) CFRelease(astr);

	CFRelease(attrs);
	CFRelease(ltr);
	CFRelease(rtl);
	CFRelease(font);
	return g_fail - before;
}

/* **AND THE BIDI DATA AND THE PARAGRAPH LEVEL — UAX #9 P2 AND P3.** The class of a code point comes from the
 * generated table, and the paragraph's level from the FIRST STRONG character while stepping over isolate runs. It is
 * the whole of the algorithm built so far: the X/W/N/I rules and the L1/L2 reordering are the next slices. */
static int case_bidi_paragraph_level(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	CFMutableDictionaryRef attrs = NULL;
	CFAttributedStringRef astr = NULL;
	CTLineRef line = NULL;
	CFArrayRef runs;
	CTRunRef run;

	/* THE CLASSES, from the generated table. */
	check(ct_bidi_class('a') == kCTBidiL, "a Latin letter is L");
	check(ct_bidi_class(0x05D0) == kCTBidiR, "a Hebrew letter is R");
	check(ct_bidi_class(0x0627) == kCTBidiAL, "an Arabic letter is AL");
	check(ct_bidi_class('0') == kCTBidiEN, "a digit is EN");
	check(ct_bidi_class(' ') == kCTBidiWS, "a space is WS");
	check(ct_bidi_class(0x2066) == kCTBidiLRI && ct_bidi_class(0x2069) == kCTBidiPDI,
	      "an isolate initiator and its PDI are their own classes");
	check(ct_bidi_class(0x10FFFF) == kCTBidiL, "a code point the table does not name is L");

	/* P2 AND P3. */
	{
		UniChar latin[] = { 'a', 'b', 'c' };
		UniChar hebrew[] = { 0x05D0, 0x05D1, 0x05D2 };
		UniChar arabic[] = { 0x0627, 0x0628 };
		UniChar latinFirst[] = { 'a', ' ', 0x05D0 };
		UniChar rtlFirst[] = { 0x05D0, ' ', 'a' };
		UniChar digits[] = { '1', '2', '3' };
		UniChar isolated[] = { 0x2066, 0x05D0, 0x2069, 'a' };

		check(ct_bidi_paragraph_level(latin, 3, 0) == 0, "a Latin paragraph is level 0");
		check(ct_bidi_paragraph_level(hebrew, 3, 0) == 1, "a Hebrew paragraph is level 1");
		check(ct_bidi_paragraph_level(arabic, 2, 0) == 1, "an Arabic (AL) paragraph is level 1");
		check(ct_bidi_paragraph_level(latinFirst, 3, 1) == 0,
		      "the FIRST strong character decides: Latin first is level 0");
		check(ct_bidi_paragraph_level(rtlFirst, 3, 0) == 1, "and a right-to-left strong character first is level 1");
		check(ct_bidi_paragraph_level(digits, 3, 0) == 0 && ct_bidi_paragraph_level(digits, 3, 1) == 1,
		      "a paragraph with NO strong character answers the caller's fallback");
		check(ct_bidi_paragraph_level(isolated, 4, 1) == 0,
		      "P2 STEPS OVER an isolate run, so the `a` after the PDI decides");
	}

	/* AND THE LINE RESOLVES A NATURAL DIRECTION BY P2/P3, which the run then reports. */
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	{
		UniChar hebrew[] = { 0x05D0, 0x05D1, 0x05D2 };
		CFStringRef h = CFStringCreateWithCharacters(kCFAllocatorDefault, hebrew, 3);

		astr = (h != NULL) ? CFAttributedStringCreate(kCFAllocatorDefault, h, attrs) : NULL;
		line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
		runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
		run = (runs != NULL && CFArrayGetCount(runs) > 0) ? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
		printf("  (a NATURAL paragraph of Hebrew reports status %u)\n",
		       (unsigned)((run != NULL) ? CTRunGetStatus(run) : 0));
		check(run != NULL && (CTRunGetStatus(run) & kCTRunStatusRightToLeft) != 0,
		      "a NATURAL paragraph of Hebrew RESOLVES to right to left (P2/P3)");
		check(run != NULL && (CTRunGetStatus(run) & kCTRunStatusNonMonotonic) != 0,
		      "and its glyphs are REORDERED now, so the run also reports NonMonotonic");
		if (line != NULL) CFRelease(line);
		if (astr != NULL) CFRelease(astr);
		if (h != NULL) CFRelease(h);
	}
	CFRelease(attrs);
	CFRelease(font);
	return g_fail - before;
}

/* **AND THE EXPLICIT LEVELS — UAX #9 X1-X8.** An embedding, an override or an isolate changes the level for what
 * follows, so the walk keeps a STACK; the probe reads the level of the ORDINARY characters (the controls' own levels
 * do not matter, because X9 removes them) and the override's effect on a class. */
static int case_bidi_explicit_levels(void)
{
	int before = g_fail;
	uint8_t levels[16];
	CTBidiClass classes[16];

	/* RLE ... PDF: the level rises to 1 and the PDF brings it back, and both controls are REMOVED. */
	{
		UniChar text[] = { 'a', 0x202B, 'b', 0x202C, 'c' };

		check(ct_bidi_explicit_levels(text, 5, 0, levels, classes), "the explicit walk runs");
		check(levels[0] == 0 && levels[2] == 1 && levels[4] == 0,
		      "an RLE raises the level to 1 and the PDF brings it back");
		check(classes[1] == kCTBidiBN && classes[3] == kCTBidiBN, "and the two controls are REMOVED (BN)");
		check(classes[2] == kCTBidiL, "while the character inside keeps its class (no override)");
	}
	/* RLO: the level rises AND the class is overridden. */
	{
		UniChar text[] = { 0x202E, 'a', 0x202C };

		check(ct_bidi_explicit_levels(text, 3, 0, levels, classes), "the override walk runs");
		check(levels[1] == 1, "an RLO raises the level to 1");
		check(classes[1] == kCTBidiR, "and OVERRIDES the character's class to R");
	}
	/* LRE inside a RIGHT-TO-LEFT paragraph: the next even level above 1 is 2. */
	{
		UniChar text[] = { 0x202A, 'a', 0x202C };

		check(ct_bidi_explicit_levels(text, 3, 1, levels, classes), "the LRE walk runs");
		check(levels[1] == 2, "an LRE inside a right-to-left paragraph raises the level to 2");
	}
	/* LRI ... PDI: an ISOLATE is not an embedding — nothing leaks out of it. */
	{
		UniChar text[] = { 0x2066, 'a', 0x2069, 'b' };

		check(ct_bidi_explicit_levels(text, 4, 0, levels, classes), "the isolate walk runs");
		check(levels[1] == 2, "an LRI raises the level to 2 INSIDE the isolate");
		check(levels[3] == 0, "and the PDI brings it back for what FOLLOWS");
	}
	/* RLI: the least odd level above 0. */
	{
		UniChar text[] = { 0x2067, 'a', 0x2069 };

		check(ct_bidi_explicit_levels(text, 3, 0, levels, classes), "the RLI walk runs");
		check(levels[1] == 1, "an RLI raises the level to 1 inside the isolate");
	}
	/* AN FSI IS X5c: its own P2/P3 decides which of the two it is. */
	{
		UniChar hebrew[] = { 0x2068, 0x05D0, 0x2069 };
		UniChar latin[] = { 0x2068, 'a', 0x2069 };

		check(ct_bidi_explicit_levels(hebrew, 3, 0, levels, classes), "the FSI walk runs (Hebrew)");
		check(levels[1] == 1, "an FSI whose text is Hebrew isolates it RIGHT to left");
		check(ct_bidi_explicit_levels(latin, 3, 0, levels, classes), "the FSI walk runs (Latin)");
		check(levels[1] == 2, "and one whose text is Latin isolates it LEFT to right");
	}
	return g_fail - before;
}

/* **AND THE WEAK TYPES — UAX #9 W1-W7, over the level runs.** The probe builds the levels and classes with the
 * explicit phase, runs the weak rules, and reads the CLASSES back: the marks, numbers and letters below are chosen so
 * that each rule is the one that changes the answer. */
static int case_bidi_weak_types(void)
{
	int before = g_fail;
	uint8_t levels[16];
	CTBidiClass classes[16];

	/* W1: a NON-SPACING MARK takes the type of what it follows. */
	{
		UniChar text[] = { 'a', 0x0301 };

		check(ct_bidi_explicit_levels(text, 2, 0, levels, classes), "the weak walk runs");
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		check(classes[1] == kCTBidiL, "W1: a combining mark takes the type it follows");
	}
	/* W3: AL becomes R. */
	{
		UniChar text[] = { 0x0627 };

		ct_bidi_explicit_levels(text, 1, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 1, 0);
		check(classes[0] == kCTBidiR, "W3: an Arabic letter becomes R");
	}
	/* W2: a European number after an Arabic letter becomes an ARABIC number. */
	{
		UniChar text[] = { 0x0627, '1' };

		ct_bidi_explicit_levels(text, 2, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		check(classes[1] == kCTBidiAN, "W2: a European number after an Arabic letter becomes an ARABIC number");
	}
	/* W7: a European number after a Latin strong type becomes L. */
	{
		UniChar text[] = { 'a', '1' };

		ct_bidi_explicit_levels(text, 2, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		check(classes[1] == kCTBidiL, "W7: a European number after a Latin strong type becomes L");
	}
	/* W4, W5 AND W6 in a RIGHT-TO-LEFT paragraph, where the strong type is R and W7 must NOT fire. */
	{
		UniChar w4[] = { 0x05D0, '1', '+', '2' };
		UniChar w5[] = { 0x05D0, '1', '$', '2' };
		UniChar w6[] = { 0x05D0, '$' };

		ct_bidi_explicit_levels(w4, 4, 1, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 4, 1);
		check(classes[1] == kCTBidiEN && classes[2] == kCTBidiEN && classes[3] == kCTBidiEN,
		      "W4: a separator between two European numbers becomes one (and W7 does not fire under R)");
		ct_bidi_explicit_levels(w5, 4, 1, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 4, 1);
		check(classes[2] == kCTBidiEN, "W5: a European terminator ADJACENT to a number becomes one");
		ct_bidi_explicit_levels(w6, 2, 1, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 2, 1);
		check(classes[1] == kCTBidiON, "W6: what is left of the terminators is OTHER NEUTRAL");
	}
	return g_fail - before;
}

/* **AND THE NEUTRALS — UAX #9 N1 AND N2.** A neutral between two strong types of the SAME direction takes that
 * direction; anything else takes the EMBEDDING direction of its run. A NUMBER counts as R, which is the case that
 * separates N1 from a plain "same as the left" reading. */
static int case_bidi_neutrals(void)
{
	int before = g_fail;
	uint8_t levels[16];
	CTBidiClass classes[16];

	/* N1: L ... L. */
	{
		UniChar text[] = { 'a', ' ', 'b' };

		check(ct_bidi_explicit_levels(text, 3, 0, levels, classes), "the neutral walk runs");
		ct_bidi_resolve_weak(classes, levels, 3, 0);
		ct_bidi_resolve_neutral(classes, levels, 3, 0);
		check(classes[1] == kCTBidiL, "N1: a neutral between two Latin strong types becomes L");
	}
	/* N1: R ... R. */
	{
		UniChar text[] = { 0x05D0, ' ', 0x05D1 };

		ct_bidi_explicit_levels(text, 3, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 3, 0);
		ct_bidi_resolve_neutral(classes, levels, 3, 0);
		check(classes[1] == kCTBidiR, "N1: a neutral between two Hebrew strong types becomes R");
	}
	/* N1: a NUMBER counts as R. */
	{
		UniChar text[] = { 0x05D0, ' ', '1' };

		ct_bidi_explicit_levels(text, 3, 1, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 3, 1);
		ct_bidi_resolve_neutral(classes, levels, 3, 1);
		check(classes[1] == kCTBidiR, "N1: a neutral beside a NUMBER and R becomes R (a number influences as R)");
	}
	/* N2: the sides disagree, so the EMBEDDING direction — R in a right-to-left paragraph. */
	{
		UniChar text[] = { 'a', ' ', 0x05D0 };

		ct_bidi_explicit_levels(text, 3, 1, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 3, 1);
		ct_bidi_resolve_neutral(classes, levels, 3, 1);
		check(classes[1] == kCTBidiR, "N2: a neutral whose sides DISAGREE takes the embedding direction (R here)");
	}
	/* N2: the same text in a left-to-right paragraph takes L. */
	{
		UniChar text[] = { 'a', ' ', 0x05D0 };

		ct_bidi_explicit_levels(text, 3, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 3, 0);
		ct_bidi_resolve_neutral(classes, levels, 3, 0);
		check(classes[1] == kCTBidiL, "and the same text in a left-to-right paragraph takes L");
	}
	return g_fail - before;
}

/* **AND X10: THE ISOLATING RUN SEQUENCES.** `R LRI a PDI R` — the isolate initiator and its PDI are on the OUTSIDE
 * level, and X10 joins the runs they sit in into ONE sequence, so those two neutrals see the R on BOTH sides and N1
 * makes them R. Without the join each would see only `sos`/`eos` — L, from the level-2 content between them — and N2
 * would make them L, which is what makes this the case that MEASURES the join. */
static int case_bidi_isolating_sequences(void)
{
	int before = g_fail;
	uint8_t levels[16];
	CTBidiClass classes[16];
	UniChar text[] = { 0x05D0, 0x2066, 'a', 0x2069, 0x05D1 };

	check(ct_bidi_explicit_levels(text, 5, 0, levels, classes), "the isolating walk runs");
	check(levels[2] == 2, "the isolate's content has its OWN level, from X5b");
	ct_bidi_resolve_weak(classes, levels, 5, 0);
	ct_bidi_resolve_neutral(classes, levels, 5, 0);
	check(classes[1] == kCTBidiR && classes[3] == kCTBidiR,
	      "X10: the initiator and its PDI are one sequence with the R's, so N1 makes them R");
	return g_fail - before;
}

/* **AND THE IMPLICIT LEVELS — UAX #9 I1 AND I2.** The last level rule: a character whose direction DISAGREES with its
 * embedding level is raised, by one or two, and that is what puts a Hebrew letter in a Latin paragraph at level 1 — the
 * ODD level that L2 will later reorder on. */
static int case_bidi_implicit_levels(void)
{
	int before = g_fail;
	uint8_t levels[8];
	CTBidiClass classes[8];

	/* I1: an EVEN level. */
	{
		CTBidiClass c[4] = { kCTBidiL, kCTBidiR, kCTBidiEN, kCTBidiAN };
		uint8_t lv[4] = { 0, 0, 0, 0 };

		ct_bidi_resolve_implicit(c, lv, 4);
		check(lv[0] == 0 && lv[1] == 1 && lv[2] == 2 && lv[3] == 2,
		      "I1: at an even level R goes up ONE and a number TWO");
	}
	/* I2: an ODD level. */
	{
		CTBidiClass c[4] = { kCTBidiL, kCTBidiR, kCTBidiEN, kCTBidiAN };
		uint8_t lv[4] = { 1, 1, 1, 1 };

		ct_bidi_resolve_implicit(c, lv, 4);
		check(lv[0] == 2 && lv[1] == 1 && lv[2] == 2 && lv[3] == 2,
		      "I2: at an odd level L and a number go up ONE and R stays");
	}
	/* AND THE WHOLE PIPELINE, over a Latin-first and a right-to-left-first string. */
	{
		UniChar latinFirst[] = { 'a', 0x05D0 };
		UniChar arabicFirst[] = { 0x0627, '1' };

		check(ct_bidi_explicit_levels(latinFirst, 2, 0, levels, classes), "the pipeline runs");
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		ct_bidi_resolve_neutral(classes, levels, 2, 0);
		ct_bidi_resolve_implicit(classes, levels, 2);
		check(levels[0] == 0 && levels[1] == 1,
		      "a Hebrew letter in a Latin paragraph ends at level 1 — the ODD level L2 reorders on");

		ct_bidi_explicit_levels(arabicFirst, 2, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		ct_bidi_resolve_neutral(classes, levels, 2, 0);
		ct_bidi_resolve_implicit(classes, levels, 2);
		check(levels[0] == 1 && levels[1] == 2,
		      "and the European number after an Arabic letter goes up TWO, to level 2");
	}
	return g_fail - before;
}

/* **AND THE BRACKET PAIRS — UAX #9 N0.** A matched pair takes ONE direction, and the case where that MATTERS is the
 * one where the embedding direction and the context disagree: with N1/N2 alone the CLOSING bracket would take the
 * embedding direction, and N0 is what makes the pair agree. */
static int case_bidi_brackets(void)
{
	int before = g_fail;
	uint8_t levels[16];
	CTBidiClass classes[16];

	/* N0.c: the inside AND the context before both disagree with the embedding direction, so the pair takes THAT
	 * direction — and the CLOSING bracket is where it shows, because N1/N2 alone would make it L. */
	{
		UniChar text[] = { 0x05D0, '(', 0x05D1, ')' };

		check(ct_bidi_explicit_levels(text, 4, 0, levels, classes), "the bracket walk runs");
		ct_bidi_resolve_weak(classes, levels, 4, 0);
		ct_bidi_resolve_brackets(text, classes, levels, 4, 0);
		check(classes[1] == kCTBidiR && classes[3] == kCTBidiR,
		      "N0.c: brackets around an R, with an R before them, become R — including the closer");
	}
	/* N0.b: something INSIDE matches the embedding direction. */
	{
		UniChar text[] = { '(', 'a', ')' };

		ct_bidi_explicit_levels(text, 3, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 3, 0);
		ct_bidi_resolve_brackets(text, classes, levels, 3, 0);
		check(classes[0] == kCTBidiL && classes[2] == kCTBidiL,
		      "N0.b: brackets around an L in a left-to-right paragraph become L");
	}
	/* A NESTED pair, the OUTER one processed first — the standard's own order. */
	{
		UniChar text[] = { 0x05D0, '[', '(', 0x05D1, ')', ']' };

		ct_bidi_explicit_levels(text, 6, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 6, 0);
		ct_bidi_resolve_brackets(text, classes, levels, 6, 0);
		check(classes[1] == kCTBidiR && classes[2] == kCTBidiR
		      && classes[4] == kCTBidiR && classes[5] == kCTBidiR,
		      "both halves of a nested pair resolve to the SAME direction");
	}
	/* N0.d: NOTHING inside is strong, so the brackets are left for N1/N2. */
	{
		UniChar text[] = { '(', ')' };

		ct_bidi_explicit_levels(text, 2, 0, levels, classes);
		ct_bidi_resolve_weak(classes, levels, 2, 0);
		ct_bidi_resolve_brackets(text, classes, levels, 2, 0);
		check(classes[0] == kCTBidiON && classes[1] == kCTBidiON,
		      "N0.d: an empty pair is left NEUTRAL for N1/N2");
		ct_bidi_resolve_neutral(classes, levels, 2, 0);
		check(classes[0] == kCTBidiL && classes[1] == kCTBidiL,
		      "and N1/N2 then settles it on the embedding direction");
	}
	return g_fail - before;
}

/* **AND THE REORDERING — UAX #9 L1 AND L2.** L2 is the payoff: the LEVELS become a VISUAL ORDER by reversing, from the
 * highest level down to the lowest odd one, every maximal run at that level or higher. L1 comes first and takes the
 * separators and the whitespace at the ends out of the picture, so a trailing space is not reordered as text. */
static int case_bidi_reordering(void)
{
	int before = g_fail;
	CFIndex order[8];

	/* L2. */
	{
		uint8_t even[] = { 0, 0, 0 };
		uint8_t rtl[] = { 1, 1, 1 };
		uint8_t mixed[] = { 0, 1, 1, 0 };
		uint8_t nested[] = { 0, 2, 2, 0 };
		uint8_t deep[] = { 0, 2, 1, 0 };

		ct_bidi_reorder(even, 3, order);
		check(order[0] == 0 && order[1] == 1 && order[2] == 2,
		      "L2: an all-even line is left in LOGICAL order");
		ct_bidi_reorder(rtl, 3, order);
		check(order[0] == 2 && order[1] == 1 && order[2] == 0, "L2: an all-right-to-left line is REVERSED");
		ct_bidi_reorder(mixed, 4, order);
		check(order[0] == 0 && order[1] == 2 && order[2] == 1 && order[3] == 3,
		      "L2: an RTL run between two LTR characters is reversed");
		ct_bidi_reorder(nested, 4, order);
		check(order[0] == 0 && order[1] == 1 && order[2] == 2 && order[3] == 3,
		      "L2: an all-EVEN embedded run is NOT reversed — there is no odd level to reverse down to");
		ct_bidi_reorder(deep, 4, order);
		check(order[0] == 0 && order[1] == 2 && order[2] == 1 && order[3] == 3,
		      "L2: a level-2 character and a level-1 one reverse together, as one run");
	}
	/* L1: a separator and the whitespace before it. */
	{
		UniChar text[] = { 'a', ' ', '\n', 'b' };
		uint8_t levels[] = { 0, 1, 1, 1 };

		ct_bidi_reset_levels(text, levels, 4, 0);
		check(levels[1] == 0 && levels[2] == 0 && levels[3] == 1,
		      "L1: a separator and the whitespace BEFORE it take the paragraph level");
	}
	/* L1: the whitespace at the END of the line. */
	{
		UniChar text[] = { 'a', 0x05D0, ' ' };
		uint8_t levels[] = { 0, 1, 1 };

		ct_bidi_reset_levels(text, levels, 3, 0);
		check(levels[1] == 1 && levels[2] == 0, "L1: trailing whitespace takes the paragraph level");
	}
	return g_fail - before;
}

/* **AND THE LINE IS REORDERED — THE PAYOFF.** A right-to-left line's glyphs are PLACED right to left, so the FIRST
 * logical character's glyph is at the RIGHT end of the line and the LAST is at the origin; and the run says so, because
 * its string indices are no longer monotonic. */
static int case_line_bidi_reorder(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	CFMutableDictionaryRef attrs = NULL;
	CFAttributedStringRef astr = NULL;
	CTLineRef line = NULL;
	UniChar hebrew[] = { 0x05D0, 0x05D1, 0x05D2 };
	CFStringRef h = (font != NULL) ? CFStringCreateWithCharacters(kCFAllocatorDefault, hebrew, 3) : NULL;

	check(font != NULL && h != NULL, "the reorder case could not be set up");
	if (font == NULL || h == NULL) {
		if (h != NULL) CFRelease(h);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
	line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
	check(line != NULL, "the Hebrew line was made");
	if (line != NULL) {
		CFArrayRef runs = CTLineGetGlyphRuns(line);
		CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
			? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
		CGFloat first = CTLineGetOffsetForStringIndex(line, 0, NULL);
		CGFloat middle = CTLineGetOffsetForStringIndex(line, 1, NULL);
		CGFloat last = CTLineGetOffsetForStringIndex(line, 2, NULL);
		CGFloat afterLast = CTLineGetOffsetForStringIndex(line, 3, NULL);
		double width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);

		printf("  (Hebrew carets: %g > %g > %g > %g; width %g)\n", (double)first, (double)middle, (double)last,
		       (double)afterLast, width);
		check(run != NULL && (CTRunGetStatus(run) & kCTRunStatusNonMonotonic) != 0,
		      "a right-to-left line's run reports NonMonotonic");
		check(fabs((double)first - width) < 0.01,
		      "the caret BEFORE the first character is at the line's RIGHT end — its LEADING edge");
		check(first > middle && middle > last && last > afterLast,
		      "and the carets run RIGHT to LEFT, which is the paragraph's direction");
		check(fabs((double)afterLast) < 0.01, "with the caret AFTER the last character at the origin");
		check(width > 0.0, "and the width is still the whole line");
		CFRelease(line);
	}
	if (astr != NULL) CFRelease(astr);
	CFRelease(attrs);
	CFRelease(h);
	CFRelease(font);
	return g_fail - before;
}

/* **AND THE MIRRORED GLYPH — UAX #9 L4.** A RIGHT-TO-LEFT character is depicted by its mirror, so `(` shows as `)`.
 * The probe reads it twice: the character rewrite directly, and the GLYPH the line actually asks the font for. */
static int case_bidi_mirroring(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);

	/* L4, DIRECTLY. */
	{
		UniChar chars[] = { '(', ')', '<', 'a' };
		uint8_t levels[] = { 1, 1, 1, 1 };

		ct_bidi_mirror(chars, 4, levels);
		check(chars[0] == ')' && chars[1] == '(' && chars[2] == '>' && chars[3] == 'a',
		      "L4: an odd level mirrors `(` to `)` and `<` to `>`, and leaves `a` alone");
	}
	{
		UniChar chars[] = { '(' };
		uint8_t levels[] = { 0 };

		ct_bidi_mirror(chars, 1, levels);
		check(chars[0] == '(', "L4: an EVEN level is not mirrored");
	}
	/* **AND THE LINE ASKS THE FONT FOR THE MIRRORED ONE.** `Hebrew alef`, then `(`: the run's SECOND glyph must be the
	 * font's glyph for `)`, which is what L4 means in the layout. */
	check(font != NULL, "the mirroring case has a font");
	if (font != NULL) {
		UniChar text[] = { 0x05D0, '(' };
		CFStringRef s = CFStringCreateWithCharacters(kCFAllocatorDefault, text, 2);
		CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
			&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		UniChar openChar = '(', closeChar = ')';
		CGGlyph openGlyph = 0, closeGlyph = 0;
		CFAttributedStringRef astr;
		CTLineRef line;

		(void)CTFontGetGlyphsForCharacters(font, &openChar, &openGlyph, 1);
		(void)CTFontGetGlyphsForCharacters(font, &closeChar, &closeGlyph, 1);
		CFDictionarySetValue(attrs, kCTFontAttributeName, font);
		astr = (s != NULL) ? CFAttributedStringCreate(kCFAllocatorDefault, s, attrs) : NULL;
		line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;
		check(line != NULL, "the mirroring line was made");
		if (line != NULL) {
			CFArrayRef runs = CTLineGetGlyphRuns(line);
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CGGlyph bracket = glyph_for_character(run, 1);	/* character 1 is the `(` */

			printf("  (the line's glyph for `(` is %u; the font's `)` glyph is %u)\n",
			       (unsigned)bracket, (unsigned)closeGlyph);
			check(closeGlyph != 0 && openGlyph != closeGlyph && bracket == closeGlyph,
			      "L4: the LINE looks up the MIRRORED character, not the one in the string");
			CFRelease(line);
		}
		if (astr != NULL) CFRelease(astr);
		CFRelease(attrs);
		if (s != NULL) CFRelease(s);
		CFRelease(font);
	}
	return g_fail - before;
}

/* **AND A LINE IS NOT ALWAYS ONE FACE.** The run array's whole point is that a line holds AS MANY FONTS AS ITS
 * ATTRIBUTES ASK FOR, so this case sets four characters in TWO faces and reads the partition back three ways: the run
 * COUNT, each run's OWN font out of its attributes, and each run's glyphs INDEXED IN THAT FONT — the last is what
 * separates a real second run from a first font drawing glyphs that face never made. The width closes it: a line of two
 * runs is as wide as the two lines it is made of, measured separately. The two faces are a MONOSPACE and a PROPORTIONAL
 * one so that they cannot agree by accident. */
static int case_line_font_runs(void)
{
	int before = g_fail;
	const char *pathB = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf";
	CTFontRef fontA = test_font(20.0);
	CGDataProviderRef providerB = (access(pathB, R_OK) == 0) ? CGDataProviderCreateWithFilename(pathB) : NULL;
	CGFontRef faceB = (providerB != NULL) ? CGFontCreateWithDataProvider(providerB) : NULL;
	CTFontRef fontB = NULL;

	if (faceB != NULL) {
		check(cg_font_register(pathB) != 0 || cg_font_is_registered(pathB),
		      "the second face at %s could not be registered", pathB);
		fontB = CTFontCreateWithName(CFSTR("DejaVuSerif"), 20.0, NULL);
		if (fontB == NULL) {
			fontB = CTFontCreateWithGraphicsFont(faceB, 20.0, NULL, NULL);
		}
	}
	check(fontA != NULL && fontB != NULL, "the two faces of the font-run case could not be made");
	if (fontA != NULL && fontB != NULL) {
		UniChar chars[] = { 'A', 'A', 'A', 'A' };
		UniChar pair[] = { 'A', 'A' };
		UniChar cap = 'A';
		CFStringRef text = CFStringCreateWithCharacters(kCFAllocatorDefault, chars, 4);
		CFStringRef two = CFStringCreateWithCharacters(kCFAllocatorDefault, pair, 2);
		CFMutableDictionaryRef attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
			&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFAttributedStringRef base = NULL, runA = NULL, runB = NULL;
		CFMutableAttributedStringRef astr = NULL;
		CTLineRef mixed = NULL, lineA = NULL, lineB = NULL;
		CGGlyph glyphA = 0, glyphB = 0;

		if (attrs != NULL && text != NULL && two != NULL) {
			CFDictionarySetValue(attrs, kCTFontAttributeName, fontA);
			base = CFAttributedStringCreate(kCFAllocatorDefault, text, attrs);
			if (base != NULL) {
				astr = CFAttributedStringCreateMutableCopy(kCFAllocatorDefault, 0, base);
			}
			if (astr != NULL) {
				/* THE SECOND FACE COVERS THE SECOND HALF: that is the whole input to the partition. */
				CFAttributedStringSetAttribute(astr, CFRangeMake(2, 2), kCTFontAttributeName, fontB);
				mixed = CTLineCreateWithAttributedString(astr);
			}
			CFDictionarySetValue(attrs, kCTFontAttributeName, fontB);
			runB = CFAttributedStringCreate(kCFAllocatorDefault, two, attrs);
			lineB = (runB != NULL) ? CTLineCreateWithAttributedString(runB) : NULL;
			CFDictionarySetValue(attrs, kCTFontAttributeName, fontA);
			runA = CFAttributedStringCreate(kCFAllocatorDefault, two, attrs);
			lineA = (runA != NULL) ? CTLineCreateWithAttributedString(runA) : NULL;
		}
		(void)CTFontGetGlyphsForCharacters(fontA, &cap, &glyphA, 1);
		(void)CTFontGetGlyphsForCharacters(fontB, &cap, &glyphB, 1);
		check(mixed != NULL && lineA != NULL && lineB != NULL, "the font-run lines were made");
		if (mixed != NULL && lineA != NULL && lineB != NULL) {
			CFArrayRef runs = CTLineGetGlyphRuns(mixed);
			CFIndex count = (runs != NULL) ? CFArrayGetCount(runs) : 0;
			double wMixed = CTLineGetTypographicBounds(mixed, NULL, NULL, NULL);
			double wA = CTLineGetTypographicBounds(lineA, NULL, NULL, NULL);
			double wB = CTLineGetTypographicBounds(lineB, NULL, NULL, NULL);

			printf("  (font runs: %ld; the two faces apart %g and %g, together %g; `A` is glyph %u then %u)\n",
			       (long)count, wA, wB, wMixed, (unsigned)glyphA, (unsigned)glyphB);
			check(count == 2, "a two-font line reports TWO runs");
			if (count == 2) {
				CTRunRef r0 = (CTRunRef)CFArrayGetValueAtIndex(runs, 0);
				CTRunRef r1 = (CTRunRef)CFArrayGetValueAtIndex(runs, 1);
				CFDictionaryRef a0 = CTRunGetAttributes(r0);
				CFDictionaryRef a1 = CTRunGetAttributes(r1);
				const void *f0 = (a0 != NULL) ? CFDictionaryGetValue(a0, kCTFontAttributeName) : NULL;
				const void *f1 = (a1 != NULL) ? CFDictionaryGetValue(a1, kCTFontAttributeName) : NULL;
				const CGGlyph *g0 = CTRunGetGlyphsPtr(r0);
				const CGGlyph *g1 = CTRunGetGlyphsPtr(r1);
				const CGSize *adv0 = CTRunGetAdvancesPtr(r0);
				const CGSize *adv1 = CTRunGetAdvancesPtr(r1);
				CGSize advA = CGSizeMake(0.0, 0.0), advB = CGSizeMake(0.0, 0.0);
				CFRange sr0 = CTRunGetStringRange(r0);
				CFRange sr1 = CTRunGetStringRange(r1);

				(void)CTFontGetAdvancesForGlyphs(fontA, kCTFontHorizontalOrientation, &glyphA, &advA, 1);
				(void)CTFontGetAdvancesForGlyphs(fontB, kCTFontHorizontalOrientation, &glyphB, &advB, 1);
				check(f0 == (const void *)fontA && f1 == (const void *)fontB,
				      "each run's attributes name its OWN face");
				check(sr0.location == 0 && sr0.length == 2 && sr1.location == 2 && sr1.length == 2,
				      "and its own half of the string");
				check(CTRunGetGlyphCount(r0) == 2 && CTRunGetGlyphCount(r1) == 2,
				      "two glyphs each, one per character");
				check(g0 != NULL && g1 != NULL && g0[0] == glyphA && g1[0] == glyphB,
				      "each run's glyphs are the ones ITS OWN face maps the character to");
				check(adv0 != NULL && adv1 != NULL && fabs(adv0[0].width - advA.width) < 0.001
				      && fabs(adv1[0].width - advB.width) < 0.001 && advA.width != advB.width,
				      "and each run's advance is its own face's, a difference no single face could show");
				check((CTRunGetStatus(r0) & kCTRunStatusNonMonotonic) == 0
				      && (CTRunGetStatus(r1) & kCTRunStatusNonMonotonic) == 0,
				      "while two running left-to-right are still monotonic");
				check(fabs((wA + wB) - wMixed) < 0.01,
				      "and the mixed line is exactly as wide as the two faces measured apart");
			}
		}
		if (mixed != NULL) CFRelease(mixed);
		if (lineA != NULL) CFRelease(lineA);
		if (lineB != NULL) CFRelease(lineB);
		if (runA != NULL) CFRelease(runA);
		if (runB != NULL) CFRelease(runB);
		if (astr != NULL) CFRelease(astr);
		if (base != NULL) CFRelease(base);
		if (text != NULL) CFRelease(text);
		if (two != NULL) CFRelease(two);
		if (attrs != NULL) CFRelease(attrs);
	}
	if (fontA != NULL) CFRelease(fontA);
	if (fontB != NULL) CFRelease(fontB);
	if (faceB != NULL) CGFontRelease(faceB);
	if (providerB != NULL) CGDataProviderRelease(providerB);
	return g_fail - before;
}

/* **THE TYPESETTER'S TWO OPTIONS, WHICH WERE RECORDED AND NOT OBEYED UNTIL THE LINE COULD BE TOLD.** Both are about
 * the Unicode Bidirectional Algorithm, and the algorithm runs in the LINE — so `CTTypesetterCreateLine` reads them
 * into a `CTLineBidiDirective` and the line acts on it. What is asserted here is that the options reach the layout at
 * all, which they could not before, and that each one does the thing its own name says.
 *
 * THE THREE READINGS THIS TIER TAKES ARE EACH AN ASSERTION, because Apple leaves them open: a value of the WRONG TYPE
 * and a forced level that is not 0 or 1 are IGNORED rather than refused, and `DisableBidiProcessing` WINS over a forced
 * level. A door that quietly ignored everything would pass the first two; a door that refused would fail the layout for
 * one bad key. */
static int case_typesetter_bidi_options(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	/* ALEF, BET, THEN A MIRRORABLE BRACKET: a right-to-left paragraph by P2/P3, and one character L4 can move. */
	UniChar hebrew[] = { 0x05D0, 0x05D1, '(' };
	UniChar latin[] = { 'a', 'b', 'c' };
	CFStringRef h = (font != NULL) ? CFStringCreateWithCharacters(kCFAllocatorDefault, hebrew, 3) : NULL;
	CFStringRef l = (font != NULL) ? CFStringCreateWithCharacters(kCFAllocatorDefault, latin, 3) : NULL;
	CFMutableDictionaryRef attrs = NULL;
	CFMutableDictionaryRef options = NULL;
	CFAttributedStringRef astr = NULL;
	CGGlyph openGlyph = 0, closeGlyph = 0;
	UniChar openChar = '(', closeChar = ')';

	check(font != NULL && h != NULL && l != NULL, "the bidi-option case could not be set up");
	if (font == NULL || h == NULL || l == NULL) {
		if (h != NULL) CFRelease(h);
		if (l != NULL) CFRelease(l);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	(void)CTFontGetGlyphsForCharacters(font, &openChar, &openGlyph, 1);
	(void)CTFontGetGlyphsForCharacters(font, &closeChar, &closeGlyph, 1);
	check(openGlyph != 0 && closeGlyph != 0 && openGlyph != closeGlyph,
	      "the face in this case maps `(` and `)` to two different glyphs, which is what makes mirroring visible");
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	options = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);

	/* 1. **THE BASELINE, WITH NO OPTIONS.** The Hebrew string runs right to left, the glyphs are REORDERED, and the
	 * bracket is MIRRORED — the three things the options below are measured against. */
	if (h != NULL) {
		astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
		if (astr != NULL) {
			CTTypesetterRef ts = CTTypesetterCreateWithAttributedString(astr);
			CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
			CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;
			CGGlyph bracket = glyph_for_character(run, 2);	/* character 2 is the `(` */

			printf("  (no options: status %u, the bracket is glyph %u)\n", (unsigned)status,
			       (unsigned)bracket);
			check((status & kCTRunStatusRightToLeft) != 0 && (status & kCTRunStatusNonMonotonic) != 0,
			      "with no options a Hebrew line reports RightToLeft AND NonMonotonic");
			check(bracket == closeGlyph, "and L4 mirrors its bracket, so the font is asked for `)`");
			if (line != NULL) CFRelease(line);
			if (ts != NULL) CFRelease(ts);
			CFRelease(astr);
		}
	}

	/* 2. **DISABLE BIDI PROCESSING: NOTHING RUNS AND NOTHING MOVES.** No direction is computed, no glyph is
	 * reordered and NO CHARACTER IS MIRRORED — which is the third of those that shows the option reaches the whole
	 * algorithm rather than just the reordering. */
	CFDictionarySetValue(options, kCTTypesetterOptionDisableBidiProcessing, kCFBooleanTrue);
	{
		astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
		if (astr != NULL) {
			CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
			CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
			CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;
			const CGGlyph *glyphs = (run != NULL) ? CTRunGetGlyphsPtr(run) : NULL;

			printf("  (bidi disabled: status %u, the bracket is glyph %u)\n", (unsigned)status,
			       (unsigned)((glyphs != NULL) ? glyphs[2] : 0));
			check(run != NULL && status == kCTRunStatusNoStatus,
			      "a line with bidi processing disabled reports NO status at all, not %u", (unsigned)status);
			check(glyphs != NULL && glyphs[2] == openGlyph,
			      "and the MIRRORING is off too, so the font is asked for the `(` it was given");
			if (line != NULL) CFRelease(line);
			if (ts != NULL) CFRelease(ts);
			CFRelease(astr);
		}
	}
	CFDictionaryRemoveValue(options, kCTTypesetterOptionDisableBidiProcessing);

	/* 3. **A FORCED LEVEL OVERRIDES P2/P3.** The same Hebrew string with level 0 forced is a LEFT-TO-RIGHT paragraph,
	 * so nothing is reordered and the run says nothing. */
	{
		CFNumberRef zero = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &(int){ 0 });
		CFNumberRef one = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &(int){ 1 });

		CFDictionarySetValue(options, kCTTypesetterOptionForcedEmbeddingLevel, zero);
		astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
		if (astr != NULL) {
			CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
			CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
			CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;

			/* **THE PARAGRAPH IS LEFT TO RIGHT NOW, AND THE HEBREW RUN INSIDE IT IS STILL REORDERED** — which is
			 * UAX #9's own answer and not a hedge: forcing the paragraph level changes the PARAGRAPH's direction,
			 * and an RTL run inside an LTR paragraph is reordered either way. The DIRECTION bit is the proof that
			 * the level was forced, because the same string with no options reports it. A check demanding no
			 * NonMonotonic here would be demanding the algorithm get its own rules wrong. */
			check(run != NULL && (status & kCTRunStatusRightToLeft) == 0,
			      "a forced level of 0 takes the RightToLeft bit off a Hebrew paragraph (status %u)",
			      (unsigned)status);
			check(run != NULL && (status & kCTRunStatusNonMonotonic) != 0,
			      "while its Hebrew RUN is still reordered, because a run inside a paragraph is not the paragraph");
			if (line != NULL) CFRelease(line);
			if (ts != NULL) CFRelease(ts);
			CFRelease(astr);
		}
		/* AND THE OTHER WAY: a LATIN string with level 1 forced is reordered, so the option is not simply
		 * "turn everything off". */
		CFDictionarySetValue(options, kCTTypesetterOptionForcedEmbeddingLevel, one);
		astr = CFAttributedStringCreate(kCFAllocatorDefault, l, attrs);
		if (astr != NULL) {
			CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
			CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
			CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;
			CGFloat first = (line != NULL) ? CTLineGetOffsetForStringIndex(line, 0, NULL) : 0.0;
			CGFloat last = (line != NULL) ? CTLineGetOffsetForStringIndex(line, 3, NULL) : 0.0;

			printf("  (a Latin line with level 1 forced: status %u, carets %g then %g)\n",
			       (unsigned)status, (double)first, (double)last);
			check((status & kCTRunStatusRightToLeft) != 0,
			      "a forced level of 1 puts the RightToLeft bit on a LATIN paragraph (status %u)",
			      (unsigned)status);
			/* **AND THE LATIN RUN KEEPS ITS OWN ORDER, WHICH IS I2 AND NOT A MISSING REORDER.** I2 raises the level
			 * of every L character in an odd-level paragraph, so "abc" becomes an EVEN-level run inside a
			 * right-to-left paragraph and L2 leaves it where it is. The caret pair is what MEASURES that the run's
			 * order did not change: the offsets still ascend with the string. */
			check((status & kCTRunStatusNonMonotonic) == 0 && first < last,
			      "and its Latin run keeps its order — carets %g then %g — which is I2's own answer",
			      (double)first, (double)last);
			if (line != NULL) CFRelease(line);
			if (ts != NULL) CFRelease(ts);
			CFRelease(astr);
		}
		/* **AND A FORCED LEVEL BEATS THE DIRECTION THE DOCUMENT ITSELF STATES**, which is the whole meaning of the
		 * word "forced": a caller whose document says left-to-right and whose option says level 1 gets a
		 * right-to-left paragraph. Without this the option would only be a DEFAULT, and a paragraph style would
		 * silently outrank the caller. */
		{
			CTParagraphStyleSetting setting;
			CTWritingDirection stated = kCTWritingDirectionLeftToRight;
			CTParagraphStyleRef ltrStyle;
			CFMutableDictionaryRef statedAttrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
				&kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

			setting.spec = kCTParagraphStyleSpecifierBaseWritingDirection;
			setting.valueSize = sizeof stated;
			setting.value = &stated;
			ltrStyle = CTParagraphStyleCreate(&setting, 1);
			CFDictionarySetValue(statedAttrs, kCTFontAttributeName, font);
			CFDictionarySetValue(statedAttrs, kCTParagraphStyleAttributeName, ltrStyle);
			astr = CFAttributedStringCreate(kCFAllocatorDefault, h, statedAttrs);
			if (astr != NULL) {
				CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
				CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
				CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
				CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
					? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
				CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;

				check(run != NULL && (status & kCTRunStatusRightToLeft) != 0,
				      "a forced level of 1 beats a paragraph style that states left-to-right (status %u)",
				      (unsigned)status);
				if (line != NULL) CFRelease(line);
				if (ts != NULL) CFRelease(ts);
				CFRelease(astr);
			}
			CFRelease(ltrStyle);
			CFRelease(statedAttrs);
		}
		CFDictionaryRemoveValue(options, kCTTypesetterOptionForcedEmbeddingLevel);

		/* 4. **A VALUE OF THE WRONG TYPE, AND A LEVEL THAT IS NOT 0 OR 1, ARE IGNORED RATHER THAN REFUSED.** Apple
		 * makes both a precondition; this tier's reading is that a typesetter with a nonsense option lays out as if
		 * the option were absent, and the case says so by comparing against the baseline's own status. */
		CFDictionarySetValue(options, kCTTypesetterOptionDisableBidiProcessing, CFSTR("yes"));
		{
			CFNumberRef five = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &(int){ 5 });

			CFDictionarySetValue(options, kCTTypesetterOptionForcedEmbeddingLevel, five);
			astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
			if (astr != NULL) {
				CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
				CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
				CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
				CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
					? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
				CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;

				check(ts != NULL, "a typesetter with a nonsense option is still made, not refused");
				check(run != NULL && (status & kCTRunStatusRightToLeft) != 0
				      && (status & kCTRunStatusNonMonotonic) != 0,
				      "and the line lays out as if both options were absent (status %u)", (unsigned)status);
				if (line != NULL) CFRelease(line);
				if (ts != NULL) CFRelease(ts);
				CFRelease(astr);
			}
			CFRelease(five);
		}
		/* AND WHEN BOTH ARE GIVEN, DISABLING WINS: a forced level is an input to an algorithm that is not going to
		 * run, so honouring it would leave a line half-processed. Apple does not say which wins; this is the choice. */
		CFDictionarySetValue(options, kCTTypesetterOptionDisableBidiProcessing, kCFBooleanTrue);
		CFDictionarySetValue(options, kCTTypesetterOptionForcedEmbeddingLevel, one);
		astr = CFAttributedStringCreate(kCFAllocatorDefault, h, attrs);
		if (astr != NULL) {
			CTTypesetterRef ts = CTTypesetterCreateWithAttributedStringAndOptions(astr, options);
			CTLineRef line = (ts != NULL) ? CTTypesetterCreateLine(ts, CFRangeMake(0, 0)) : NULL;
			CFArrayRef runs = (line != NULL) ? CTLineGetGlyphRuns(line) : NULL;
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			CTRunStatus status = (run != NULL) ? CTRunGetStatus(run) : kCTRunStatusNoStatus;
			CGGlyph bracket = glyph_for_character(run, 2);	/* character 2 is the `(` */

			printf("  (both given: status %u, the bracket is glyph %u)\n", (unsigned)status,
			       (unsigned)bracket);
			/* **DISABLING WINS OVER THE FORCED LEVEL, AND THE WAY IT WINS IS RATHER THAN BY DISCARDING IT.** The
			 * forced level is the CALLER STATING THE PARAGRAPH'S DIRECTION — the same kind of fact as a paragraph
			 * style's base writing direction — so it still decides the direction; what disabling stops is the
			 * ALGORITHM. A stopped algorithm is exactly "no NonMonotonic and no mirroring", so a caller that asked
			 * for both gets the direction it asked for with nothing moved. */
			/* **AND THE FLAG IS NOW APPLE'S OWN FACT RATHER THAN THE PIPELINE'S**: `kCTRunStatusNonMonotonic` says
			 * the run's STRING INDICES do not ascend, and with the pipeline off the shaper is still told the line's
			 * direction — it has no levels to read — so a Hebrew run comes back in the shaper's visual order and its
			 * indices DESCEND. The flag is therefore SET, and what says the algorithm was off is the MIRRORING
			 * below rather than the flag. Before shaping these two answers were the same; they are not any more,
			 * and the case records the difference instead of hiding it. */
			check(run != NULL && (status & kCTRunStatusRightToLeft) != 0
			      && (status & kCTRunStatusNonMonotonic) != 0,
			      "with both given the forced DIRECTION stands, and the SHAPER's descending order sets the flag "
			      "(status %u)", (unsigned)status);
			check(bracket == openGlyph,
			      "and the algorithm really was off, because the bracket is NOT mirrored");
			if (line != NULL) CFRelease(line);
			if (ts != NULL) CFRelease(ts);
			CFRelease(astr);
		}
		CFRelease(one);
		CFRelease(zero);
	}

	/* 5. **AND THE BREAKER IS UNMOVED, WHICH IS WHY THE OPTIONS COST IT NOTHING.** Bidi changes no advance, so the
	 * count `CTTypesetterSuggestLineBreak` answers is the same with the options as without — if the typesetter had
	 * tried to apply the algorithm while breaking, this is the check that would catch it. */
	{
		CFStringRef wide = CFSTR("one two three four five");
		CFAttributedStringRef plainText = CFAttributedStringCreate(kCFAllocatorDefault, wide, attrs);

		CFDictionaryRemoveValue(options, kCTTypesetterOptionForcedEmbeddingLevel);
		if (plainText != NULL) {
			CTTypesetterRef plain = CTTypesetterCreateWithAttributedString(plainText);
			CTTypesetterRef off = CTTypesetterCreateWithAttributedStringAndOptions(plainText, options);
			CFIndex a = (plain != NULL) ? CTTypesetterSuggestLineBreak(plain, 0, 60.0) : -1;
			CFIndex b = (off != NULL) ? CTTypesetterSuggestLineBreak(off, 0, 60.0) : -2;

			printf("  (the breaker answers %ld with no options and %ld with bidi disabled)\n",
			       (long)a, (long)b);
			check(a > 0 && a == b, "the break count is the same with bidi disabled as without");
			if (plain != NULL) CFRelease(plain);
			if (off != NULL) CFRelease(off);
			CFRelease(plainText);
		}
	}

	CFRelease(options);
	CFRelease(attrs);
	CFRelease(l);
	CFRelease(h);
	CFRelease(font);
	return g_fail - before;
}

/* =================================================================================================
 * SHAPING, WHICH IS THE ONE THING THIS LIBRARY TAKES FROM OUTSIDE ITSELF.
 * =============================================================================================== */
/* **A FACE FROM ITS FILE, WHICH IS HOW THESE CASES GET FACES THAT ARE NOT THE SUITE'S ONE.** Every claim below is a
 * property of a FACE — one font has an `ffi` ligature and another does not, one composes `a`+U+0301 and another
 * attaches the mark — so a case that could only reach the suite's monospace face could not make any of them. The
 * graphics font is released after the `CTFont` is made, because that door retains it. */
static CTFontRef shaping_font(const char *path, CGFloat size)
{
	CGDataProviderRef provider = CGDataProviderCreateWithFilename(path);
	CGFontRef face = (provider != NULL) ? CGFontCreateWithDataProvider(provider) : NULL;
	CTFontRef font = (face != NULL) ? CTFontCreateWithGraphicsFont(face, size, NULL, NULL) : NULL;

	if (face != NULL) {
		CGFontRelease(face);
	}
	if (provider != NULL) {
		CGDataProviderRelease(provider);
	}
	return font;
}

/* **A LIGATURE IS THE SMALLEST THING A ONE-GLYPH-PER-CHARACTER MAPPING CANNOT EXPRESS**, and the case uses THREE
 * faces to show that the difference is the FONT's rather than the text's: `DejaVuSans` sets `ffi` in ONE glyph,
 * `DejaVuSerif` sets it in TWO (an `ff` ligature and the `i`), and `LiberationSerif` sets it in THREE. Same three
 * characters, same shaper, three answers — which is why the check is on the CLUSTERS as well as the count: a
 * ligature is a glyph that stands for characters 0..2, so cluster 0 is what says so. */
static int case_shaping_ligatures(void)
{
	int before = g_fail;
	UniChar ffi[] = { 'f', 'f', 'i' };
	struct { const char *path; CFIndex glyphs; } faces[] = {
		{ "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 1 },
		{ "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 2 },
		{ "/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf", 3 },
	};
	size_t i;

	for (i = 0; i < sizeof faces / sizeof faces[0]; i++) {
		CTFontRef font = shaping_font(faces[i].path, 13.0);
		CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
		CTShapedRun run;
		bool ok;

		CTShapedRunZero(&run);
		check(shaper != NULL, "a shaper could not be made for %s", faces[i].path);
		if (shaper == NULL) {
			if (font != NULL) {
				CFRelease(font);
			}
			continue;
		}
		ok = CTShaperShape(shaper, ffi, 3, kCTWritingDirectionLeftToRight, "Latn", NULL, &run);
		printf("  (%s: 3 characters -> %ld glyph(s), first cluster %ld)\n", faces[i].path + 26,
		       (long)run.glyphCount, (long)((run.glyphCount > 0) ? run.clusters[0] : -1));
		check(ok, "`ffi` could not be shaped in %s", faces[i].path);
		check(ok && run.glyphCount == faces[i].glyphs,
		      "%s sets `ffi` in %ld glyph(s), not %ld", faces[i].path + 26, (long)run.glyphCount,
		      (long)faces[i].glyphs);
		/* **AND THE CLUSTER IS AN INDEX INTO THE CHARACTERS THE CALLER GAVE**, with nothing added: the first glyph
		 * of every one of the three answers stands for the character at 0, because that is where its run started. */
		check(ok && run.clusters[0] == 0, "the first glyph's cluster is 0, the first character");
		if (faces[i].glyphs == 1) {
			/* ONE GLYPH FOR THREE CHARACTERS IS THE LIGATURE, and the cluster is the only thing that says so. */
			check(ok && run.glyphCount == 1, "and one glyph stands for ALL THREE characters, which is a ligature");
			check(ok && run.advances[0].width > 0.0, "with a real advance, %g", (double)run.advances[0].width);
		}
		CTShapedRunFree(&run);
		/* AND A SECOND SHAPE OF THE SAME RUN IS THE SAME ANSWER, so nothing was consumed by the first. */
		ok = CTShaperShape(shaper, ffi, 3, kCTWritingDirectionLeftToRight, "Latn", NULL, &run);
		check(ok && run.glyphCount == faces[i].glyphs, "shaping the same run twice gives the same answer");
		CTShapedRunFree(&run);
		/* A FREED RUN IS ZEROED, so a second free is harmless — the contract the header states. */
		CTShapedRunFree(&run);
		CTShaperRelease(shaper);
		CFRelease(font);
	}
	return g_fail - before;
}

/* **AND A PAIR ADJUSTMENT IS WHAT SHAPING DOES TO ADVANCES.** `To` is the pair this suite already measures against
 * the font's own `kern` table, so the claim here is a cross-check rather than a new number: the shaped advance of
 * the `T` must be NARROWER than the `T`'s own advance, and the size of the difference is printed beside what
 * `CTFontGetKerningBetweenGlyphs` says, because HarfBuzz prefers `GPOS` where a face has it and `kern` where it does
 * not — so the two need not be equal and the case does not pretend they are. */
static int case_shaping_kerning(void)
{
	int before = g_fail;
	CTFontRef font = shaping_font("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 13.0);
	CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
	UniChar to[] = { 'T', 'o' };
	UniChar tChar = 'T', oChar = 'o';
	CGGlyph tGlyph = 0, oGlyph = 0;
	CGSize plain = CGSizeMake(0.0, 0.0);
	CTShapedRun run;

	CTShapedRunZero(&run);
	check(font != NULL && shaper != NULL, "the kerning case could not make its face or its shaper");
	if (font == NULL || shaper == NULL) {
		if (shaper != NULL) CTShaperRelease(shaper);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	(void)CTFontGetGlyphsForCharacters(font, &tChar, &tGlyph, 1);
	(void)CTFontGetGlyphsForCharacters(font, &oChar, &oGlyph, 1);
	(void)CTFontGetAdvancesForGlyphs(font, kCTFontHorizontalOrientation, &tGlyph, &plain, 1);
	check(CTShaperShape(shaper, to, 2, kCTWritingDirectionLeftToRight, "Latn", NULL, &run),
	      "`To` could not be shaped");
	if (run.glyphCount == 2) {
		printf("  (`T` alone is %g, shaped `To` gives it %g, the `kern` table says %g)\n", (double)plain.width,
		       (double)run.advances[0].width, (double)CTFontGetKerningBetweenGlyphs(font, tGlyph, oGlyph));
		check(run.advances[0].width < plain.width,
		      "shaping closes the `To` pair: %g against the `T`'s own %g", (double)run.advances[0].width,
		      (double)plain.width);
		check(run.clusters[0] == 0 && run.clusters[1] == 1,
		      "and the two glyphs keep one cluster each, at 0 and 1");
	}
	CTShapedRunFree(&run);
	CTShaperRelease(shaper);
	CFRelease(font);
	return g_fail - before;
}

/* **A COMBINING MARK, WHERE TWO FACES GIVE TWO DIFFERENT AND EQUALLY CORRECT ANSWERS.** `a` followed by U+0301 is
 * two characters. `DejaVuSerif` COMPOSES them into the one glyph the face already has for U+00E1 — so the check is
 * against that glyph, read out of the face independently of the shaper. `FreeSerif` does NOT: it gives two glyphs
 * whose clusters are BOTH 0, which is the shaping fact that one character can begin a glyph and the next can share
 * its cluster, and its second glyph carries an OFFSET, which is how a mark is moved onto the letter it belongs to.
 * A case that only knew about the first face would conclude that marks are composed and be wrong about the second. */
static int case_shaping_marks(void)
{
	int before = g_fail;
	UniChar aacute[] = { 'a', 0x0301 };
	UniChar precomposed = 0x00E1;

	{
		CTFontRef font = shaping_font("/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf", 13.0);
		CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
		CTShapedRun run;
		CGGlyph composed = 0;

		CTShapedRunZero(&run);
		if (font != NULL) {
			(void)CTFontGetGlyphsForCharacters(font, &precomposed, &composed, 1);
		}
		if (shaper != NULL && CTShaperShape(shaper, aacute, 2, kCTWritingDirectionLeftToRight, "Latn", NULL, &run)) {
			printf("  (DejaVuSerif: `a`+U+0301 -> %ld glyph, %u; the face's U+00E1 is %u)\n",
			       (long)run.glyphCount, (unsigned)((run.glyphCount > 0) ? run.glyphs[0] : 0),
			       (unsigned)composed);
			check(run.glyphCount == 1 && composed != 0 && run.glyphs[0] == composed,
			      "DejaVuSerif COMPOSES `a`+U+0301 into the glyph it already has for U+00E1");
		} else {
			check(0, "`a`+U+0301 could not be shaped in DejaVuSerif");
		}
		CTShapedRunFree(&run);
		if (shaper != NULL) CTShaperRelease(shaper);
		if (font != NULL) CFRelease(font);
	}
	{
		CTFontRef font = shaping_font("/usr/share/fonts/truetype/freefont/FreeSerif.ttf", 13.0);
		CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
		CTShapedRun run;

		CTShapedRunZero(&run);
		if (shaper != NULL && CTShaperShape(shaper, aacute, 2, kCTWritingDirectionLeftToRight, "Latn", NULL, &run)) {
			printf("  (FreeSerif: `a`+U+0301 -> %ld glyphs, clusters %ld/%ld, the mark offset %g)\n",
			       (long)run.glyphCount, (long)((run.glyphCount > 1) ? run.clusters[0] : -1),
			       (long)((run.glyphCount > 1) ? run.clusters[1] : -1),
			       (double)((run.glyphCount > 1) ? run.offsets[1].x : 0.0));
			check(run.glyphCount == 2, "FreeSerif does NOT compose it: two glyphs, not %ld",
			      (long)run.glyphCount);
			if (run.glyphCount == 2) {
				check(run.clusters[0] == 0 && run.clusters[1] == 0,
				      "and BOTH glyphs share cluster 0, because both stand for the character at 0");
				/* THE MARK'S OFFSET IS WHAT MOVES IT ONTO THE LETTER. A zero here would mean the accent is set
				 * where its own advance put it, which is where a ONE-GLYPH-PER-CHARACTER layout puts it — so
				 * this number is the difference between mark placement and no mark placement. */
				check(run.offsets[1].x != 0.0 || run.offsets[1].y != 0.0,
				      "and the mark carries a POSITIONING offset, which is what places it on the letter");
			}
		} else {
			check(0, "`a`+U+0301 could not be shaped in FreeSerif");
		}
		CTShapedRunFree(&run);
		if (shaper != NULL) CTShaperRelease(shaper);
		if (font != NULL) CFRelease(font);
	}
	return g_fail - before;
}

/* **AND THE TWO THINGS THIS ENGINE EXISTS FOR: A CONJUNCT AND A JOINED SCRIPT.** Neither can be expressed by
 * mapping characters to glyphs one at a time, and both are measured here against faces that do them:
 *
 *   - **DEVANAGARI.** `ka` + VIRAMA + `ssa` is three characters; FreeSerif sets the CONJUNCT in ONE glyph, and the
 *     cluster says the one glyph stands for all three. Nothing about the characters predicts that count.
 *   - **ARABIC.** Three letters shape to three glyphs, but the glyphs are all DIFFERENT from what each letter would
 *     be alone — that is joining, where a letter's form depends on its neighbours — and they come back in VISUAL
 *     order, which is the order the header documents for a right-to-left run: the clusters run 2, 1, 0. */
static int case_shaping_complex_scripts(void)
{
	int before = g_fail;
	CTFontRef font = shaping_font("/usr/share/fonts/truetype/freefont/FreeSerif.ttf", 13.0);
	CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
	/* KA, VIRAMA, SSA — the conjunct `kssa`. */
	UniChar deva[] = { 0x0915, 0x094D, 0x0937 };
	/* BEH, SEEN, MEEM — an Arabic word, shaped right to left. */
	UniChar arab[] = { 0x0628, 0x0633, 0x0645 };
	CTShapedRun run;

	CTShapedRunZero(&run);
	check(font != NULL && shaper != NULL, "the complex-script case could not make its face or its shaper");
	if (font == NULL || shaper == NULL) {
		if (shaper != NULL) CTShaperRelease(shaper);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	if (CTShaperShape(shaper, deva, 3, kCTWritingDirectionLeftToRight, "Deva", NULL, &run)) {
		printf("  (Devanagari ka+virama+ssa: 3 characters -> %ld glyph(s), first cluster %ld)\n",
		       (long)run.glyphCount, (long)((run.glyphCount > 0) ? run.clusters[0] : -1));
		check(run.glyphCount < 3, "a CONJUNCT is fewer glyphs than characters: %ld for 3",
		      (long)run.glyphCount);
		check(run.glyphCount > 0 && run.clusters[0] == 0, "and its first glyph stands for the character at 0");
	} else {
		check(0, "the Devanagari conjunct could not be shaped");
	}
	CTShapedRunFree(&run);
	if (CTShaperShape(shaper, arab, 3, kCTWritingDirectionRightToLeft, "Arab", NULL, &run)) {
		CGGlyph alone = 0;
		UniChar beh = 0x0628;

		(void)CTFontGetGlyphsForCharacters(font, &beh, &alone, 1);
		printf("  (Arabic beh+seen+meem: %ld glyphs, clusters %ld/%ld/%ld, first glyph %u against %u alone)\n",
		       (long)run.glyphCount, (long)((run.glyphCount > 0) ? run.clusters[0] : -1),
		       (long)((run.glyphCount > 1) ? run.clusters[1] : -1),
		       (long)((run.glyphCount > 2) ? run.clusters[2] : -1),
		       (unsigned)((run.glyphCount > 0) ? run.glyphs[0] : 0), (unsigned)alone);
		check(run.glyphCount == 3, "three Arabic letters shape to three glyphs");
		if (run.glyphCount == 3) {
			/* **VISUAL ORDER FOR A RIGHT-TO-LEFT RUN**, which is the contract the header states: the clusters run
			 * backwards, so the first glyph is the LAST character's form. */
			check(run.clusters[0] == 2 && run.clusters[1] == 1 && run.clusters[2] == 0,
			      "and they come back in VISUAL order, so the clusters run 2, 1, 0");
			/* **JOINING: the same letter is a different glyph in context.** `beh` in this word takes a form that is
			 * NOT the glyph the face gives for `beh` on its own, which is the whole of what joining means. */
			check(alone != 0 && run.glyphs[2] != alone,
			      "and the letter's form depends on its neighbours: %u in context against %u alone",
			      (unsigned)run.glyphs[2], (unsigned)alone);
		}
	} else {
		check(0, "the Arabic word could not be shaped");
	}
	CTShapedRunFree(&run);
	CTShaperRelease(shaper);
	CFRelease(font);
	return g_fail - before;
}

/* **AND THE EDGES, BECAUSE A DOOR THAT CAN BE REACHED WITH NOTHING MUST REFUSE RATHER THAN CRASH.** Every one of
 * these is a call a caller can make by accident: an unloaded face, a NULL shaper from a failed create, an empty run,
 * a NULL where the caller meant "guess", and a run released twice. */
static int case_shaping_edges(void)
{
	int before = g_fail;
	CTShapedRun run;
	UniChar abc[] = { 'a', 'b', 'c' };
	CTFontRef font = shaping_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13.0);
	CTShaperRef shaper = (font != NULL) ? CTShaperCreate(font) : NULL;
	UniChar highSurrogate = 0xD83D;

	CTShapedRunZero(&run);
	check(CTShaperCreate(NULL) == NULL, "a NULL font makes no shaper");
	CTShaperRelease(NULL);	/* must not crash */
	check(!CTShaperShape(NULL, abc, 3, kCTWritingDirectionLeftToRight, NULL, NULL, &run),
	      "a NULL shaper shapes nothing and says so");
	check(!CTShaperShape(shaper, NULL, 3, kCTWritingDirectionLeftToRight, NULL, NULL, &run),
	      "NULL characters shape nothing");
	check(!CTShaperShape(shaper, abc, 0, kCTWritingDirectionLeftToRight, NULL, NULL, &run),
	      "an EMPTY run shapes nothing, because there is nothing to shape");
	check(!CTShaperShape(shaper, abc, -1, kCTWritingDirectionLeftToRight, NULL, NULL, &run),
	      "a NEGATIVE length shapes nothing");
	/* A RUN OF ONE UNPAIRED SURROGATE is not text, and the answer is what the shaper can make of it rather than a
	 * crash — the same standing every other door in this library gives a caller who hands it a lone surrogate. */
	(void)CTShaperShape(shaper, &highSurrogate, 1, kCTWritingDirectionLeftToRight, NULL, NULL, &run);
	check(run.glyphCount >= 0, "a lone surrogate is shaped or refused, and does not crash");
	CTShapedRunFree(&run);
	/* **A NULL SCRIPT TAG MEANS "READ IT OFF THE CHARACTERS"**, which is the guess path, and it must agree with an
	 * explicit tag for a run that is one script — otherwise the guess would be a different answer rather than a
	 * convenience. */
	if (shaper != NULL) {
		CTShapedRun guessed, tagged;

		CTShapedRunZero(&guessed);
		CTShapedRunZero(&tagged);
		(void)CTShaperShape(shaper, abc, 3, kCTWritingDirectionLeftToRight, NULL, NULL, &guessed);
		(void)CTShaperShape(shaper, abc, 3, kCTWritingDirectionLeftToRight, "Latn", NULL, &tagged);
		check(guessed.glyphCount == tagged.glyphCount && guessed.glyphCount == 3,
		      "a guessed script and a stated `Latn` agree: %ld and %ld glyphs", (long)guessed.glyphCount,
		      (long)tagged.glyphCount);
		if (guessed.glyphCount == 3 && tagged.glyphCount == 3) {
			check(memcmp(guessed.glyphs, tagged.glyphs, sizeof(CGGlyph) * 3) == 0
			      && guessed.advances[0].width == tagged.advances[0].width,
			      "and the same glyphs with the same advances");
		}
		CTShapedRunFree(&guessed);
		CTShapedRunFree(&tagged);
	}
	/* AND A FACE THAT IS NOT THERE IS A REFUSAL, NOT A SHAPER OVER NOTHING. */
	check(CTShaperCreate(shaping_font("/nonexistent/none.ttf", 13.0)) == NULL,
	      "a face that cannot be opened makes no shaper");
	CTShaperRelease(shaper);
	if (font != NULL) {
		CFRelease(font);
	}
	return g_fail - before;
}

/* **THE CORRESPONDENCE BETWEEN A GLYPH AND THE CHARACTERS IT STANDS FOR, WHICH IS NOW A MAP RATHER THAN AN INDEX.**
 * The line holds `clusters` — one entry per glyph, saying which character it begins at — and every door that used to
 * compute "the glyph for character i" as "i" now reads that map: the two caret doors, and the string indices a run
 * reports.
 *
 * **THIS CASE IS A CONTRACT RATHER THAN A MEASUREMENT OF SHAPING, AND IT IS WRITTEN SO THAT IT SURVIVES SHAPING BEING
 * SWITCHED ON.** What it asserts is what is true of a NON-LIGATING string in a face that ligates nothing: one glyph
 * per character, each glyph's cluster its own index, and the run reporting 0..length-1. A string where the face DOES
 * ligate would give fewer glyphs — which is what the engine's own cases measure — so the case picks text with no
 * ligature pair in it and says why.
 *
 * AND THE DURABLE HALF IS THE CARET CONTRACT: for a left-to-right line the offsets for 0..length are NON-DECREASING
 * from 0 to the width, whatever the glyph count is, because a caret may stand at a cluster boundary rather than at a
 * character. That is true of the identity map and of a shaped one, which is why it is asserted here. */
static int case_cluster_map(void)
{
	int before = g_fail;
	CTFontRef font = test_font(13.0);
	/* `abc def` HAS NO LIGATURE PAIR IN IT, so one glyph per character is what ANY face answers for it. */
	UniChar text[] = { 'a', 'b', 'c', ' ', 'd', 'e', 'f' };
	CFStringRef str = (font != NULL) ? CFStringCreateWithCharacters(kCFAllocatorDefault, text, 7) : NULL;
	CFMutableDictionaryRef attrs = NULL;
	CFAttributedStringRef astr = NULL;

	check(font != NULL && str != NULL, "the cluster case could not be set up");
	if (font == NULL || str == NULL) {
		if (str != NULL) CFRelease(str);
		if (font != NULL) CFRelease(font);
		return g_fail - before;
	}
	attrs = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(attrs, kCTFontAttributeName, font);
	astr = CFAttributedStringCreate(kCFAllocatorDefault, str, attrs);
	{
		CTLineRef line = (astr != NULL) ? CTLineCreateWithAttributedString(astr) : NULL;

		check(line != NULL, "the cluster case's line was made");
		if (line != NULL) {
			CFArrayRef runs = CTLineGetGlyphRuns(line);
			CTRunRef run = (runs != NULL && CFArrayGetCount(runs) > 0)
				? (CTRunRef)CFArrayGetValueAtIndex(runs, 0) : NULL;
			const CFIndex *indices = (run != NULL) ? CTRunGetStringIndicesPtr(run) : NULL;
			CFIndex count = (run != NULL) ? CTRunGetGlyphCount(run) : 0;
			CFIndex i;
			Boolean identity = (count == 7) && (indices != NULL);

			for (i = 0; identity && i < 7; i++) {
				if (indices[i] != i) {
					identity = false;
				}
			}
			printf("  (a non-ligating string: %ld characters -> %ld glyphs, the run's indices %ld..%ld)\n",
			       (long)CTLineGetGlyphCount(line), (long)count,
			       (long)((indices != NULL && count > 0) ? indices[0] : -1),
			       (long)((indices != NULL && count > 0) ? indices[count - 1] : -1));
			check(identity, "seven characters with no ligature pair shape to seven glyphs, each reporting its own "
					"character as its string index");
			/* **THE CARET CONTRACT, WHICH IS WHAT SURVIVES SHAPING.** */
			{
				CGFloat previous = -1.0;
				Boolean ascending = true;

				for (i = 0; i <= 7; i++) {
					CGFloat at = CTLineGetOffsetForStringIndex(line, i, NULL);

					if (at < previous - 0.001) {
						ascending = false;
					}
					previous = at;
				}
				check(ascending, "and the caret offsets for 0..7 are non-decreasing, whatever the glyph count is");
				check(CTLineGetOffsetForStringIndex(line, 0, NULL) == 0.0,
				      "with the caret before the first character at the line's origin");
				check(fabs((double)CTLineGetOffsetForStringIndex(line, 7, NULL)
					   - CTLineGetTypographicBounds(line, NULL, NULL, NULL)) < 0.01,
				      "and the one past the last character at the line's width");
			}
			/* AND THE HIT TEST ANSWERS WITH A CHARACTER, not a glyph index: a point in the middle of the line
			 * answers a character the string actually has. */
			{
				CFIndex at = CTLineGetStringIndexForPosition(line, CGPointMake(
					(CGFloat)(CTLineGetTypographicBounds(line, NULL, NULL, NULL) / 2.0), 0.0));

				check(at >= 0 && at <= 7, "the hit test answers a character in 0..7, and said %ld", (long)at);
			}
			CFRelease(line);
		}
	}
	if (astr != NULL) CFRelease(astr);
	CFRelease(attrs);
	CFRelease(str);
	CFRelease(font);
	return g_fail - before;
}

static const struct { const char *name; int (*fn)(void); } g_cases[] = {
	{ "The symbolic traits carry Apple's published values", case_symbolic_trait_values },
	{ "The four trait keys are the AppKit's strings", case_trait_key_strings },
	{ "The descriptor's twenty-two key strings", case_descriptor_key_strings },
	{ "The descriptor is an object, and its identity is the runtime's", case_descriptor_object },
	{ "A font is a face, and the glyph step is the reason for it", case_font_object },
	{ "A glyph's outline is a path, scaled, matrixed and transformed", case_font_outline },
	{ "The collection is a question, and its value is that question", case_collection_object },
	{ "The collection's one option key is the inference it is recorded as", case_collection_option_key },
	{ "The error vocabulary, including Apple's duplicate 202", case_font_manager_errors },
	{ "The manager lists a registered face three ways", case_manager_listing },
	{ "The family-name comparator, including the recorded deviation", case_manager_comparator },
	{ "Registration round-trips, and a scope this tier refuses", case_manager_registration },
	{ "The auto-activation setting round-trips as a preference", case_manager_settings },
	{ "The paragraph vocabularies and the fourteen settings", case_paragraph_style_values },
	{ "A paragraph style answers defaults, copies and retains", case_paragraph_style_object },
	{ "A line is one run, and every measuring door agrees with the font", case_line_object },
	{ "A line draws, and the ink follows the text position", case_line_drawing },
	{ "The run agrees with its line, and draws over a range", case_run_object },
	{ "The typesetter's own surface, and its range rule", case_typesetter_object },
	{ "The typesetter breaks at the width, and the paragraph style says how", case_typesetter_breaking },
	{ "The typesetter's two bidi options are obeyed by the line", case_typesetter_bidi_options },
	{ "A frame fills a path, and its lines tile what it showed", case_frame_object },
	{ "The attribute keys, twelve proven and two inferred", case_string_attribute_keys },
	{ "The tab, and the default it was owed for", case_text_tab },
	{ "The glyph substitution, named three ways", case_glyph_info },
	{ "The umbrella, and the version of the surface it implements", case_umbrella_version },
	{ "A line truncates the way Apple prints it, and refuses another font", case_line_truncation },
	{ "A line justifies at the spaces, by the fraction it is given", case_line_justification },
	{ "A line is kerned: the font's pair adjustments reach the positions", case_line_kerning },
	{ "A run delegate is a box: the line reserves its width and draws no ink", case_run_delegate },
	{ "A tab lands at its stop, and the fit follows it there", case_text_tab_stops },
	{ "The base writing direction reaches the run as a status", case_base_writing_direction },
	{ "The bidi classes, and the paragraph level P2/P3 reads", case_bidi_paragraph_level },
	{ "The explicit levels: embeddings, overrides and isolates", case_bidi_explicit_levels },
	{ "The weak types W1-W7 over the level runs", case_bidi_weak_types },
	{ "The neutral types N1 and N2", case_bidi_neutrals },
	{ "The isolating run sequences X10 joins", case_bidi_isolating_sequences },
	{ "The implicit levels I1 and I2", case_bidi_implicit_levels },
	{ "The bracket pairs N0", case_bidi_brackets },
	{ "The reordering L1 and L2", case_bidi_reordering },
	{ "A right-to-left line's glyphs are placed right to left", case_line_bidi_reorder },
	{ "The mirrored glyph L4", case_bidi_mirroring },
	{ "A line holds two fonts, and each run is indexed in its own", case_line_font_runs },
	{ "And the FRAMESETTER path is kerned too, which is where the AppKit draws", case_frame_kerning },
	{ "Matching finds the faces a query describes, in a stable order", case_font_matching },
	{ "A disabled face leaves every matching answer, and comes back", case_font_visibility },
	{ "The two halves of the font service speak the same wire", case_font_service_wire },
	{ "The character set agrees with the mapping, code unit by code unit", case_font_character_set },
	{ "A table copies as the file holds it, and the directory agrees", case_font_tables },
	{ "The underline metrics come out of `post`, scaled to the point size", case_font_post_metrics },
	{ "The symbolic traits come out of OS/2, head and post", case_font_symbolic_traits },
	{ "A variation instance is the choice the font was asked for", case_font_variation },
	{ "The variation axes come out of `fvar`, and the case reads it too", case_font_variation_axes },
	{ "A feature setting is a tuple, and toggling one keeps the others", case_font_feature_settings },
	{ "Same family and these traits, and the family a style cannot cross", case_font_family_and_traits },
	{ "A cascade answers by list first, and a role answers by fact", case_font_cascade_and_roles },
	{ "The traits dictionary: four keys and one anchor each", case_font_traits_dictionary },
	{ "The slant angle is the face's plus the matrix's, in degrees", case_font_slant_angle },
	{ "The vertical origin is half the advance and the ascent", case_font_vertical_origin },
	{ "The glyph-to-character map the caret doors read", case_cluster_map },
	{ "A ligature is one glyph for three characters", case_shaping_ligatures },
	{ "A pair adjustment reaches the advances", case_shaping_kerning },
	{ "A combining mark, composed in one face and attached in another", case_shaping_marks },
	{ "A Devanagari conjunct and a joined Arabic word", case_shaping_complex_scripts },
	{ "The shaper's edges refuse rather than crash", case_shaping_edges },
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
