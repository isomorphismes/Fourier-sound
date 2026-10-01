# Real audio rendering fixtures

`make media-test` runs an opt-in corpus of real environmental and musical
audio through the production plain-C Fourier-sound boundaries. The normal
`make test` target remains deterministic, offline, and fast.

The recording corpus is owned by `dilapidated-shed/sounds`. Fourier-sound
pins one exact corpus revision in `SOUNDS_REV`, checks it out under
`build/sounds`, and resolves recordings by stable ID through
`manifest.tsv`. Source recordings, provenance, rights notes, checksums, and
reference-only material are maintained only in `sounds`.

Fourier-sound derives eight-second mono 44.1 kHz signed-16 PCM files under
`build/media`. Those PCM files and generated images are build artifacts, not
a second sound corpus.

| Fourier fixture | sounds corpus id | Source window | Useful structure |
| --- | --- | --- | --- |
| `thunder-rain.s16` | `thunder-rain-veranda` | 0-8 s | broadband rain and impulsive thunder |
| `iceberg-contact.s16` | `noaa-iceberg-harmonic-tremor` | 0-8 s | low-frequency harmonic tremor |
| `dvorak-largo.s16` | `dvorak-new-world-largo` | 60-68 s | sustained orchestral partials |
| `bartok-sonatina.s16` | `bartok-sonatina` | 10-18 s | piano transients and chords |
| `russolo-corale.s16` | `russolo-corale` | 0-8 s | orchestral tone plus intonarumori noise |
| `russolo-serenata.s16` | `russolo-serenata` | 0-8 s | instrumental and intonarumori contrast |

## Pipeline exercised

For every fixture the test now performs

```text
sounds corpus recording
  -> canonical mono s16 PCM
  -> fourier_pcm_mono
  -> explicit 4096-sample framing
  -> choose loudest block by RMS
  -> mean removal
  -> periodic Hann window
  -> production radix-2 FFT
  -> full complex coefficients
  -> current smoke-test polynomial construction
  -> sampled complex field
  -> canonical Wegert color
  -> 128x128 PPM
```

The unwindowed loudest block also goes through `speaker_input` and back through
`fourier_pcm_mono` to retain the existing speaker-boundary round-trip check.

The test requires nontrivial spectral occupancy and nonconstant rendered color,
and writes one image beside each PCM fixture, for example
`build/media/thunder-rain.s16.ppm`.

The selected construction is deliberately still the simple polynomial
`sum c[k] z^k`, using the first 64 coefficients on a viewport contained
inside the unit disk. These images prove that real sound reaches complex-field
pixels; they are not presented as Owen Maresh's q-series.

For recording provenance, redistribution status, credits, checksums, and
reference-only recordings such as Chris Watson's Antarctic hydrophone
material, use the pinned `sounds` revision rather than duplicating that
information here.

Run:

```text
make media-test
```

Requirements for the optional target are `git`, `ffmpeg`, and `awk`.

## Construction comparison experiment

The `experiments/audio-construction-compare` branch holds sound analysis and
Wegert coloring fixed while comparing the same first twelve Fourier coefficients under dense exponents
`0..11` versus one explicit dyadic sparse exponent schedule. It writes `.dense.ppm` and `.dyadic.ppm`
for each recording. The dyadic schedule is a candidate experiment, not a Maresh
reconstruction.
