# S1 version 8 responsiveness candidates

Exact source commit: a9f09b9ddbb3b1ea45929dd81888fe661f4c3b6c.
Fetched base before editing: ac0e9b40a556012b42be240f1c32c88370028b4a.
The implementation source branch has not been published.
Version 7 is an intermediate, superseded candidate; no physical v7 run exists.

The C67 user verified and replacement-installed diagnostic v6, cold-launched
to Android's permission controller, and subsequently reported the app runs
but is slow/unresponsive. Three scheduling repairs now apply:
- update interval 200 ms to 33 ms (target, not achieved physical frame rate);
- render once after draining, using the newest complete retained window;
- bound the drain by the entire 32768-frame capture queue, not 8192 frames.

The two-window regression rejects v6. The stronger full-queue regression
rejects intermediate v7 and requires the last bin-16 window, after older
bin-8 windows, to reach rendering. No fresh PCM must produce no stale redraw.
Startup Fourier/GPU invariants remain authoritative and unchanged.

Fourier mathematical tests, success/failure startup tests, simulated Android
lifecycle, hosted production compute/fragment coefficient/framebuffer checks
and stale-upload/wrong-scale/stale-draw mutants pass. ASan/UBSan pass with leak
detection disabled because this environment blocks LeakSanitizer process
inspection. Mesa pbuffer evidence is not Android/PowerVR acceptance.
CPU-wall timing logs separate submission/audit/swap costs; no GPU timer claim.

Paired API26 NativeActivity builds use NDK r27c, the normal shared packager,
org.isomorphismes.fouriersound.voice, version 8, stable public Wegert development
signer. A1 ARMv7 remains primary; C67 arm64-v8a remains paired. Replace-install,
preserve data. Never uninstall to hide a signer conflict.

Release performs mandatory startup validation but no live readback.
Diagnostic additionally audits frame1/every20 live coefficients/framebuffer.
Compare exact variant performance separately.

| Variant | Target | Bytes | SHA256 |
| --- | --- | ---: | --- |
| diagnostic | A1 | 33219 | 463cfba61a099ae1169880a3ba8bacf4fc89ad3bff21090909da681861747d0a |
| diagnostic | C67 | 37313 | b8780473e224562020e0a89cc6cc5e3d8860a8c93fea987669e911a1e5f82b2c |
| release | A1 | 33219 | fdfe5af624f54f339fcac8dbffa38dff5347506f0959e3bb648877c48086ff7e |
| release | C67 | 33217 | 86a781645b57974af126fab8723a1e6801d64d021054775b8f74d96c14fd70f8 |

All v8 physical microphone/lifecycle/GPU/live-audio results are NOT_RUN.
A1 physical install/launch is also NOT_RUN. Producer promotion remains BLOCKED
by the unregistered Fourier Voice package/signer row. These APKs remain
acceptance candidates, not a complete Sun S1 acceptance receipt.
Adjacent TSV receipts identify exact ABI/version/native/signer/source digests.
Prior v6 and intermediate v7 bytes remain immutable in s1/ and s1-v7/.
