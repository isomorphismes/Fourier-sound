# Architecture

Fourier-sound should separate four independent questions:

1. How do bytes/samples enter or leave the phone?
2. How is a sample block decomposed?
3. How do decomposition results parameterize a mathematical object?
4. How is that object visualized?

## 1. Hardware boundary

Keep acquisition and output minimal enough to expose through a C ABI, then wrap that ABI from every language/compiler under test.

Conceptual input contract:

```text
source open
source start
source stop
source close

sample block:
    samples
    sample count
    channel count
    sample rate
    monotonic timestamp
```

Conceptual output contract:

```text
sink open
sink start
sink write block
sink stop
sink close
```

Backends may coexist:

```text
generic Android native
        ↓ optional replacement
Linux / PCM
        ↓ optional replacement
SoC- or device-specific
```

Do not force every device or capability to use the same depth. A generic microphone backend can coexist with a device-specific accelerometer backend. The mathematical layers should not care.

The generic Android implementation should coordinate with [isomorphisms/android-NDK](https://github.com/isomorphisms/android-NDK), so the native hardware work can be reused by other repositories and languages instead of copied into Fourier-sound.

## 2. Framing

Acquisition produces samples. Framing decides what constitutes one mathematical observation:

- block length;
- overlap;
- windowing;
- channel selection;
- gain/normalization;
- sample-rate conversion if needed.

Keep raw samples available. Maresh explicitly notes that the voice-to-structure response needs tuning; normalization belongs here rather than inside q-series mathematics.

## 3. Decomposition

Expose a common transform/decomposition interface.

Initial implementations:

- direct DFT reference;
- FFT backend;
- reflection / even-odd projector;
- finite cyclic (C_n) character decomposition.

Later candidates can include wavelets, autocorrelation, spectral estimators, learned transforms, or geometry-specific decompositions.

## 4. Mathematical construction

Transform output should feed a separate construction.

First target: reproduce the diproton q-series experiment closely enough to compare behavior.

Other targets should remain possible. A Fatou/Julia construction is not the same kind of object as a q-series, so do not hide both behind a misleading “series” interface. Give this layer a broader field/geometry contract.

## 5. Rendering

Use a renderer that accepts sampled complex values over a domain and maps them to pixels.

Elias Wegert's phase plots provide the main visual reference: encode argument/phase by a circular color map, optionally adding modulus information. See [Phase Plots of Complex Functions](https://arxiv.org/abs/1007.2295).

The renderer must not know whether its field came from voice, accelerometer data, a q-series, or another dynamical construction.

## phyphox as the systems reference

phyphox supplies a useful comparison because it treats phone hardware as experiment inputs and analysis as a separate layer. Its official Audio Spectrum experiment records microphone audio in blocks and computes a discrete Fourier transform; its hardware documentation also exposes the speaker as output.

References:

- [Input modules](https://phyphox.org/docs/file-format/input/)
- [Audio Spectrum](https://phyphox.org/experiment/audio-spectrum/)
- [Supported sensors and outputs](https://phyphox.org/sensors/)
- [Android source](https://github.com/phyphox/phyphox-android)

Fourier-sound should borrow this separation without copying phyphox's experiment language or forcing its analysis choices.
