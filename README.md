# Fourier-sound

Voice- and sensor-driven visual mathematics on phones.

The working target is **phyphox-like acquisition with Wegert-style complex-function output**: expose phone hardware through small reusable interfaces, analyze the resulting streams with replaceable mathematical decompositions, map the result into complex objects, and render those objects with phase/domain coloring.

## First reference experiment

Owen Maresh / diproton: [q-series visualization](https://www.youtube.com/watch?v=NcqSDIGU02I)

Maresh drives a complex q-series visualization from his voice through an FFT. See [docs/diproton-q-series.md](docs/diproton-q-series.md).

## Do not identify Fourier analysis with FFT

The FFT is one efficient algorithm for a finite Fourier transform. It should not define the mathematics or the software boundary.

Terence Tao's short article [Fourier Transform](https://www.math.ucla.edu/~tao/preprints/fourier.pdf), written for *The Princeton Companion to Mathematics*, begins with even/odd decomposition and then generalizes the same idea to rotational harmonics for the cyclic group of roots of unity. That is the right conceptual starting point for this project: a transform decomposes an object according to symmetry; FFT is one implementation for one particular decomposition.

See [docs/harmonic-decomposition.md](docs/harmonic-decomposition.md).

## Pipeline

```text
phone hardware
    ↓
acquisition source / output sink
    ↓
framing and normalization
    ↓
decomposition / transform
    ↓
mathematical construction
    ↓
complex field / geometry
    ↓
Wegert-style renderer
```

Each boundary must remain replaceable.

Examples:

- input: microphone, accelerometer, later other phone sensors;
- output: speaker, later other actuators;
- decomposition: direct DFT, FFT, even/odd decomposition, cyclic-character decompositions, wavelets or other experiments;
- mathematical construction: Maresh-style lacunary q-series, dynamical systems such as Fatou/Julia constructions, or other maps;
- rendering: phase/domain coloring following Elias Wegert.

## Hardware depth

Hardware support should allow several depths side by side:

1. generic Android native interface;
2. lower-level Linux/PCM or kernel-facing interface;
3. SoC/device-specific paths.

The application should depend on the small generic acquisition/sink contract, not directly on Android APIs. Deeper implementations can replace the backend without changing the mathematics. The reusable Android/native boundary belongs alongside the work in [isomorphisms/android-NDK](https://github.com/isomorphisms/android-NDK).

See [docs/architecture.md](docs/architecture.md).

## First native audio implementation

The audio branch adds a plain C microphone source, an AAudio backend, a bounded
PCM buffer and a native acceptance APK. Captured PCM also passes through
`fourier/pcm_block`, an Android-free conversion boundary for mathematical code.
It introduces no decomposition algorithm or visualization.

Run `make test` for deterministic host checks. Run `make media-test` for the opt-in real-audio corpus (thunder, iceberg contact, Dvořák and Bartók); see [real audio fixtures](tests/media/README.md). See [audio implementation](docs/audio-input.md) for the accelerometer comparison and [acceptance](docs/acceptance.md) for APK builds and the MIRO A1 procedure.

## References

- [phyphox audio spectrum](https://phyphox.org/experiment/audio-spectrum/) — microphone blocks analyzed with a Fourier transform.
- [phyphox hardware input format](https://phyphox.org/docs/file-format/input/) — sensors and microphone represented as input sources.
- [phyphox supported sensors](https://phyphox.org/sensors/) — microphone and sensors as inputs; speaker as an output.
- [phyphox Android source](https://github.com/phyphox/phyphox-android).
- Terence Tao, [Fourier Transform](https://www.math.ucla.edu/~tao/preprints/fourier.pdf).
- Elias Wegert, [Phase Plots of Complex Functions: a Journey in Illustration](https://arxiv.org/abs/1007.2295).
