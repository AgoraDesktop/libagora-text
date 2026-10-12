#ifndef CORE_TEXT_CTFONTSERVICE_INTERNAL_H
#define CORE_TEXT_CTFONTSERVICE_INTERNAL_H

/*
 * THE WIRE, AND NOT THE TRANSPORT.
 *
 * `CTFontManagerCreateFontRequestRunLoopSource` is the one door this library still owes, and its reason is no longer "a
 * FONT SERVER on the other end": FONT-SERVICE.md records the service this project decided to define, and this file is
 * THE PART OF IT THAT BELONGS TO THE FRAMEWORK — the conversation itself, as bytes, with no process, no port and no
 * run loop in it. THE TRANSPORT IS AGORA'S SYSTEM LAYER'S HALF, and the two halves meet HERE rather than in each
 * other's code.
 *
 * TWO DOORS, ONE FOR EACH END OF THE CONVERSATION, so that both sides encode and decode with the same code and neither
 * can drift from the other without this file's test failing:
 *
 *   CTFontServiceCopyRequestData   A REQUESTER'S HALF: attributes in, the bytes that ask for them out. A font service
 *                                  daemon sends these to a process that serves fonts.
 *   CTFontServiceCopyReplyData     A RESPONDER'S HALF, AND IT IS THE WHOLE ANSWER: bytes in, the bytes of the matched
 *                                  descriptors out, produced by calling the MATCHING TIER (which is why the framework's
 *                                  half of this conversation lives in the library that owns matching at all).
 *
 * THE FORMAT IS A PROPERTY LIST, and that is a constraint rather than a taste: this library is CORE FOUNDATION AND CORE
 * GRAPHICS ONLY, so Foundation's property-list codec is unavailable to it at any price, and `CFPropertyList` is the one
 * CF-native encoding there is. AND IT IS WRITTEN AS **XML**, explicitly: the form a reader can also write is the one to
 * ask for, and the binary form's support was never measured — the first draft ASKED FOR BINARY and this file's test is
 * what said no (see PROVENANCE.md).
 *
 *   A REQUEST is a dictionary:  { "version": <string>, "attributes": <dictionary> }
 *   A REPLY   is a dictionary:  { "version": <string>, "matches":    <array of attribute dictionaries> }
 *
 * THE VERSION IS CARRIED IN BOTH DIRECTIONS FROM THE FIRST BYTE, because a wire format without one can only ever be
 * changed by breaking it, and because a REPLY THAT DOES NOT SAY WHAT IT SPEAKS cannot be trusted by a client that speaks
 * something else. IT IS THE STRING AND NOT A NUMBER because a number has a REPRESENTATION to agree on and a string does
 * not: the two ends of this conversation are separate components with separate property-list
 * implementations, and the simplest type both are certain of is the one to put on the wire.
 *
 * AND THE ORDER OF `matches` IS PART OF THE ANSWER: it is the matching tier's own order (by family name, the recorded
 * decision in `CTFontMatching_internal.h`), sent as it stands rather than re-sorted here — a reply is an ANSWER, not a
 * rearrangement of one.
 *
 * AND THE ENCODING IS LOSSLESS, WHICH IS NOT A COINCIDENCE: a descriptor in this library holds exactly the attributes it
 * was built with, so a match's ATTRIBUTES ARE THE MATCH, with nothing left over to serialize. That is the same property
 * the matcher already keys on, and it is why a plain property list is enough here.
 *
 * LASTLY, THE TWO FAILURES ARE DIFFERENT AND THE CODE SAYS SO: bytes this service cannot read, or a version it does not
 * speak, are REFUSED with NULL — "I cannot read what you sent" — while a request it understands perfectly well that
 * describes no installed face is ANSWERED, with an empty `matches` array. Collapsing those two into one answer would
 * make a client unable to tell a broken conversation from an empty one.
 */

#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>

#ifdef __cplusplus
extern "C" {
#endif

/* THE VERSION THIS LIBRARY SPEAKS, IN THE TWO FORMS IT NEEDS: the number it means, and THE STRING IT TRAVELS AS. They
 * are one fact and they sit together for that reason — a change to one is a change to the other. */
enum { kCTFontServiceWireVersion = 1 };
#define kCTFontServiceWireVersionString CFSTR("1")

/* THE TWO HALVES. Both RETURN A +1 REFERENCE, and both answer NULL when they cannot do their job (a NULL or malformed
 * argument, or, for the reply, a request this version does not speak). */
CFDataRef CTFontServiceCopyRequestData(CFDictionaryRef attributes);
CFDataRef CTFontServiceCopyReplyData(CFDataRef requestData);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TEXT_CTFONTSERVICE_INTERNAL_H */
