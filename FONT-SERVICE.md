# The Agora font service

**DESIGNED, NOT BUILT.** This file records the decision to define an Agora font service — taken because
`CTFontManagerCreateFontRequestRunLoopSource` is the last door this library owes and the thing it waits for is a
system, not a tier — and the shape that service will have. **NOTHING DESCRIBED BELOW IS IMPLEMENTED.** The door is still
declared and not defined, so reaching for it is still a link error, and the `owed` test still names it. This file is a
PLAN, and it says so here because a document that reads like an implementation is the failure this tree records at
length in `PROVENANCE.md`.

## What the door is

Apple's own comment (10.6 headers):

> Creates a CFRunLoopSourceRef that will be used to convey font requests **from CTFontManager**. … Will return NULL on
> error, in the case of **a duplicate requestPortName**, or invalid context structure.

The direction is therefore the **opposite** of what "font server" suggests: **the application answers, and the system
asks.** A process adds the source to its run loop; another process asks "give me the descriptors matching these
attributes"; the block fires with `(requestAttributes, pid_t requestingProcess)` and returns the array. The `pid_t` is
the tell: the request arrives **from somebody else**, and the answer may legitimately depend on **who** is asking. And
the error case names the missing infrastructure exactly: the framework registers **a port under a name**, in a namespace
a requester can look up.

## What Agora already has — verified, not assumed

Measured with `nm -D ../build-prefix/lib/libagora-core.so` and `grep`, this session:

| piece | what Agora has | where it leaves the design |
|---|---|---|
| port namespace | `CFSocketRegisterSocketSignature`, `CFSocketCopyRegisteredSocketSignature`, `CFSocketGetDefaultNameRegistryPortNumber` | **the name registry already exists as an API** — this is the "requestPortName" half |
| run-loop integration | `CFSocketCreateRunLoopSource`, and the whole `CFRunLoop` family | the door's mechanism is **not blocked by our Core Foundation** |
| a socket to wrap | `CFSocketCreate`, `CFSocketCreateWithNative`, `CFSocketConnectToAddress` | BSD sockets are reachable without leaving Core Foundation |
| wire codec | `CFPropertyListCreateData`, `CFPropertyListCreateWithData` | a property list is the only **CF-native** encoding, and `libagora-text` is Core Foundation + Core Graphics **only** — so Foundation's `NSPropertyListSerialization` is not available here at any price |
| a name to derive from | `kCTFontManagerBundleIdentifier` (declared in `CTFontManager.h`) | the service name comes from an Apple-era key rather than an invention |

**AND ONE THING IS NOT VERIFIED, WHICH IS THE FIRST THING TO CHECK:** those `CFSocket` registry *symbols* are exported,
but whether a **runtime rendezvous** stands behind them — a service that actually answers a lookup — is in
`libagora-core`, a different repository, and was not read. If the registry is a stub, then the namespace is the first
thing the service has to build and the rest of this design is unchanged.

**AND `CFMessagePort` IS ABSENT** (`nm -D` finds none), so there is no Apple-era named-message primitive to lean on:
the transport is `CFSocket`, because it is the one IPC mechanism Agora's Core Foundation actually has. Note also that
`window-server/` is an **empty skeleton** (one commit, a `.gitignore`, no README), so this would be **Agora's first
server**, not a second one.

## The three pieces

1. **The responder — this library.** The door itself: derive a port name, listen on a `CFSocket`, wrap it with
   `CFSocketCreateRunLoopSource`, decode the request, answer it by calling the matching tier, encode the reply. The
   answer is literally what `CTFontDescriptorCreateMatchingFontDescriptors` returns — which is why this door lives in
   the tier that owns matching.
2. **The directory and requester — a new component** (a sibling of `window-server`). ITS EXISTENCE IS NOT A PREFERENCE:
   a responder registers a port under a name **derived from its process** (Apple's error case is a *duplicate*
   `requestPortName`, so the name is derived rather than supplied), and **only a system component can enumerate
   processes to find another process's port.** A random application cannot discover a random application. That is the
   whole job of the daemon, and it is the honest reason for the daemon to exist at all.
3. **The protocol** — below.

## The wire

**Request**: a property-list dictionary, at least `{"version": <number>, "attributes": <dictionary>}`. The version
exists from the first byte because a wire format without one has to be changed by breaking it.

**Reply**: a property-list **array of attribute dictionaries**, one per matched descriptor, in the order the matching
tier returned them — **the order is part of the answer**, and the tier's own decision (by family name) travels with it.

**AND THE ENCODING IS LOSSLESS FOR THIS LIBRARY'S DESCRIPTORS, WHICH IS NOT AN ACCIDENT**: a descriptor here holds
exactly the attributes it was built with — a caller's, or the three facts this tier reads off a face — so serializing
its attributes IS serializing the descriptor, with nothing left over. That property is why the wire can be a plain
property list, and it is the same property that made the disabled-face fix work (`CTFontMatching.c`'s list of queries).

**Identity**: `pid_t requestingProcess` is taken from the **socket** (`SO_PEERCRED`), never from the wire. A peer that
states its own pid is stating something unverified, and this parameter is what a responder would use to decide whether
to answer at all.

## The first unit, when it is built

**The responder half, behind a seam, testable in-process — no daemon, no name registry.** The source, the request
decode, the answer, the reply encode, with the transport sitting behind an internal interface exactly as this library's
other seams sit. A test can then hand the responder a request and check the descriptors come back through the codec,
which is the same shape as every other seam here, and it keeps **the difference between "the framework's half of the
conversation" and "the system's half"** visible instead of invented.

## Open decisions

1. Whether `CFSocket`'s name registry has a runtime behind it — read `libagora-core` and find out.
2. The daemon's component name and where it sits relative to `window-server`.
3. Whether the daemon also **serves** fonts of its own, or only asks processes what they have (Apple's own service does
   both, which is what the "duplicate requestPortName" error implies about per-process ports).
