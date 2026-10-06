# Retained ARMv7 QEMU screening results

Source: `da4d436d24f006e6c90c6de5b2957b43687ca1aa`.
Executed [run](https://github.com/isomorphismes/Fourier-sound/actions/runs/36956930667),
[artifact](https://github.com/isomorphismes/Fourier-sound/actions/runs/36956930667/artifacts/11206327313).
Downloaded ZIP SHA-256: `d0837f5bc965060f836c19f1b1d1f9227f071d69cf4169e26f6693ebd3359355`.

Compiler: Ubuntu arm-linux-gnueabi GCC 13.3.0-6ubuntu2~24.04.
QEMU: 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), TCG user mode,
`cortex-a7` and `cortex-a15`. Target: static Linux ELF32 ARM,
ARMv7 Thumb-2, NEON/VFPv4, softfp, IEEE half storage. This is **not** an
Android/Bionic binary or an ICK-produced application. ICK precision definitions
come from `2176498723bf01792236cef3bebf349822cae496`.

Build flags: `-std=c11 -O3 -static -fno-tree-vectorize -march=armv7-a
-mthumb -mfpu=neon-vfpv4 -mfloat-abi=softfp -mfp16-format=ieee`.
Raw timing/numeric rows, ELF attributes, compiler identity and summary are
retained here. The historical executable/disassembly remain in the linked
artifact; the reusable benchmark source and summary program remain in `bench/`.

The screening question has been answered: binary32 and explicit NEON warrant
physical measurement; FP16/fp8 storage did not buy throughput in these runs.
At 1024 samples, NEON FFT speedup over the binary64 baseline was 1.286–1.290,
with maximum coefficient error about 7.00×10⁻⁷. At 24 terms, polynomial NEON
speedup was 1.134–1.209. Polynomial `error` is relative error of the aggregate
checksum, **not** maximum per-pixel or visual error. The function historically
named `max_poly_error` does not compute a maximum. Unsigned E5M3 measures
decode only; it cannot represent the signed complex components used here.

QEMU wall time is not cycle-accurate MIRO performance. No production precision
or SIMD policy changes follow from these rows. Physical optimization remains
an acceptance obligation, rather than a reason to keep this completed screening
PR open indefinitely.
