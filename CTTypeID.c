/*
 * CTTypeID — every `CT…GetTypeID` door in this library, in one file.
 *
 * Copyright © 2026 Kyle J. Cardoza. MIT licensed — see LICENSE.
 * SPDX-License-Identifier: MIT
 *
 * THE SHAPE IS `CGTypeID.c`'s, ONE FRAMEWORK OVER, FOR THE REASON THAT FILE GIVES: "a door defined beside its
 * class would put the numbers in eleven places and make them agree by review". So the eleven doors live here,
 * each a constant out of `CTTypeID_internal.h`, and the two properties the API promises are properties of the
 * TABLE rather than of any call site.
 *
 * AND IT IS ONE FILE WHILE THE OBJECTS ARE ELEVEN: a type identity is not a property of an object, it is a
 * property of the CLASS, so nothing here needs an instance or a class object — which is why this is plain C in
 * a library whose objects are all Objective-C. */
#include <CoreText/CTTypeID_internal.h>

#include <CoreText/CTFont.h>
#include <CoreText/CTFontCollection.h>
#include <CoreText/CTFontDescriptor.h>
#include <CoreText/CTFrame.h>
#include <CoreText/CTFramesetter.h>
#include <CoreText/CTGlyphInfo.h>
#include <CoreText/CTLine.h>
#include <CoreText/CTParagraphStyle.h>
#include <CoreText/CTRun.h>
#include <CoreText/CTTextTab.h>
#include <CoreText/CTTypesetter.h>

CFTypeID CTFontGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTFONT;
}

CFTypeID CTFontDescriptorGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTFONTDESCRIPTOR;
}

CFTypeID CTFontCollectionGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTFONTCOLLECTION;
}

CFTypeID CTFrameGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTFRAME;
}

CFTypeID CTFramesetterGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTFRAMESETTER;
}

CFTypeID CTGlyphInfoGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTGLYPHINFO;
}

CFTypeID CTLineGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTLINE;
}

CFTypeID CTParagraphStyleGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTPARAGRAPHSTYLE;
}

CFTypeID CTRunGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTRUN;
}

CFTypeID CTTextTabGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTTEXTTAB;
}

CFTypeID CTTypesetterGetTypeID(void)
{
	return (CFTypeID)CT_TYPE_ID_CTTYPESETTER;
}
