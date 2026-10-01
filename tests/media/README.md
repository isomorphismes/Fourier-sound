# Real audio rendering fixtures

`make media-test` runs an opt-in corpus of real environmental and musical
audio through the production plain-C Fourier-sound boundaries. The normal
`make test` target remains deterministic, offline, and fast.

Sources are downloaded under `build/media` and converted with ffmpeg to eight
seconds of mono 44.1 kHz signed-16 little-endian PCM. Binary media and generated
images are not committed.

| Fixture | Source window | Reuse status | Useful structure |
| --- | --- | --- | --- |
| `thunder-rain.s16` | 0-8 s | public-domain PDSounds recording via Wikimedia Commons | broadband rain and impulsive thunder |
| `iceberg-contact.s16` | 0-8 s | NOAA PMEL U.S.-government recording | low-frequency harmonic tremor |
| `dvorak-largo.s16` | 60-68 s | public-domain Musopen recording | sustained orchestral partials |
| `bartok-sonatina.s16` | 10-18 s | composition public domain; La Pianista recording CC BY-SA 3.0 | piano transients and chords |
| `russolo-corale.s16` | 0-8 s | public-domain historical recording | orchestral tone plus intonarumori noise |
| `russolo-serenata.s16` | 0-8 s | public-domain historical recording | instrumental and intonarumori contrast |

## Pipeline exercised

For every fixture the test now performs

```text
downloaded recording
  -> canonical mono s16 PCM
  -> fourier_pcm_mono
  -> explicit 4096-sample framing
  -> choose loudest block by RMS
  -> mean removal
  -> Hann window
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

## Sources and rights notes

- Thunder and rain: https://commons.wikimedia.org/wiki/File:Thunder_and_rain_on_a_v.ogg
- NOAA iceberg harmonic tremor: https://pmel.noaa.gov/acoustics/specs_cryogenic.html
- Dvořák Largo: https://commons.wikimedia.org/wiki/File:Antonin_Dvorak_-_symphony_no._9_in_e_minor_%27from_the_new_world%27%2C_op._95_-_ii._largo.ogg
- Bartók Sonatina: https://commons.wikimedia.org/wiki/File:Bartok_-_Sonatina.ogg
- Russolo recordings: https://archive.org/details/russolo-luigi-corale-serenata-1921
- PennSound discovery page: https://writing.upenn.edu/pennsound/linking-page/Marinetti.php

PennSound is useful for discovery but is not treated as a blanket
public-domain corpus. Chris Watson's Antarctic Touch recordings likewise remain
listening/reference material rather than automated fixtures because free
download is not the same thing as public-domain reuse permission.

Run:

```text
make media-test
```

Requirements for the optional target are `curl`, `ffmpeg`, and `sha1sum`.
