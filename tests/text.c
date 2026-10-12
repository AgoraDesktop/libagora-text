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
#include <CoreText/CTFontService_internal.h>
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
	 * AND THE SIGN IS WORTH RECORDING, BECAUSE THIS TIER'S DESCENT COMES BACK NEGATIVE. The reason is one layer
	 * down: the seam answers `CGFontGetDescent` with the engine's own `face->descender`, which is negative in
	 * FreeType's convention, and nothing on the way up changes it. WHETHER APPLE'S IS NEGATIVE TOO IS AN OPEN
	 * QUESTION THE ERA'S HEADERS DO NOT SETTLE — so it is recorded here as a finding for the tier that can answer
	 * it rather than as a sign invented in a case. The line reports what the font reports; that is the line's
	 * whole contract, and the assertion below is that contract and nothing more. */
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
	{ "A frame fills a path, and its lines tile what it showed", case_frame_object },
	{ "The attribute keys, twelve proven and two inferred", case_string_attribute_keys },
	{ "The tab, and the default it was owed for", case_text_tab },
	{ "The glyph substitution, named three ways", case_glyph_info },
	{ "The umbrella, and the version of the surface it implements", case_umbrella_version },
	{ "A line truncates the way Apple prints it, and refuses another font", case_line_truncation },
	{ "A line justifies at the spaces, by the fraction it is given", case_line_justification },
	{ "Matching finds the faces a query describes, in a stable order", case_font_matching },
	{ "A disabled face leaves every matching answer, and comes back", case_font_visibility },
	{ "The two halves of the font service speak the same wire", case_font_service_wire },
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
