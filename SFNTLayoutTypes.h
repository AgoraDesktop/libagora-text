/*
 * SFNTLayoutTypes.h — the OpenType layout vocabulary (feature types and their selectors).
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 */
#ifndef SFNT_LAYOUT_TYPES_H
#define SFNT_LAYOUT_TYPES_H

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
/* From SFNTLayoutTypes.h, in its own groups. */
enum {
	kAllTypographicFeaturesType = 0,
	kLigaturesType = 1,
	kCursiveConnectionType = 2,
	kLetterCaseType = 3,
	kVerticalSubstitutionType = 4,
	kLinguisticRearrangementType = 5,
	kNumberSpacingType = 6,
	kSmartSwashType = 8,
	kDiacriticsType = 9,
	kVerticalPositionType = 10,
	kFractionsType = 11,
	kOverlappingCharactersType = 13,
	kTypographicExtrasType = 14,
	kMathematicalExtrasType = 15,
	kOrnamentSetsType = 16,
	kCharacterAlternativesType = 17,
	kDesignComplexityType = 18,
	kStyleOptionsType = 19,
	kCharacterShapeType = 20,
	kNumberCaseType = 21,
	kTextSpacingType = 22,
	kTransliterationType = 23,
	kAnnotationType = 24,
	kKanaSpacingType = 25,
	kIdeographicSpacingType = 26,
	kUnicodeDecompositionType = 27,
	kRubyKanaType = 28,
	kCJKSymbolAlternativesType = 29,
	kIdeographicAlternativesType = 30,
	kCJKVerticalRomanPlacementType = 31,
	kItalicCJKRomanType = 32,
	kCJKRomanSpacingType = 103,
	kLastFeatureType = -1,
};

enum {
	kAllTypeFeaturesOnSelector = 0,
	kAllTypeFeaturesOffSelector = 1,
};

enum {
	kRequiredLigaturesOnSelector = 0,
	kRequiredLigaturesOffSelector = 1,
	kCommonLigaturesOnSelector = 2,
	kCommonLigaturesOffSelector = 3,
	kRareLigaturesOnSelector = 4,
	kRareLigaturesOffSelector = 5,
	kLogosOnSelector = 6,
	kLogosOffSelector = 7,
	kRebusPicturesOnSelector = 8,
	kRebusPicturesOffSelector = 9,
	kDiphthongLigaturesOnSelector = 10,
	kDiphthongLigaturesOffSelector = 11,
	kSquaredLigaturesOnSelector = 12,
	kSquaredLigaturesOffSelector = 13,
	kAbbrevSquaredLigaturesOnSelector = 14,
	kAbbrevSquaredLigaturesOffSelector = 15,
	kSymbolLigaturesOnSelector = 16,
	kSymbolLigaturesOffSelector = 17,
};

enum {
	kUnconnectedSelector = 0,
	kPartiallyConnectedSelector = 1,
	kCursiveSelector = 2,
};

enum {
	kUpperAndLowerCaseSelector = 0,
	kAllCapsSelector = 1,
	kAllLowerCaseSelector = 2,
	kSmallCapsSelector = 3,
	kInitialCapsSelector = 4,
	kInitialCapsAndSmallCapsSelector = 5,
};

enum {
	kSubstituteVerticalFormsOnSelector = 0,
	kSubstituteVerticalFormsOffSelector = 1,
};

enum {
	kLinguisticRearrangementOnSelector = 0,
	kLinguisticRearrangementOffSelector = 1,
};

enum {
	kMonospacedNumbersSelector = 0,
	kProportionalNumbersSelector = 1,
	kThirdWidthNumbersSelector = 2,
	kQuarterWidthNumbersSelector = 3,
};

enum {
	kWordInitialSwashesOnSelector = 0,
	kWordInitialSwashesOffSelector = 1,
	kWordFinalSwashesOnSelector = 2,
	kWordFinalSwashesOffSelector = 3,
	kLineInitialSwashesOnSelector = 4,
	kLineInitialSwashesOffSelector = 5,
	kLineFinalSwashesOnSelector = 6,
	kLineFinalSwashesOffSelector = 7,
	kNonFinalSwashesOnSelector = 8,
	kNonFinalSwashesOffSelector = 9,
};

enum {
	kShowDiacriticsSelector = 0,
	kHideDiacriticsSelector = 1,
	kDecomposeDiacriticsSelector = 2,
};

enum {
	kNormalPositionSelector = 0,
	kSuperiorsSelector = 1,
	kInferiorsSelector = 2,
	kOrdinalsSelector = 3,
};

enum {
	kNoFractionsSelector = 0,
	kVerticalFractionsSelector = 1,
	kDiagonalFractionsSelector = 2,
};

enum {
	kPreventOverlapOnSelector = 0,
	kPreventOverlapOffSelector = 1,
};

enum {
	kHyphensToEmDashOnSelector = 0,
	kHyphensToEmDashOffSelector = 1,
	kHyphenToEnDashOnSelector = 2,
	kHyphenToEnDashOffSelector = 3,
	kSlashedZeroOnSelector = 4,
	kSlashedZeroOffSelector = 5,
	kFormInterrobangOnSelector = 6,
	kFormInterrobangOffSelector = 7,
	kSmartQuotesOnSelector = 8,
	kSmartQuotesOffSelector = 9,
	kPeriodsToEllipsisOnSelector = 10,
	kPeriodsToEllipsisOffSelector = 11,
};

enum {
	kHyphenToMinusOnSelector = 0,
	kHyphenToMinusOffSelector = 1,
	kAsteriskToMultiplyOnSelector = 2,
	kAsteriskToMultiplyOffSelector = 3,
	kSlashToDivideOnSelector = 4,
	kSlashToDivideOffSelector = 5,
	kInequalityLigaturesOnSelector = 6,
	kInequalityLigaturesOffSelector = 7,
	kExponentsOnSelector = 8,
	kExponentsOffSelector = 9,
};

enum {
	kNoOrnamentsSelector = 0,
	kDingbatsSelector = 1,
	kPiCharactersSelector = 2,
	kFleuronsSelector = 3,
	kDecorativeBordersSelector = 4,
	kInternationalSymbolsSelector = 5,
	kMathSymbolsSelector = 6,
};

enum {
	kNoAlternatesSelector = 0,
};

enum {
	kDesignLevel1Selector = 0,
	kDesignLevel2Selector = 1,
	kDesignLevel3Selector = 2,
	kDesignLevel4Selector = 3,
	kDesignLevel5Selector = 4,
};

enum {
	kNoStyleOptionsSelector = 0,
	kDisplayTextSelector = 1,
	kEngravedTextSelector = 2,
	kIlluminatedCapsSelector = 3,
	kTitlingCapsSelector = 4,
	kTallCapsSelector = 5,
};

enum {
	kTraditionalCharactersSelector = 0,
	kSimplifiedCharactersSelector = 1,
	kJIS1978CharactersSelector = 2,
	kJIS1983CharactersSelector = 3,
	kJIS1990CharactersSelector = 4,
	kTraditionalAltOneSelector = 5,
	kTraditionalAltTwoSelector = 6,
	kTraditionalAltThreeSelector = 7,
	kTraditionalAltFourSelector = 8,
	kTraditionalAltFiveSelector = 9,
	kExpertCharactersSelector = 10,
};

enum {
	kLowerCaseNumbersSelector = 0,
	kUpperCaseNumbersSelector = 1,
};

enum {
	kProportionalTextSelector = 0,
	kMonospacedTextSelector = 1,
	kHalfWidthTextSelector = 2,
};

enum {
	kNoTransliterationSelector = 0,
	kHanjaToHangulSelector = 1,
	kHiraganaToKatakanaSelector = 2,
	kKatakanaToHiraganaSelector = 3,
	kKanaToRomanizationSelector = 4,
	kRomanizationToHiraganaSelector = 5,
	kRomanizationToKatakanaSelector = 6,
	kHanjaToHangulAltOneSelector = 7,
	kHanjaToHangulAltTwoSelector = 8,
	kHanjaToHangulAltThreeSelector = 9,
};

enum {
	kNoAnnotationSelector = 0,
	kBoxAnnotationSelector = 1,
	kRoundedBoxAnnotationSelector = 2,
	kCircleAnnotationSelector = 3,
	kInvertedCircleAnnotationSelector = 4,
	kParenthesisAnnotationSelector = 5,
	kPeriodAnnotationSelector = 6,
	kRomanNumeralAnnotationSelector = 7,
	kDiamondAnnotationSelector = 8,
	kInvertedBoxAnnotationSelector = 9,
	kInvertedRoundedBoxAnnotationSelector = 10,
};

enum {
	kFullWidthKanaSelector = 0,
	kProportionalKanaSelector = 1,
};

enum {
	kFullWidthIdeographsSelector = 0,
	kProportionalIdeographsSelector = 1,
	kHalfWidthIdeographsSelector = 2,
};

enum {
	kCanonicalCompositionOnSelector = 0,
	kCanonicalCompositionOffSelector = 1,
	kCompatibilityCompositionOnSelector = 2,
	kCompatibilityCompositionOffSelector = 3,
	kTranscodingCompositionOnSelector = 4,
	kTranscodingCompositionOffSelector = 5,
};

enum {
	kNoRubyKanaSelector = 0,
	kRubyKanaSelector = 1,
	kRubyKanaOnSelector = 2,
	kRubyKanaOffSelector = 3,
};

enum {
	kNoCJKSymbolAlternativesSelector = 0,
	kCJKSymbolAltOneSelector = 1,
	kCJKSymbolAltTwoSelector = 2,
	kCJKSymbolAltThreeSelector = 3,
	kCJKSymbolAltFourSelector = 4,
	kCJKSymbolAltFiveSelector = 5,
};

enum {
	kNoIdeographicAlternativesSelector = 0,
	kIdeographicAltOneSelector = 1,
	kIdeographicAltTwoSelector = 2,
	kIdeographicAltThreeSelector = 3,
	kIdeographicAltFourSelector = 4,
	kIdeographicAltFiveSelector = 5,
};

enum {
	kCJKVerticalRomanCenteredSelector = 0,
	kCJKVerticalRomanHBaselineSelector = 1,
};

enum {
	kNoCJKItalicRomanSelector = 0,
	kCJKItalicRomanSelector = 1,
	kCJKItalicRomanOnSelector = 2,
	kCJKItalicRomanOffSelector = 3,
};

enum {
	kHalfWidthCJKRomanSelector = 0,
	kProportionalCJKRomanSelector = 1,
	kDefaultCJKRomanSelector = 2,
	kFullWidthCJKRomanSelector = 3,
};

enum {
	kSFNTLookupSimpleArray = 0,
	kSFNTLookupSegmentSingle = 2,
	kSFNTLookupSegmentArray = 4,
	kSFNTLookupSingleTable = 6,
	kSFNTLookupTrimmedArray = 8,
};

enum {
	kSTClassEndOfText = 0,
	kSTClassOutOfBounds = 1,
	kSTClassDeletedGlyph = 2,
	kSTClassEndOfLine = 3,
	kSTSetMark = 0x8000,
	kSTNoAdvance = 0x4000,
	kSTMarkEnd = 0x2000,
	kSTLigActionMask = 0x3FFF,
	kSTRearrVerbMask = 0x000F,
};

enum {
	kSTXHasLigAction = 0x2000,
};

enum {
	kLCARTag = 0x6C636172,
	kLCARCurrentVersion = 0x00010000,
	kLCARLinearFormat = 0,
	kLCARCtlPointFormat = 1,
};

enum {
	kJUSTTag = 0x6A757374,
	kJUSTCurrentVersion = 0x00010000,
	kJUSTStandardFormat = 0,
	kJUSTnoGlyphcode = 0xFFFF,
	kJUSTpcDecompositionAction = 0,
	kJUSTpcUnconditionalAddAction = 1,
	kJUSTpcConditionalAddAction = 2,
	kJUSTpcGlyphStretchAction = 3,
	kJUSTpcDuctilityAction = 4,
	kJUSTpcGlyphRepeatAddAction = 5,
};

enum {
	kJUSTKashidaPriority = 0,
	kJUSTSpacePriority = 1,
	kJUSTLetterPriority = 2,
	kJUSTNullPriority = 3,
	kJUSTPriorityCount = 4,
};

enum {
	kJUSTOverridePriority = 0x8000,
	kJUSTOverrideLimits = 0x4000,
	kJUSTOverrideUnlimited = 0x2000,
	kJUSTUnlimited = 0x1000,
	kJUSTPriorityMask = 0x0003,
};

enum {
	kOPBDTag = 0x6F706264,
	kOPBDCurrentVersion = 0x00010000,
	kOPBDDistanceFormat = 0,
	kOPBDControlPointFormat = 1,
};

enum {
	kMORTTag = 0x6D6F7274,
	kMORTCurrentVersion = 0x00010000,
	kMORTCoverVertical = 0x8000,
	kMORTCoverDescending = 0x4000,
	kMORTCoverIgnoreVertical = 0x2000,
	kMORTCoverTypeMask = 0x000F,
	kMORTRearrangementType = 0,
	kMORTContextualType = 1,
	kMORTLigatureType = 2,
	kMORTSwashType = 4,
	kMORTInsertionType = 5,
	kMORTLigLastAction = (int)0x80000000,
	kMORTLigStoreLigature = 0x40000000,
	kMORTLigFormOffsetMask = 0x3FFFFFFF,
	kMORTLigFormOffsetShift = 2,
	kMORTraNoAction = 0,
	kMORTraxA = 1,
	kMORTraDx = 2,
	kMORTraDxA = 3,
	kMORTraxAB = 4,
	kMORTraxBA = 5,
	kMORTraCDx = 6,
	kMORTraDCx = 7,
	kMORTraCDxA = 8,
	kMORTraDCxA = 9,
	kMORTraDxAB = 10,
	kMORTraDxBA = 11,
	kMORTraCDxAB = 12,
	kMORTraCDxBA = 13,
	kMORTraDCxAB = 14,
	kMORTraDCxBA = 15,
	kMORTDoInsertionsBefore = 0x80,
	kMORTIsSplitVowelPiece = 0x40,
	kMORTInsertionsCountMask = 0x3F,
	kMORTCurrInsertKashidaLike = 0x2000,
	kMORTMarkInsertKashidaLike = 0x1000,
	kMORTCurrInsertBefore = 0x0800,
	kMORTMarkInsertBefore = 0x0400,
	kMORTMarkJustTableCountMask = 0x3F80,
	kMORTMarkJustTableCountShift = 7,
	kMORTCurrJustTableCountMask = 0x007F,
	kMORTCurrJustTableCountShift = 0,
	kMORTCurrInsertCountMask = 0x03E0,
	kMORTCurrInsertCountShift = 5,
	kMORTMarkInsertCountMask = 0x001F,
	kMORTMarkInsertCountShift = 0,
};

enum {
	kMORXTag = 0x6D6F7278,
	kMORXCurrentVersion = 0x00020000,
	kMORXCoverVertical = (int)0x80000000,
	kMORXCoverDescending = 0x40000000,
	kMORXCoverIgnoreVertical = 0x20000000,
	kMORXCoverTypeMask = 0x000000FF,
};

enum {
	kPROPTag = 0x70726F70,
	kPROPCurrentVersion = 0x00030000,
	kPROPPairOffsetShift = 8,
	kPROPPairOffsetSign = 7,
	kPROPIsFloaterMask = 0x8000,
	kPROPCanHangLTMask = 0x4000,
	kPROPCanHangRBMask = 0x2000,
	kPROPUseRLPairMask = 0x1000,
	kPROPPairOffsetMask = 0x0F00,
	kPROPRightConnectMask = 0x0080,
	kPROPZeroReserved = 0x0060,
	kPROPDirectionMask = 0x001F,
};

enum {
	kPROPLDirectionClass = 0,
	kPROPRDirectionClass = 1,
	kPROPALDirectionClass = 2,
	kPROPENDirectionClass = 3,
	kPROPESDirectionClass = 4,
	kPROPETDirectionClass = 5,
	kPROPANDirectionClass = 6,
	kPROPCSDirectionClass = 7,
	kPROPPSDirectionClass = 8,
	kPROPSDirectionClass = 9,
	kPROPWSDirectionClass = 10,
	kPROPONDirectionClass = 11,
	kPROPSENDirectionClass = 12,
	kPROPLREDirectionClass = 13,
	kPROPLRODirectionClass = 14,
	kPROPRLEDirectionClass = 15,
	kPROPRLODirectionClass = 16,
	kPROPPDFDirectionClass = 17,
	kPROPNSMDirectionClass = 18,
	kPROPBNDirectionClass = 19,
	kPROPNumDirectionClasses = 20,
};

enum {
	kTRAKTag = 0x7472616B,
	kTRAKCurrentVersion = 0x00010000,
	kTRAKUniformFormat = 0,
};

enum {
	kKERNTag = 0x6B65726E,
	kKERNCurrentVersion = 0x00010000,
	kKERNVertical = 0x8000,
	kKERNResetCrossStream = 0x8000,
	kKERNCrossStream = 0x4000,
	kKERNVariation = 0x2000,
	kKERNUnusedBits = 0x1F00,
	kKERNFormatMask = 0x00FF,
};

enum {
	kKERNOrderedList = 0,
	kKERNStateTable = 1,
	kKERNSimpleArray = 2,
	kKERNIndexArray = 3,
};

enum {
	kKERNLineStart = 0x00000001,
	kKERNLineEndKerning = 0x00000002,
	kKERNNoCrossKerning = 0x00000004,
	kKERNNotesRequested = 0x00000008,
	kKERNNoStakeNote = 1,
	kKERNCrossStreamResetNote = 2,
	kKERNNotApplied = 0x00000001,
};

enum {
	kBSLNTag = 0x62736C6E,
	kBSLNCurrentVersion = 0x00010000,
	kBSLNDistanceFormatNoMap = 0,
	kBSLNDistanceFormatWithMap = 1,
	kBSLNControlPointFormatNoMap = 2,
	kBSLNControlPointFormatWithMap = 3,
};

enum {
	kBSLNRomanBaseline = 0,
	kBSLNIdeographicCenterBaseline = 1,
	kBSLNIdeographicLowBaseline = 2,
	kBSLNHangingBaseline = 3,
	kBSLNMathBaseline = 4,
	kBSLNLastBaseline = 31,
	kBSLNNumBaselineClasses = kBSLNLastBaseline + 1,
	kBSLNNoBaselineOverride = 255,
};

/* ------------------------------------------------------------------------------------------------
 * **THE FEATURE-SET KEYS, WHICH ARE NOT ENUMERATORS AND SO ARE NOT PART OF THE GENERATED GROUPS ABOVE.**
 * In the era these live in `SFNTLayoutTypes.h` beside the vocabulary they describe, and a regeneration of
 * those groups must leave THIS block alone: it is hand-written, because a key is an `NSString` constant with
 * this tier's own value rather than a number the vocabulary fixes.
 * ---------------------------------------------------------------------------------------------- */
extern NSString *const kCTFontFeatureTypeIdentifierKey;
extern NSString *const kCTFontFeatureSelectorIdentifierKey;

#endif /* SFNT_LAYOUT_TYPES_H */
