# Functorial C / ICK qualification — 2026-10-06

**FUNCTORIAL + ICK BLOCKED:** current source-built ICK fails on the Bionic
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

## 2026-10-08 Icky Horner source pass

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
