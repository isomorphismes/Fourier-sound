# Fourier implementation consolidation — 2026-10-06

The source integration line is `audio/native-input`, targeting `main`.
The remaining GPU implementation is `gpu/fft-wegert-resident`.
Source integration does not accept current physical-device behavior.

## Actual starting topology

Main `70ddfa1da60edcaa64fa9aaa9ef5bca69f33d595` contained design/movie instructions only. The following
full branch tips were inspected, rather than treating PR numbers as ancestry:

| PR | Branch tip | Unique obligations/material |
| --- | --- | --- |
| [6, native microphone](https://github.com/isomorphismes/Fourier-sound/pull/6) | `8d57183fe247066a4be6fd45d37b27acf3e6da34` | AAudio, ring, PCM, microphone acceptance; includes speaker and phase-reference merge commits and sparse code |
| [13, FFT](https://github.com/isomorphismes/Fourier-sound/pull/13) | `323b3e8328fa3a95aa02b05f98a65b1e1d32cda7` | complete DFT-oracle FFT tests, stronger framing tests, real media harness and transform/framing notes |
| [19, construction comparison](https://github.com/isomorphismes/Fourier-sound/pull/19) | `82ea9d8d95a17d727f96bb19508cf2e690093ff2` | sparse series, controlled comparison, sounds revision, measured result table |
| [22, native window](https://github.com/isomorphismes/Fourier-sound/pull/22) | `21e0617482c4651349004c5f0a111a39f41269f1` | RGB/window tests, acceptance portrait; includes merged live CPU voice/ICK-leaf diagnostic line |
| [28, GPU spectrum](https://github.com/isomorphismes/Fourier-sound/pull/28) | `e1eb2344ed357eaf6e9f77b1423e0677c3e43834` | compute FFT, SSBO ownership, fragment renderer; separate physical shader/visual gate |
| [29, numeric screening](https://github.com/isomorphismes/Fourier-sound/pull/29) | `da4d436d24f006e6c90c6de5b2957b43687ca1aa` | benchmark source/workflow, smooth CPU changes, raw QEMU results retained under bench/results |
| [30, plotting composition](https://github.com/isomorphismes/Fourier-sound/pull/30) | `04353383b6383cc70885a0eb63ca473f45edbccb` | shared complex mapping/raster, A32 qualification and explicit full-ICK gap |

PR 6 contained the original sparse implementation through its phase-reference
merge, but not PR 19's comparison. PR 22 contained diagnostic copies of FFT and
framing, but not all PR 13's regression tests/history. PR 28 forked the ICK voice
line; PR 29 forked the smooth-render line. The newer plotting composition forked
the ICK voice line too. These were intersecting branches, not a six-link chain.

All CPU/reference branches above are preserved as merge parents of the source
integration. Conflicts retain the independent DFT oracle, the fuller framing
tests, sparse construction, current plotting composition and smooth cadence.
The integration calls the shared android-NDK packager, preserves the signer,
and records the exact ICK application-compiler gap.

## Acceptance ownership after consolidation

The one live GPU PR carries the unresolved physical integration obligations:
microphone allow/deny and acoustic response, overflow/clean shutdown and
pause/resume, replacement install, static portrait and timing, PowerVR GLSL
compile/link, GPU numeric/colour/orientation comparison against the CPU reference,
and live audio-driven motion/pacing. TAB_P10 requires its own receipt.

Do not call host doubles microphone execution, linkage GPU execution, or QEMU
wall time physical performance. CPU spectra copied to a GPU are distinct from
GPU-computed spectra retained in SSBOs. PR 28 implements the latter in source;
only an actual driver run can accept it. Any test-only spectrum readback must
remain outside the shipping no-readback path.

Current ICK cannot compile the whole Bionic application surface. This remains
an explicit qualification gap, not a full-Icky completion claim. Production
numeric policy remains binary64 CPU reference until separately accepted physical
measurements justify changing it.
