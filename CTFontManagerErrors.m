/*
 * CTFontManagerErrors.m — the error domain and the user-info key, whose values are ours.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **WHY A FILE FOR TWO CONSTANTS.** The same reason `CTStringAttributes.m` gives for its keys: Apple publishes
 * the NAME of a `CFStringRef` constant and never the string, and a domain is only ever compared with the
 * constant declared beside it — which is what makes an unpublished value safe. A `.m` rather than a `.c`
 * because the constant IS an `NSString` and a string literal is a Foundation object; `CGBase.h` teaches a C
 * consumer to spell the type, exactly as it does for the font's own names.
 *
 * AND THE VALUES ARE THE `CFError` SHAPE THIS FRAMEWORK WOULD USE: the DOMAIN is the fully-qualified
 * reverse-DNS name Apple's bundled frameworks spell, and the KEY is the name Apple's own doc block gives it
 * ("User info key to be used with CFError references returned from registration functions"), which is the
 * convention every other user-info key in Apple's frameworks follows.
 */
#import <Foundation/Foundation.h>

#include <CoreText/CTFontManagerErrors.h>

NSString *const kCTFontManagerErrorDomain = @"com.apple.CoreText.CTFontManagerErrorDomain";
NSString *const kCTFontManagerErrorFontURLsKey = @"kCTFontManagerErrorFontURLsKey";
