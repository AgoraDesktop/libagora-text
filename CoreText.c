/*
 * CoreText.c — the umbrella's one door: the framework's version.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * `kCTVersionNumber10_6`, AND THE ANSWER IS A CLAIM ABOUT THIS LIBRARY THAT IS WORTH STATING PLAINLY. Apple's door
 * returns "the version of the CoreText framework", and the number a caller compares it against selects a set of
 * CONSTANTS AND FUNCTIONS — which is what an API version IS. THIS LIBRARY IMPLEMENTS THE 10.6 SURFACE: every header
 * written from Apple's 10.6 headers, every value the era's, and every door either defined or owed with its reason.
 * So the number it answers is the era's own 10.6 number, which tells a caller who is comparing (as Apple's own note
 * recommends) that the API they are asking about is the API this library provides.
 *
 * WHAT IT DOES NOT MEAN, SAID HERE SO THAT THE NUMBER IS NOT READ AS MORE THAN IT IS: it is NOT a claim about the
 * platform underneath, and it is not "this is macOS 10.6" — Core Text on Agora runs on Linux or BSD, over Core
 * Foundation and Core Graphics built by this project, and the version exists so that a caller written against Apple's
 * framework can decide what to call. A 10.6 caller asking for 10.6 gets the answer it expects and finds the surface it
 * expects, because that surface is what is written here.
 *
 * AND THE DOOR IS DECLARED IN THE UMBRELLA AND NOWHERE ELSE, which is not an accident of organisation: a version of
 * the FRAMEWORK belongs to the header that means the whole framework.
 */
#include <CoreText/CoreText.h>

uint32_t CTGetCoreTextVersion(void)
{
	return kCTVersionNumber10_6;
}
