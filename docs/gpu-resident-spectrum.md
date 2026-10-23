# GPU-resident live spectrum path

The MIRO A1 experiment on `gpu/fft-wegert-resident` removes the live
CPU spectrum/render loop.

## Data path

```text
AAudio microphone
    ↓
mono float block (CPU)
    ↓
remove mean + periodic Hann window (CPU)
    ↓
one 1024-float upload
    ↓
bit-reversal compute pass (SSBO)
    ↓
10 radix-2 compute passes (ping-pong SSBOs)
    ↓
final spectrum SSBO
    ↓
24-term complex polynomial in the fragment shader
    ↓
Wegert phase/log-modulus colouring
    ↓
EGL window surface
```

The final spectrum is never mapped, copied, or read back by the CPU.  The
fragment shader reads the same GPU buffer produced by the final FFT stage.

The CPU reference FFT and CPU Wegert renderer remain in the repository as
tests/reference implementations, but the GPU APK does not link the CPU FFT
into its live native library.

## First-slice choices

- 1024 samples, matching the current voice experiment.
- 24 complex coefficients feed the displayed polynomial.
- The DC coefficient is ignored by the fragment shader, matching the live CPU
  path that zeroed coefficient 0 before rendering.
- The requested EGL buffer remains 96×192 for the first device test, matching
  the old CPU renderer's logical resolution.  Android scales that surface to
  the display.  This isolates the architectural change before increasing the
  fragment workload.
- OpenGL ES 3.1 is required because the implementation uses compute shaders and
  shader-storage buffers.  MIRO A1 reports OpenGL ES 3.2 in the existing device
  inventory.

## Synchronization invariant

Every compute dispatch is followed by
`glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT)`.  The fragment pass begins
only after the final FFT buffer has been made visible to later shader stages.
No CPU synchronization or coefficient transfer sits between FFT and rendering.

## Device receipt

A successful run should contain logs similar to:

```text
GPU_READY ... gl=3.2 ... surface=96x192 ...
GPU_PIPELINE sample_count=1024 fft_stages=10 terms=24 spectrum_readback=none
VOICE_GPU_PATH fft=compute-shader spectrum=SSBO renderer=fragment-shader spectrum_readback=none
VOICE_FRAME ... fft=gpu-radix2 ... spectrum_readback=none
```

The CI artifact is `fourier-voice-gpu-miro-release.apk`, a stripped
`armeabi-v7a`, no-DEX package using the same signing key and package name as
the earlier Fourier Voice test APK, with version code 5. It shares package
identity with the CPU release; switching variants still requires digest and
signer verification and does not authorize uninstalling an incompatible package.

## Outstanding integration gate

This is the one remaining implementation PR after CPU/reference consolidation.
Follow `docs/pr-consolidation.md`, `docs/acceptance.md` and
`docs/android-rendering.md` for the inherited physical obligations. Microphone
permission/acoustic/lifecycle checks, static window visual/timing checks and
replacement installation remain unverified for these newly packaged bytes.
PowerVR shader compile/link, independent spectrum/colour/orientation comparison,
live motion and pacing must be accepted before this GPU PR merges.
TAB_P10 requires separate ABI and Mali acceptance; this APK targets MIRO A1 only.

Builds use the shared android-NDK packager and request A32/softfp application C.
The exact current ICK/Bionic compilation gap is retained in
`docs/functorial-c.md`; no full-Icky or GPU-driver result follows from a native
library link. The CPU reference remains available through its separate activity
and host tests, while the GPU activity uploads PCM rather than CPU-produced
spectrum. Host tests do not execute GLSL.
