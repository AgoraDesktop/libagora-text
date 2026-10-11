/*
 * SFNTTypes.h — the SFNT table and platform vocabulary.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 */
#ifndef SFNT_TYPES_H
#define SFNT_TYPES_H

/*
 * **NAMES FROM THE ERA'S OWN PUBLIC HEADERS; VALUES AS THIS VOCABULARY FIXES THEM.** The standing grant
 * allows Apple's public headers to be read for declarations, and an enumerator's value is part of its
 * declaration — which matters here in a way it has not in this framework's other vocabularies: **these
 * numbers INDEX OpenType features and selectors, and the era's own type enum proves it by having GAPS** (7 and
 * 12 are absent from the sequence 6, 8, 9, 10, 11, 13). A generator that emitted declaration ORDER would name
 * a different feature than it meant to. The rule §36's headers state — the values are ours — holds where
 * nothing outside fixes them; where the vocabulary itself fixes them, the fixing wins. Apple's own comment
 * text is NOT copied: what follows is enumerator names and their values.
 */
/* From SFNTTypes.h, in its own groups. */
enum {
	cmapFontTableTag = 'cmap',
};

enum {
	kFontUnicodePlatform = 0,
	kFontMacintoshPlatform = 1,
	kFontReservedPlatform = 2,
	kFontMicrosoftPlatform = 3,
	kFontCustomPlatform = 4,
};

enum {
	kFontUnicodeDefaultSemantics = 0,
	kFontUnicodeV1_1Semantics = 1,
	kFontISO10646_1993Semantics = 2,
	kFontUnicodeV2_0BMPOnlySemantics = 3,
	kFontUnicodeV2_0FullCoverageSemantics = 4,
	kFontUnicodeV4_0VariationSequenceSemantics = 5,
};

enum {
	kFontRomanScript = 0,
	kFontJapaneseScript = 1,
	kFontTraditionalChineseScript = 2,
	kFontChineseScript = kFontTraditionalChineseScript,
	kFontKoreanScript = 3,
	kFontArabicScript = 4,
	kFontHebrewScript = 5,
	kFontGreekScript = 6,
	kFontCyrillicScript = 7,
	kFontRussian = kFontCyrillicScript,
	kFontRSymbolScript = 8,
	kFontDevanagariScript = 9,
	kFontGurmukhiScript = 10,
	kFontGujaratiScript = 11,
	kFontOriyaScript = 12,
	kFontBengaliScript = 13,
	kFontTamilScript = 14,
	kFontTeluguScript = 15,
	kFontKannadaScript = 16,
	kFontMalayalamScript = 17,
	kFontSinhaleseScript = 18,
	kFontBurmeseScript = 19,
	kFontKhmerScript = 20,
	kFontThaiScript = 21,
	kFontLaotianScript = 22,
	kFontGeorgianScript = 23,
	kFontArmenianScript = 24,
	kFontSimpleChineseScript = 25,
	kFontTibetanScript = 26,
	kFontMongolianScript = 27,
	kFontGeezScript = 28,
	kFontEthiopicScript = kFontGeezScript,
	kFontAmharicScript = kFontGeezScript,
	kFontSlavicScript = 29,
	kFontEastEuropeanRomanScript = kFontSlavicScript,
	kFontVietnameseScript = 30,
	kFontExtendedArabicScript = 31,
	kFontSindhiScript = kFontExtendedArabicScript,
	kFontUninterpretedScript = 32,
};

enum {
	kFontMicrosoftSymbolScript = 0,
	kFontMicrosoftStandardScript = 1,
	kFontMicrosoftUCS4Script = 10,
};

enum {
	kFontCustom8BitScript = 0,
	kFontCustom816BitScript = 1,
	kFontCustom16BitScript = 2,
};

enum {
	kFontEnglishLanguage = 0,
	kFontFrenchLanguage = 1,
	kFontGermanLanguage = 2,
	kFontItalianLanguage = 3,
	kFontDutchLanguage = 4,
	kFontSwedishLanguage = 5,
	kFontSpanishLanguage = 6,
	kFontDanishLanguage = 7,
	kFontPortugueseLanguage = 8,
	kFontNorwegianLanguage = 9,
	kFontHebrewLanguage = 10,
	kFontJapaneseLanguage = 11,
	kFontArabicLanguage = 12,
	kFontFinnishLanguage = 13,
	kFontGreekLanguage = 14,
	kFontIcelandicLanguage = 15,
	kFontMalteseLanguage = 16,
	kFontTurkishLanguage = 17,
	kFontCroatianLanguage = 18,
	kFontTradChineseLanguage = 19,
	kFontUrduLanguage = 20,
	kFontHindiLanguage = 21,
	kFontThaiLanguage = 22,
	kFontKoreanLanguage = 23,
	kFontLithuanianLanguage = 24,
	kFontPolishLanguage = 25,
	kFontHungarianLanguage = 26,
	kFontEstonianLanguage = 27,
	kFontLettishLanguage = 28,
	kFontLatvianLanguage = kFontLettishLanguage,
	kFontSaamiskLanguage = 29,
	kFontLappishLanguage = kFontSaamiskLanguage,
	kFontFaeroeseLanguage = 30,
	kFontFarsiLanguage = 31,
	kFontPersianLanguage = kFontFarsiLanguage,
	kFontRussianLanguage = 32,
	kFontSimpChineseLanguage = 33,
	kFontFlemishLanguage = 34,
	kFontIrishLanguage = 35,
	kFontAlbanianLanguage = 36,
	kFontRomanianLanguage = 37,
	kFontCzechLanguage = 38,
	kFontSlovakLanguage = 39,
	kFontSlovenianLanguage = 40,
	kFontYiddishLanguage = 41,
	kFontSerbianLanguage = 42,
	kFontMacedonianLanguage = 43,
	kFontBulgarianLanguage = 44,
	kFontUkrainianLanguage = 45,
	kFontByelorussianLanguage = 46,
	kFontUzbekLanguage = 47,
	kFontKazakhLanguage = 48,
	kFontAzerbaijaniLanguage = 49,
	kFontAzerbaijanArLanguage = 50,
	kFontArmenianLanguage = 51,
	kFontGeorgianLanguage = 52,
	kFontMoldavianLanguage = 53,
	kFontKirghizLanguage = 54,
	kFontTajikiLanguage = 55,
	kFontTurkmenLanguage = 56,
	kFontMongolianLanguage = 57,
	kFontMongolianCyrLanguage = 58,
	kFontPashtoLanguage = 59,
	kFontKurdishLanguage = 60,
	kFontKashmiriLanguage = 61,
	kFontSindhiLanguage = 62,
	kFontTibetanLanguage = 63,
	kFontNepaliLanguage = 64,
	kFontSanskritLanguage = 65,
	kFontMarathiLanguage = 66,
	kFontBengaliLanguage = 67,
	kFontAssameseLanguage = 68,
	kFontGujaratiLanguage = 69,
	kFontPunjabiLanguage = 70,
	kFontOriyaLanguage = 71,
	kFontMalayalamLanguage = 72,
	kFontKannadaLanguage = 73,
	kFontTamilLanguage = 74,
	kFontTeluguLanguage = 75,
	kFontSinhaleseLanguage = 76,
	kFontBurmeseLanguage = 77,
	kFontKhmerLanguage = 78,
	kFontLaoLanguage = 79,
	kFontVietnameseLanguage = 80,
	kFontIndonesianLanguage = 81,
	kFontTagalogLanguage = 82,
	kFontMalayRomanLanguage = 83,
	kFontMalayArabicLanguage = 84,
	kFontAmharicLanguage = 85,
	kFontTigrinyaLanguage = 86,
	kFontGallaLanguage = 87,
	kFontOromoLanguage = kFontGallaLanguage,
	kFontSomaliLanguage = 88,
	kFontSwahiliLanguage = 89,
	kFontRuandaLanguage = 90,
	kFontRundiLanguage = 91,
	kFontChewaLanguage = 92,
	kFontMalagasyLanguage = 93,
	kFontEsperantoLanguage = 94,
	kFontWelshLanguage = 128,
	kFontBasqueLanguage = 129,
	kFontCatalanLanguage = 130,
	kFontLatinLanguage = 131,
	kFontQuechuaLanguage = 132,
	kFontGuaraniLanguage = 133,
	kFontAymaraLanguage = 134,
	kFontTatarLanguage = 135,
	kFontUighurLanguage = 136,
	kFontDzongkhaLanguage = 137,
	kFontJavaneseRomLanguage = 138,
	kFontSundaneseRomLanguage = 139,
};

enum {
	kFontNoPlatformCode = (unsigned int)(~0),
	kFontNoScriptCode = (unsigned int)(~0),
	kFontNoLanguageCode = (unsigned int)(~0),
};

enum {
	nameFontTableTag = 'name',
};

enum {
	kFontCopyrightName = 0,
	kFontFamilyName = 1,
	kFontStyleName = 2,
	kFontUniqueName = 3,
	kFontFullName = 4,
	kFontVersionName = 5,
	kFontPostscriptName = 6,
	kFontTrademarkName = 7,
	kFontManufacturerName = 8,
	kFontDesignerName = 9,
	kFontDescriptionName = 10,
	kFontVendorURLName = 11,
	kFontDesignerURLName = 12,
	kFontLicenseDescriptionName = 13,
	kFontLicenseInfoURLName = 14,
	kFontPreferredFamilyName = 16,
	kFontPreferredSubfamilyName = 17,
	kFontMacCompatibleFullName = 18,
	kFontSampleTextName = 19,
	kFontPostScriptCIDName = 20,
	kFontLastReservedName = 255,
};

enum {
	kFontNoNameCode = (unsigned int)(~0),
};

enum {
	variationFontTableTag = 'fvar',
};

enum {
	descriptorFontTableTag = 'fdsc',
};

enum {
	featureFontTableTag = 'feat',
};

enum {
	os2FontTableTag = 'OS/2',
};

enum {
	kFontNoPlatform = (unsigned int)(~0),
	kFontNoScript = (unsigned int)(~0),
	kFontNoLanguage = (unsigned int)(~0),
	kFontNoName = (unsigned int)(~0),
};

/* **AND THE TWELVE SIZES, WHICH ARE THE HALF OF THE SFNT LAYER THAT IS *PUBLISHED*.** §37 measured this
 * vocabulary and concluded that its struct layouts "stay open and honestly owed" because "a struct layout is
 * not a name: it is the font FILE's binary format, which in this tree belongs to the font engine". **THAT IS
 * RIGHT ABOUT THE STRUCTS AND WRONG ABOUT THESE TWELVE**: §37’s own subject was the 388 `property` and 93
 * `struct` rows, and among them sat twelve ENUMERATOR NAMES WHOSE VALUES THE ERA PRINTS — `sizeof_sfntDirectory
 * = 12`, `nonGlyphID = 65535`, and ten more like them. A size is a number, not a layout; shipping it needs no
 * struct, and the era fixes every one. So the structs stay owed (they are a font-file format this tier's engine
 * owns) and the sizes ship, which is this file's own rule applied honestly: **a value is ours only where
 * nothing outside fixes it.**
 *
 * **AND ONE HOME DEVIATION IS RECORDED RATHER THAN HIDDEN**: in the era these live in
 * `ATS.framework/…/SFNTTypes.h` — Apple Type Services, NOT CoreText, whose framework has no SFNTTypes.h at all
 * — and this tree has no ATS framework. The SFNT vocabulary was re-homed into `CoreText/` here (the deviation
 * §29 records), so the twelve go where their neighbours already are; a tree with an ATS.framework would put
 * them back, and the values would be identical either way.
 *
 * The values are the era's, in the era's own order, and the ORDER is Apple’s grouping rather than this file’s:
 * each sizes one of the SFNT structs, which are the half that stays owed. */
enum {
	sizeof_sfntDirectory = 12,

	sizeof_sfntCMapHeader = 4,
	sizeof_sfntCMapSubHeader = 6,
	sizeof_sfntCMapExtendedSubHeader = 12,
	sizeof_sfntCMapEncoding = 8,

	sizeof_sfntNameHeader = 6,
	sizeof_sfntNameRecord = 12,

	sizeof_sfntVariationHeader = 16,
	sizeof_sfntVariationAxis = 20,
	sizeof_sfntInstance = 4,

	sizeof_sfntDescriptorHeader = 8,

	nonGlyphID = 65535
};

#endif /* SFNT_TYPES_H */
