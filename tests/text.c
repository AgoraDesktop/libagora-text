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
#include <CoreText/CTFont.h>
/* AND THE ENGINE'S INTERNAL HEADER, FOR THE DOORS CORE TEXT OWNS BUT DOES NOT IMPLEMENT: the Unicode
 * glyph step and the face registry. */
#include <CoreGraphics/CGFont_internal.h>
#include <CoreText/CTFontCollection.h>
#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFontManager.h>
#include <CoreText/CTFontManagerErrors.h>
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
