# GPU-resident live spectrum path

The retained MIRO A1 experiment from `gpu/fft-wegert-resident` is reconciled
with the owned-ICK producer branch. It removes the live CPU spectrum/render loop.

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

The measured candidates are `fourier-voice-gpu-ABI-v7.apk`, stripped
ARMv7, AArch64 and x86-64 no-DEX packages using the same signing key and package
name as the earlier Fourier Voice test APK, with version code 7. They share package
identity with the CPU release; switching variants still requires digest and
signer verification and does not authorize uninstalling an incompatible package.

## Outstanding integration gate

The retained GPU work remains subject to its original physical acceptance gate.
Follow `docs/pr-consolidation.md`, `docs/acceptance.md` and
`docs/android-rendering.md` for the inherited physical obligations. Microphone
permission/acoustic/lifecycle checks, static window visual/timing checks and
replacement installation remain unverified for these newly packaged bytes.
PowerVR shader compile/link, independent spectrum/colour/orientation comparison,
live motion and pacing must be accepted before this GPU PR merges.
MIRO A1 is primary; C67 and TAB_P10 require their own acceptance even when
they use the same AArch64 candidate.

Builds use the shared android-NDK packager. All application C, including the
GPU pipeline, goes through ICK; upstream glue, assembly and linking remain
NDK stages. ARMv7 retains A32/softfp. No Android driver result follows from a
native library link.

`make gpu-reference-test` executes the production compute/fragment shaders
on a host GLES driver with a diagnostic pbuffer. The independent direct DFT
checks all 1024 complex coefficients. Pixel tests compare the CPU construction
and Wegert colors at the same rectangle endpoints, reversing OpenGL's bottom
row convention explicitly. Tiny values and color discontinuities are counted
as exclusions, not silently included in a broad accuracy claim. Silence still
compares every pixel, including the defined zero phase. An optional exact
Nightingale signed16 fixture supplies a real recorded window.

Only this test defines FOURIER_GPU_DIAGNOSTICS, exposing coefficient readback
and a host pbuffer. Shipping import checks reject glMapBufferRange and
glReadPixels, as well as the CPU FFT. The shipping spectrum remains in its
final ping-pong SSBO. Mesa/llvmpipe execution is HOST_TESTED; it does not accept
the MIRO A1/C67 PowerVR or TAB_P10 Mali driver.
