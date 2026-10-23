# Fourier Voice continuation, 2026-10-23

Issue 31 is addressed through android/voice_main.c. Both enabled startup checks
set a persistent validation gate before the event loop. A failure identifies
the check, requests Activity finish, and blocks start, consumption and rendering
even if late lifecycle/input events arrive. Final cleanup clears the callback
and user pointer. The host harness includes the production entrypoint and real
FFT/framing/polynomial/raster code; it substitutes only the Android environment
and the deliberately failed numerical boundary. It checks both failures
independently, success after failure, initial permission denial and recovery,
pause/resume, focus loss/recovery, window recreation, and balanced close.
This is simulated Android behavior, not execution on Android.

Host ICK source fbe86e23d55cfec2000c08e61deea2a407fd7175 passed the existing
suite plus both entrypoint variants. Address/undefined behavior instrumentation
passed with local leak detection disabled because this container's process
visibility makes LeakSanitizer fail before test execution. The hosted lane
retains its original leak settings and independently uses the pinned
c61e448251744a2f40ad743ebef1a027bdcd2f9d compiler.

The GPU test executes the retained production shaders on Mesa llvmpipe GLES
3.2. Silence, constant, impulse, exact cosine/sine bins, mixture, deterministic
random, framed random, and one retained Nightingale window are covered. The
largest complex coefficient error was 7.27937032e-8; compared color channels
differed by at most one byte. Discontinuity/near-zero exclusions are printed
per fixture. The recorded window's coefficient error was 2.66070382e-10.
CPU reference arithmetic and binary32 GPU arithmetic remain distinct.

The reference comparison exposed different sample locations: the old GPU used
cell centers, while the CPU includes the rectangle endpoints. The shader now
uses fragment coordinates and the actual surface size to match those points.
It defines phase zero at the exact complex zero. No noise gate was added and
the explicit 24-term polynomial, DC removal and periodic Hann remain intact.
The unresolved Maresh/Diproton formula remains unresolved.

## Bytes and allocations

`make measured-ABI` builds stripped CPU/GPU candidates, separate debug files,
ELF dynamic sections, unresolved-symbol lists, APK digests and size tables.
`build/freestanding/ABI/polynomial.o` compiles the Horner component with
freestanding/no-builtin/no-stack-protector flags and rejects any undefined
symbol. The host x86-64 object has 157 text bytes, zero data/BSS, no undefined
symbols and only an internal unwind relocation. This qualifies a mathematical
leaf, not the renderer, application binary or running Android system.

The AAudio backend allocates one control object and a 32768-frame ring after
negotiation. Ring storage is 32768 × channels × sample-bytes: requested mono
float uses 131072 bytes; the accepted maximum of eight float channels uses
1048576 bytes. PCM callbacks do not allocate. The application retains 1024
float samples (4096 bytes); a read batch reserves 16384 raw bytes and 2048
mono bytes. CPU rendering also reserves 16384 coefficient bytes and 55296 RGB
bytes on the stack. The GPU requests 4096 sample bytes and two 8192-byte
spectrum buffers, plus opaque driver allocations. These are source-derived
payload bounds, excluding object, stack-frame and allocator overhead.

Actual captured/dropped/rendered counts, frame pacing, peak resident memory,
driver allocation and complete loaded-library closure require execution on
each device and remain NOT_RUN. NDK link stubs cannot establish the runtime
transitive dependency closure.

## Audio hardware boundary

The current backend requests shared AAudio input, mono/48 kHz/float, then
negotiates rate and signed16 fallback while reporting actual properties.
It does not request or open a vendor HAL or kernel microphone device.
[AOSP's AAudio description](https://source.android.com/docs/core/audio/aaudio)
distinguishes service-managed MMAP from the legacy AudioFlinger fallback,
explains the reverse data flow for capture, and describes why a raw /dev/snd
descriptor cannot simply be passed to an ordinary client. Requesting SHARED
does not reveal which route this particular handset negotiated. Determining
that route and its service/vendor/kernel dependencies needs a device receipt.
No policy, permission or system property is changed by this continuation.

The historical user observation that Fourier Voice moved graphics on MIRO A1
is preserved. It does not accept these rebuilt candidates, coefficients or
GPU execution. A1, C67 and TAB_P10 physical tests remain separate. No branch
is merged or release promoted by this work.

`acceptance/voice-collect.grease` prepares read-only collection through an
already-authorized host ADB channel. It requires an explicit device instance,
canonical absolute paths and a matching candidate digest; checks A1/C67 model
and ABI before writing a new host receipt directory; and collects current
package, tagged logs, audio-service and memory information. It neither installs
nor accepts an APK and never resets data or permissions. Actual installation
must preserve the established signer and package. The collector was parsed by
Grease ba869518c7d850de6c47d8c6234654575e264e6c, source gitlink
5651cf97a1b5042f24f14112a7ade9a1518eb0bc; its missing-argument refusal exited
2. Physical collection was NOT_RUN. Tablet ADB remains deferred.

For physical acceptance, retain an installed-artifact/signer match and a
separate human acoustic/visual receipt: deny then grant microphone permission;
quiet then audible input while reading signal/coefficient diagnostics;
background/foreground, focus and window recreation; clean close/restart;
capture/drop/render totals and pacing; vendor/renderer and numerical/orientation
comparison for the GPU candidate. Collection alone satisfies none of those
behavioral requirements. The historical offline Nightingale result supplies
repeatable coefficients and audio timing, not a live-source substitute.
