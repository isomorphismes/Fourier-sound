# Real audio fixtures

`make media-test` runs real environmental and musical audio through the same plain-C boundaries used by Fourier-sound. The normal `make test` target remains deterministic and offline.

The recording corpus is owned by `dilapidated-shed/sounds`. Fourier-sound pins one exact `sounds` revision in `SOUNDS_REV`, checks out that revision under `build/sounds`, and asks that repository to materialize its redistributable originals. Fourier-sound does not maintain its own copy of recording URLs, rights notes, checksums, or source-audio catalogue.

The Fourier test then derives eight-second mono 44.1 kHz signed-16 PCM files under `build/media`. Those PCM files are build artifacts, not a second corpus.

| Fourier fixture | sounds corpus id | Source window | Why it is useful |
| --- | --- | --- | --- |
| `thunder-rain.s16` | `thunder-rain-veranda` | 0-8 s | broadband rain plus impulsive thunder |
| `iceberg-contact.s16` | `noaa-iceberg-harmonic-tremor` | 0-8 s | low-frequency iceberg harmonic tremor |
| `dvorak-largo.s16` | `dvorak-new-world-largo` | 60-68 s | orchestral mixture with sustained and overlapping partials |
| `bartok-sonatina.s16` | `bartok-sonatina` | 10-18 s | solo piano transients, decay and harmonically dense chords |
| `russolo-corale.s16` | `russolo-corale` | 0-8 s | orchestral tone plus machine-like intonarumori noise |
| `russolo-serenata.s16` | `russolo-serenata` | 0-8 s | strings/woodwinds interrupted by intonarumori noise |

For provenance, redistribution status, credits, checksums, and reference-only recordings such as Chris Watson's Antarctic hydrophone material, use the pinned `sounds` repository and its `manifest.tsv`, `sources/`, and `reference-only/` directories.

## What the test checks

For every fixture, `tests/media_fixture_test.c`:

1. reads canonical signed-16 PCM and sends every complete block through `fourier_pcm_mono`;
2. finds a non-silent 4096-frame block;
3. runs a deliberately small test-local direct DFT over that block and requires finite, nontrivial spectral energy in more than one bin;
4. sends the same mono block through `speaker_input` as stereo signed-16 output;
5. converts that rendered speaker PCM back through `fourier_pcm_mono` and checks the quantized round trip.

The direct DFT is only a test oracle. It does not establish a production Fourier/FFT API or window policy.

Run:

```text
make media-test
```

The optional target requires `git`, `curl`, `ffmpeg`, `awk`, and `sha1sum`. Everything downloaded or derived remains under `build/`.
