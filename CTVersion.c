/*
 * CTVersion.c — the one function the umbrella declares.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * **A FILE FOR ONE RETURN STATEMENT, AND IT IS THE DECLARATION THAT JUSTIFIES IT**: the value is a constant of
 * this tier's surface pin, and putting it beside the constants it is compared against — in the umbrella, as
 * Apple does — is what keeps the answer and the vocabulary from drifting apart. Plain C, because there is no
 * object here to be a class of.
 */
#include <CoreText/CoreText.h>

uint32_t CTGetCoreTextVersion(void)
{
	/* THE ERA THIS TIER IMPLEMENTS, and the constant rather than the literal so that the two cannot disagree:
	 * the surface is pinned to 10.6 (the plan's §2), and 10.6 is therefore what a caller is told they have. */
	return (uint32_t)kCTVersionNumber10_6;
}
