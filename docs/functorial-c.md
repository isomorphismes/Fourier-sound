# Functorial C and division-glyph qualification

## 2026-10-09: owned division and complete C producers

The maintained C/header scan covers all 60 checked-in source files. No binary
ASCII division or compound division assignment remains. The remaining directive
slashes are include-header paths. This continuation changes 71 binary divisions
in 15 C files and rewrites one simple local `mean /= count` assignment as
`mean = mean ÷ count`; the earlier native-test migration is retained.
Paths, comments, literal text, and frozen references keep their original meaning.

All four application source groups and the reference-render library now compile
from their actual source through ICK
`c61e448251744a2f40ad743ebef1a027bdcd2f9d`. ICK produces assembly; NDK r27c
assembles it, compiles the unchanged upstream `native_app_glue`, and links Bionic
and the Android platform libraries. There is no source transliteration step.
The shared producer is pinned at
`903b2cb27ea572c9c6cb2ffa9f39e0fbf06ec9f8`.

Local full-library qualification passes for ARMv7 A32/softfp, AArch64 with x18
reserved, and x86-64 baseline. Every ABI builds microphone, speaker, render-window,
voice, and reference-render libraries. Both specialized ARM voice variants also
link. The source stages retain API 26, `_FORTIFY_SOURCE=2`, stack protection,
optimization, and all existing warning/error flags. ICK's own resource headers
precede the NDK headers under `-nostdinc`, so C atomics use the correct compiler
intrinsics. Debug information is retained as DWARF 4, with variable-location
views disabled because the NDK assembler does not accept GCC's newer directives.

The bounded shared Fortify adapter uses the real Bionic checking entrypoints.
Unsupported fortified APIs fail compilation. Its dynamic-overflow and argument
evaluation controls are qualified by the shared producer; this consumer does not
disable Fortify to compile. A checked immutable frame count now preserves the
audio input bound across out-of-line calls; an oversized result is rejected.

All 13 existing native tests pass with actual ICK, and the rendered reference
frame remains byte-identical. The ARMv7 numeric matrix also compiles with ICK and
runs under Cortex-A7 and Cortex-A15 QEMU. That lane explicitly links Ubuntu's GNU
glibc, libm, libgcc and unwind runtime, preserving Thumb-2, NEON-vfpv4, softfp,
static linkage, optimization and the existing precision matrix. Its timings are
emulator throughput, with the original physical-phone interpretation boundary.
The existing Python summary producer is retained as named migration debt.

The native-audio, specialized-leaf, stripped-release, and QEMU workflows all
check the exact PR source head and select the pinned ICK producer. The Android
workflow retains all three ABIs and the established APK identities, signer,
version codes, receipts, stripping checks, and package verification. Historical
`release/ndk` and `release/ick` paths distinguish baseline and specialized-leaf
artifacts; both now use ICK for owned C. Runtime provenance names the application
compiler and platform link separately.

The specialized leaf keeps its GNU ARM attribute oracle. The pinned compiler
action installs `arm-linux-gnueabi-readelf`; its tag spelling matches the existing
ARMv7/A32/alignment/softfp predicates. NDK tools still inspect ELF headers and
code-mapping symbols. LLVM's structured attribute display does not match those
GNU text predicates, so the attribute reader is selected explicitly.

Current-head hosted APK, sanitizer, media-corpus, and QEMU results remain workflow
evidence. Local compile/link results do not assert microphone/speaker runtime,
installation, physical-device acceptance, or release promotion.

## Historical qualification: 2026-10-06

The following records the earlier compiler and producer state. Its pending claims
are superseded by the dated continuation above, not retroactively reclassified.

**Historical ICK blocker:** the then-current source-built ICK failed on the Bionic
nullability and Android availability declarations required by application C.

The CPU voice path now reads as a composition:
`frame_voice_spectrum` → `log_spectrum` → `render_spectral_field` → native window
presentation. Framing retains mean removal, periodic Hann, normalized FFT, DC
removal, sample count, term count, viewport, and canonical Wegert colour.

`fourier_polynomial` represents borrowed coefficients, coefficient count and
term count. `complex_mapping` is a const mathematical state plus its evaluation
operation. `complex_plot_raster` composes coordinate projection, mapping
evaluation and `wegert_color_complex`; it owns no audio or Android window state.
The acceptance portrait and live voice share that implementation. The scalar
ICK-leaf boundary is isolated in its evaluator, outside plotting mathematics.

Executed: all twelve host executables, including independent expansion of
`z^3-1`, projection/colour checks, readonly coefficients, invalid domains, and
the existing audio/FFT/window backends. Both the ordinary NDK voice library and
the mixed ICK-leaf/NDK application library linked for API 26, A32/softfp ELF32 ARM
with NDK r27c. The ICK leaf was actually produced by current ICK
`79eccb8ff232e05bdbb9e345fc224f251636b43f`, not host GCC or NDK Clang.

The maintained Makefile and leaf workflow now request A32 and check code mapping
symbols; a Thumb capability attribute alone is not an instruction-state test.
Mixed targets/artifacts explicitly say `ick-leaf`, and runtime provenance says
`application_c=NDK-clang polynomial_leaf=ICK full_icky=0`. Package identity,
signer, API floor and version codes remain unchanged. Historical version names
are retained as package metadata, not upgraded to full Icky claims.

The shared ICK application compiler step stops on failure. The explicit NDK
build is a separately selected diagnostic lane. No automatic fallback, new
APK, microphone/speaker runtime, replacement install or physical acceptance is
claimed. GPU and numeric-screening branches were not replaced by this repair.

## Historical qualification: 2026-10-08 Icky Horner source pass

The production header-free polynomial leaf now composes
`coefficient_at → cartesian_product → with_added_coefficient →
horner_polynomial_value`. Its scalar/array Cartesian ABI is preserved.
Owned assignments use literal `←`; mathematical multiplication uses `×`;
pointer spelling remains `*`.

The native suite is compiled and linked by ICK pinned at
`c5d28dde9cc333a562b907785d0370b725146cdf`, through the declared native scalar producer.
All 13 executed host tests pass, including the independent DFT/FFT comparison,
Parseval/conjugacy, simulated audio/window boundaries and Wegert rendering.
The new Horner test checks an independently expanded polynomial and 882
comparisons against the frozen previous leaf: count clamping, empty inputs,
signed zeros, nonfinite inputs and output/input aliasing are included.

ICK-instrumented AddressSanitizer and UndefinedBehaviorSanitizer tests pass
locally with leak detection disabled only for that local run: this container
cannot inspect the process tasks needed by LeakSanitizer. The required CI
sanitizer command retains its default leak detection. The producer declares
and hashes its prebuilt Ubuntu glibc/GCC 13 startup and sanitizer dependencies;
no host compiler compiles consumer source. Automatic libatomic linkage and the
full native ICK runtime remain outside that native scalar profile.

The ARMv7 leaf producer pins the same compiler and is triggered by leaf/producer
PR changes. Its existing A32/softfp object checks and NDK application/link
boundary remain explicit. New target receipts are pending CI; native results
do not imply Android or physical-device execution.

This pass refactors the owned Horner leaf and its maintained test. Other
application C, headers, generators and experimental producers remain pending
source/profile review; `full_icky=0` remains the application claim.
