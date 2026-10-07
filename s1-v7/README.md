# S1 version 7 acceptance candidates

Source commit: 314ce39c78ae40930ce4a866c26ab8ffd1f2900f.
Fetched application base: ac0e9b40a556012b42be240f1c32c88370028b4a.
The implementation source branch has not been published.

The C67 user verified and installed v6 diagnostic, cold launched to the
permission controller, then reported slow/unresponsive operation.
Version 7 removes the 200 ms display cap, drains the bounded capture queue
before rendering the newest window once, and refuses stale redraw without
new samples. A deterministic two-window regression rejects the old code.
33 ms is a scheduling target, not a physical frame-rate receipt.
CPU-wall timing separates FFT submission, audits, draw submission and swap.

Existing Fourier mathematics, authoritative startup failure injection,
simulated lifecycle tests, hosted production GLSL coefficient/framebuffer
checks and their stale-upload/wrong-scale/stale-display mutants pass.
ASan/UBSan pass with leak detection disabled because this hosted environment
does not permit LeakSanitizer's process inspection.

Both release and diagnostic variants are paired for MIRO A1 (armeabi-v7a,
primary) and MIRO C67 (arm64-v8a). NDK r27c, shared NativeActivity packager,
package org.isomorphismes.fouriersound.voice, version 7, stable Wegert public
development signer. The diagnostic keeps frame 1/every20 live readback audits;
release has startup validation but no live readback. Replace install, preserve
data. Never uninstall to bypass a signer conflict.

All v7 physical microphone/lifecycle/GPU/live-audio acceptance is NOT_RUN.
A1 physical install/launch is also NOT_RUN. Producer promotion is BLOCKED by
the unregistered Fourier Voice package/signer row. These are reviewable
candidates, not a claim that the full Cat Food distribution or S1 is accepted.

| Variant | Target | Bytes | SHA256 |
| --- | --- | ---: | --- |
| diagnostic | A1 | 33219 | 00d158329a37a4f5c48179837f043c5c08dfd3d97094b3d40e22f1afcc8383d7 |
| diagnostic | C67 | 37313 | bd3e19eb2b976df5222021ae04ce7e91b382f92bbe97c171bf502a1609e079ee |
| release | A1 | 33219 | 3ff43d750197499b5734dda7dc071baea05a77802cd1e2dc8aefc053c215659e |
| release | C67 | 33217 | 2f43d4c740ebdf1715d365c3aa3873371ef7710429e15e06ace5d01a56b418ba |

Each APK has an adjacent build receipt with ABI/version/native digest/signer
and exact source commit. v6 artifacts remain available under s1/.
