/*
 * CTParagraphStyle.m — the paragraph attributes, stored by value.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **THE OBJECT IS FOURTEEN SLOTS, EACH EITHER EMPTY OR A SIZE AND SOME BYTES**, which is the whole of what a
 * paragraph style needs to be: the values arrive as `{specifier, valueSize, value}` triples, so each one is
 * copied into a slot of its own and handed back the same way. A UNION WOULD HAVE BEEN THE OTHER DESIGN and
 * it is wrong here for a reason worth writing down: the value sizes differ per specifier (an alignment is one
 * byte, an indent is a `CGFloat`, a tab-stop array is a pointer), and a union sized for the largest would
 * still have to remember which member was live — which is a slot keyed by specifier, i.e. this.
 *
 * **THE DEFAULTS ARE THE SLOTS' ABSENCE, NOT A TABLE OF ZEROS.** `GetValueForSpecifier` answers `false` for
 * a specifier that was never set, and the *layout* is what decides what an absent setting means (the frame
 * reads Alignment and treats absent as natural). That keeps one source of truth for a default — the code
 * that consumes it — instead of two, which is the mistake that makes a style object disagree with the
 * renderer about what "unset" means.
 */
#import <Foundation/Foundation.h>

#include "CTParagraphStyle.h"

#include <stdlib.h>
#include <string.h>

#define CT_SPECIFIER_COUNT ((size_t)kCTParagraphStyleSpecifierCount)

@interface CTParagraphStyle : NSObject {
	@public
	void *values[CT_SPECIFIER_COUNT];	/* malloc'd copies, or NULL for "never set" */
	size_t sizes[CT_SPECIFIER_COUNT];
}
@end

@implementation CTParagraphStyle

- (void)dealloc
{
	size_t i;

	for (i = 0; i < CT_SPECIFIER_COUNT; i++) {
		free(values[i]);
	}
	[super dealloc];
}

@end

CTParagraphStyleRef CTParagraphStyleCreate(const CTParagraphStyleSetting *settings, size_t settingCount)
{
	CTParagraphStyle *style = [[CTParagraphStyle alloc] init];
	size_t i;

	if (style == nil) {
		return NULL;
	}
	for (i = 0; i < settingCount; i++) {
		size_t spec = (size_t)settings[i].spec;

		/* A SPECIFIER OUT OF RANGE OR A NULL VALUE IS IGNORED, NOT REFUSED — see CTParagraphStyle.h: the
		 * constructor has no way to report a failure, and dropping the settings that were fine would be a
		 * worse answer than ignoring the one that was not. */
		if (spec >= CT_SPECIFIER_COUNT || settings[i].value == NULL || settings[i].valueSize == 0) {
			continue;
		}
		style->values[spec] = malloc(settings[i].valueSize);
		if (style->values[spec] == NULL) {
			continue;
		}
		memcpy(style->values[spec], settings[i].value, settings[i].valueSize);
		style->sizes[spec] = settings[i].valueSize;
	}
	return (CTParagraphStyleRef)style;
}

CTParagraphStyleRef CTParagraphStyleCreateCopy(CTParagraphStyleRef paragraphStyle)
{
	CTParagraphStyle *copy;
	size_t i;

	if (paragraphStyle == NULL) {
		return NULL;
	}
	copy = [[CTParagraphStyle alloc] init];
	if (copy == nil) {
		return NULL;
	}
	for (i = 0; i < CT_SPECIFIER_COUNT; i++) {
		if (paragraphStyle->values[i] != NULL) {
			copy->values[i] = malloc(paragraphStyle->sizes[i]);
			if (copy->values[i] != NULL) {
				memcpy(copy->values[i], paragraphStyle->values[i], paragraphStyle->sizes[i]);
				copy->sizes[i] = paragraphStyle->sizes[i];
			}
		}
	}
	return (CTParagraphStyleRef)copy;
}

bool CTParagraphStyleGetValueForSpecifier(CTParagraphStyleRef paragraphStyle, CTParagraphStyleSpecifier spec,
					  size_t bufferSize, void *buffer)
{
	size_t s = (size_t)spec;

	if (paragraphStyle == NULL || buffer == NULL || s >= CT_SPECIFIER_COUNT) {
		return false;
	}
	/* **APPLE'S CONTRACT FOR THIS DOOR IS THE BUFFER SIZE, AND IT IS CHECKED RATHER THAN GUESSED**: the
	 * caller says how much room it has, the object says how much the value needs, and a mismatch is `false`
	 * with nothing written. That is also why the value is not returned by reference or grown — the door's
	 * shape is the contract. */
	if (paragraphStyle->values[s] == NULL || paragraphStyle->sizes[s] != bufferSize) {
		return false;
	}
	memcpy(buffer, paragraphStyle->values[s], bufferSize);
	return true;
}

CTParagraphStyleRef CTParagraphStyleRetain(CTParagraphStyleRef paragraphStyle)
{
	return (CTParagraphStyleRef)[(id)(uintptr_t)paragraphStyle retain];
}

void CTParagraphStyleRelease(CTParagraphStyleRef paragraphStyle)
{
	[(id)(uintptr_t)paragraphStyle release];
}
