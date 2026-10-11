/*
 * CTFontDescriptor.c — the attribute keys, which are the part of this header with a value at run time.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE TWENTY-TWO STRINGS ARE A RECORDED GUESS AND THE HEADER SAYS WHY AT LENGTH: Apple publishes no value
 * for any of these keys anywhere, eight of them are provably "NSFont<Suffix>Attribute" because the AppKit
 * declares the same eight and a dictionary has to cross between the frameworks, and the other fourteen have
 * no counterpart to check against. The user's decision is that all twenty-two take that form
 * (`dec-4795696b09d362f6`), recorded here where the values are rather than asserted as a fact.
 */
#include <CoreText/CTFontDescriptor.h>

const CFStringRef kCTFontURLAttribute = CFSTR("NSFontURLAttribute");
const CFStringRef kCTFontNameAttribute = CFSTR("NSFontNameAttribute");
const CFStringRef kCTFontDisplayNameAttribute = CFSTR("NSFontDisplayNameAttribute");
const CFStringRef kCTFontFamilyNameAttribute = CFSTR("NSFontFamilyNameAttribute");
const CFStringRef kCTFontStyleNameAttribute = CFSTR("NSFontStyleNameAttribute");
const CFStringRef kCTFontTraitsAttribute = CFSTR("NSFontTraitsAttribute");
const CFStringRef kCTFontVariationAttribute = CFSTR("NSFontVariationAttribute");
const CFStringRef kCTFontSizeAttribute = CFSTR("NSFontSizeAttribute");
const CFStringRef kCTFontMatrixAttribute = CFSTR("NSFontMatrixAttribute");
const CFStringRef kCTFontCascadeListAttribute = CFSTR("NSFontCascadeListAttribute");
const CFStringRef kCTFontCharacterSetAttribute = CFSTR("NSFontCharacterSetAttribute");
const CFStringRef kCTFontLanguagesAttribute = CFSTR("NSFontLanguagesAttribute");
const CFStringRef kCTFontBaselineAdjustAttribute = CFSTR("NSFontBaselineAdjustAttribute");
const CFStringRef kCTFontMacintoshEncodingsAttribute = CFSTR("NSFontMacintoshEncodingsAttribute");
const CFStringRef kCTFontFeaturesAttribute = CFSTR("NSFontFeaturesAttribute");
const CFStringRef kCTFontFeatureSettingsAttribute = CFSTR("NSFontFeatureSettingsAttribute");
const CFStringRef kCTFontFixedAdvanceAttribute = CFSTR("NSFontFixedAdvanceAttribute");
const CFStringRef kCTFontOrientationAttribute = CFSTR("NSFontOrientationAttribute");
const CFStringRef kCTFontFormatAttribute = CFSTR("NSFontFormatAttribute");
const CFStringRef kCTFontRegistrationScopeAttribute = CFSTR("NSFontRegistrationScopeAttribute");
const CFStringRef kCTFontPriorityAttribute = CFSTR("NSFontPriorityAttribute");
const CFStringRef kCTFontEnabledAttribute = CFSTR("NSFontEnabledAttribute");
