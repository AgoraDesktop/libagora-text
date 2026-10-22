/*
 * Copyright © 2026 Kyle J. Cardoza.
 * SPDX-FileCopyrightText: 2026 Kyle J. Cardoza
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Portions of this file may derive from GNUstep; see THIRD-PARTY.md.
 */

/*
 * CTFontService.c -- THE WIRE. See CTFontService_internal.h for what it is FOR and why the format is what it is.
 *
 * AND WHAT IS NOT HERE IS AS DELIBERATE AS WHAT IS: no socket, no name, no run loop, no process. Those are Agora's
 * system layer's half of the conversation (FONT-SERVICE.md), and this file is the framework's half — so it can be
 * tested in-process, with no daemon anywhere in sight, which is exactly how the case that comes with it tests it.
 *
 * AND THREE THINGS HERE ARE CORRECTIONS RATHER THAN CHOICES, each found by that case or by reading it again afterwards:
 * the format is XML because the binary form was never measured and the case said so; the version is NOT RELEASED,
 * because it is a `CFSTR` — a CONSTANT, not an owned object, and releasing something you do not own is a bug whichever
 * way a particular Core Foundation takes it; and the decode RELEASES ITS PROPERTY LIST BEFORE IT RETURNS, which the
 * first draft did not (it returned the reply and leaked the request it had decoded). See PROVENANCE.md.
 */

#include <CoreText/CTFontService_internal.h>
#include <CoreText/CTFontDescriptor.h>

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFPropertyList.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFURL.h>

#include <stdlib.h>

/* THE KEYS, NAMED ONCE. They are the wire format, so they are here and nowhere else. */
#define kCTFontServiceVersionKey CFSTR("version")
#define kCTFontServiceAttributesKey CFSTR("attributes")
#define kCTFontServiceMatchesKey CFSTR("matches")

/* THE FORMAT IS NAMED EXPLICITLY. Asking for binary is what the first draft did, and binary's support by this tree's
 * property-list layer was never measured — the form a reader can also write is the one to ask for. */
static CFDataRef ct_service_serialize(CFPropertyListRef plist)
{
	return CFPropertyListCreateData(kCFAllocatorDefault, plist, kCFPropertyListXMLFormat_v1_0, 0, NULL);
}

static CFPropertyListRef ct_service_deserialize(CFDataRef data)
{
	if (data == NULL) {
		return NULL;
	}
	return CFPropertyListCreateWithData(kCFAllocatorDefault, data, 0, NULL, NULL);
}

static bool ct_service_request_is_spoken(CFDictionaryRef request)
{
	CFTypeRef version = CFDictionaryGetValue(request, kCTFontServiceVersionKey);

	/* THE STRING AND NOTHING ELSE. The first draft accepted a number too, on a theory about why the case had failed
	 * that turned out to be wrong (see PROVENANCE.md); a wire format with two spellings for one value is a wire format
	 * with two ways to disagree, and this one has one. */
	return version != NULL && CFGetTypeID(version) == CFStringGetTypeID()
		       && CFEqual(version, kCTFontServiceWireVersionString);
}

/* THE REQUESTER'S HALF: the attributes a client wants faces for, as the bytes that ask for them. */
CFDataRef CTFontServiceCopyRequestData(CFDictionaryRef attributes)
{
	CFMutableDictionaryRef request;
	CFDataRef data;

	if (attributes == NULL || CFGetTypeID(attributes) != CFDictionaryGetTypeID()) {
		return NULL;
	}
	request = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					    &kCFTypeDictionaryValueCallBacks);
	if (request == NULL) {
		return NULL;
	}
	/* THE VERSION GOES IN AS THE CONSTANT ITSELF, with no copy to own and so nothing to release: a `CFSTR` cannot be
	 * owned, and the first draft's `CFRelease` of one was a bug waiting for a Core Foundation that takes it. */
	CFDictionarySetValue(request, kCTFontServiceVersionKey, kCTFontServiceWireVersionString);
	CFDictionarySetValue(request, kCTFontServiceAttributesKey, attributes);
	data = ct_service_serialize(request);
	CFRelease(request);
	return data;
}

/* A DESCRIPTOR ATTRIBUTE AS SOMETHING A PROPERTY LIST CAN HOLD, WHICH IS NOT EVERYTHING AN ATTRIBUTE CAN BE.
 *
 * AND THIS IS THE CORRECTION THE CASE WAS WAITING FOR, FOUND BY ELIMINATION RATHER THAN BY LUCK: the request encoded
 * fine and the reply did not, and the difference between them is the ATTRIBUTES OF A MATCHED FACE — which carry
 * `kCTFontURLAttribute`, a **CFURL** OBJECT, and a property list cannot hold one. Four probes ruled out everything else
 * (a plain dictionary, a dictionary of dictionaries, a dictionary of arrays, an empty array, a string version, the
 * decoded type ids and `CFEqual` on decoded values all behave), and a CFURL is the one value type the wire was being
 * asked to carry that nothing had ever tried to serialize. THE WIRE CARRIES PROPERTY-LIST TYPES AND NOTHING ELSE, and a
 * URL crosses it AS ITS STRING, which is the form a client can turn back into one.
 *
 * THE KEYS ARE NOT TOUCHED: they are attribute NAMES, and a name is a string already. Only the values are converted,
 * and a value that is already a property-list type passes through as itself. */
static CFTypeRef ct_service_plist_value(CFTypeRef value)
{
	if (value == NULL) {
		return NULL;
	}
	if (CFGetTypeID(value) == CFURLGetTypeID()) {
		CFStringRef text = CFURLGetString((CFURLRef)value);

		return (text != NULL) ? CFRetain(text) : NULL;	/* +1, like every other answer here. */
	}
	return CFRetain(value);
}

static CFDictionaryRef ct_service_plist_attributes(CFDictionaryRef attributes)
{
	CFIndex count = CFDictionaryGetCount(attributes);
	const void **keys = (const void **)malloc((size_t)((count > 0) ? count : 1) * sizeof(void *));
	const void **values = (const void **)malloc((size_t)((count > 0) ? count : 1) * sizeof(void *));
	CFMutableDictionaryRef safe;
	CFIndex i;

	if (keys == NULL || values == NULL) {
		if (keys != NULL) {
			free(keys);
		}
		if (values != NULL) {
			free(values);
		}
		return NULL;
	}
	CFDictionaryGetKeysAndValues(attributes, keys, values);
	safe = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					 &kCFTypeDictionaryValueCallBacks);
	if (safe != NULL) {
		for (i = 0; i < count; i++) {
			CFTypeRef held = ct_service_plist_value(values[i]);

			if (held != NULL) {
				CFDictionarySetValue(safe, keys[i], held);
				CFRelease(held);
			}
		}
	}
	free(keys);
	free(values);
	return safe;
}

/* THE RESPONDER'S HALF, WHICH IS THE WHOLE ANSWER: the request's attributes become a query, the MATCHING TIER answers it
 * (this file decides nothing about what matches — it only says so in bytes), and the matched descriptors' attributes
 * become the reply. */
CFDataRef CTFontServiceCopyReplyData(CFDataRef requestData)
{
	CFPropertyListRef plist;
	CFTypeRef attributes;
	CFMutableArrayRef matches;
	CFMutableDictionaryRef reply;
	CTFontDescriptorRef query;
	CFArrayRef found;
	CFDataRef data = NULL;
	CFIndex i, count;

	plist = ct_service_deserialize(requestData);
	if (plist == NULL) {
		return NULL;	/* BYTES THAT ARE NOT A REQUEST, refused as such. */
	}
	if (CFGetTypeID(plist) != CFDictionaryGetTypeID()) {
		CFRelease(plist);
		return NULL;
	}
	if (!ct_service_request_is_spoken((CFDictionaryRef)plist)) {
		CFRelease(plist);	/* OR A VERSION THIS SERVICE DOES NOT SPEAK, which is a conversation it cannot hold. */
		return NULL;
	}
	attributes = CFDictionaryGetValue((CFDictionaryRef)plist, kCTFontServiceAttributesKey);
	if (attributes == NULL || CFGetTypeID(attributes) != CFDictionaryGetTypeID()) {
		CFRelease(plist);
		return NULL;	/* A REQUEST WITH NO QUESTION IN IT. */
	}

	matches = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	reply = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	if (matches == NULL || reply == NULL) {
		if (matches != NULL) {
			CFRelease(matches);
		}
		if (reply != NULL) {
			CFRelease(reply);
		}
		CFRelease(plist);
		return NULL;
	}
	query = CTFontDescriptorCreateWithAttributes((CFDictionaryRef)attributes);
	found = (query != NULL) ? CTFontDescriptorCreateMatchingFontDescriptors(query, NULL) : NULL;
	if (found != NULL) {
		count = CFArrayGetCount(found);
		for (i = 0; i < count; i++) {
			CFDictionaryRef what = (CFDictionaryRef)CTFontDescriptorCopyAttributes(
				(CTFontDescriptorRef)CFArrayGetValueAtIndex(found, i));

			if (what != NULL) {
				CFDictionaryRef safe = ct_service_plist_attributes(what);

				CFRelease(what);
				if (safe != NULL) {
					CFArrayAppendValue(matches, safe);
					CFRelease(safe);
				}
			}
		}
		CFRelease(found);
	}
	if (query != NULL) {
		CFRelease(query);
	}
	CFDictionarySetValue(reply, kCTFontServiceVersionKey, kCTFontServiceWireVersionString);
	CFDictionarySetValue(reply, kCTFontServiceMatchesKey, matches);
	data = ct_service_serialize(reply);
	CFRelease(reply);
	CFRelease(matches);
	CFRelease(plist);	/* AND THE DECODED REQUEST IS RELEASED ON THIS PATH TOO, which the first draft forgot. */
	return data;
}
