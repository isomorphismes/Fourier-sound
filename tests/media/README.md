# Real audio fixtures

`make media-test` runs an opt-in corpus of real environmental and musical audio through the same plain-C boundaries used by Fourier-sound. The normal `make test` target remains deterministic and offline.

The source recordings are downloaded into `build/media`, checked against a published source digest where one is available, and converted with ffmpeg to eight seconds of mono 44.1 kHz signed 16-bit little-endian PCM. The binary media is not committed to the repository.

| Fixture | Source window | Reuse status | Why it is useful |
| --- | --- | --- | --- |
| `thunder-rain.s16` | 0-8 s | Public domain, ezwa / PDSounds | broadband rain plus impulsive thunder |
| `iceberg-contact.s16` | 0-8 s | NOAA PMEL U.S.-government recording | low-frequency iceberg harmonic tremor; NOAA describes it as contact with the seafloor or another iceberg |
| `dvorak-largo.s16` | 60-68 s | Public-domain Musopen recording | orchestral mixture with sustained and overlapping partials |
| `bartok-sonatina.s16` | 10-18 s | Composition public domain; recording CC BY-SA 3.0, La Pianista | solo piano transients, decay and harmonically dense chords |

Sources:

- Thunder and rain on a veranda: https://commons.wikimedia.org/wiki/File:Thunder_and_rain_on_a_v.ogg
- NOAA PMEL Cryogenic (Ice), `Iceberg Harmonic Tremor`: https://pmel.noaa.gov/acoustics/specs_cryogenic.html
- Dvořák, Symphony No. 9, II. Largo: https://commons.wikimedia.org/wiki/File:Antonin_Dvorak_-_symphony_no._9_in_e_minor_%27from_the_new_world%27%2C_op._95_-_ii._largo.ogg
- Bartók, Sonatina, performed by La Pianista: https://commons.wikimedia.org/wiki/File:Bartok_-_Sonatina.ogg

The Chris Watson / Touch Antarctic recordings are good listening references but are not included as automated fixtures because the recordings are commercially released rather than a clearly reusable public-domain corpus. The NOAA iceberg-contact recording fills the same test role without copying that material.

## What the test checks

For every fixture, `tests/media_fixture_test.c`:

1. reads canonical signed-16 PCM and sends every complete block through `fourier_pcm_mono`;
2. finds a non-silent 512-frame block;
3. runs a deliberately small test-local direct DFT over that block and requires finite, nontrivial spectral energy in more than one bin;
4. sends the same mono block through `speaker_input` as stereo signed-16 output;
5. converts that rendered speaker PCM back through `fourier_pcm_mono` and checks the quantized round trip.

The direct DFT is only a test oracle. It does not establish a production Fourier/FFT API or window policy.

Run:

```text
make media-test
```

Requirements for this optional target are `curl`, `ffmpeg`, and `sha1sum`. Downloaded and converted files remain under `build/media` and are already excluded by the repository's build ignore rule.
