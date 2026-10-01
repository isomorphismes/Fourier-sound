# Real audio fixtures

`make media-test` runs an opt-in corpus of real environmental and musical audio through the same plain-C boundaries used by Fourier-sound. The normal `make test` target remains deterministic and offline.

The source recordings are downloaded into `build/media`, checked against a published source digest where one is available, and converted with ffmpeg to eight seconds of mono 44.1 kHz signed 16-bit little-endian PCM. The binary media is not committed to the repository.

| Fixture | Source window | Reuse status | Why it is useful |
| --- | --- | --- | --- |
| `thunder-rain.s16` | 0-8 s | Public domain, ezwa / PDSounds | broadband rain plus impulsive thunder |
| `iceberg-contact.s16` | 0-8 s | NOAA PMEL U.S.-government recording | low-frequency iceberg harmonic tremor; NOAA describes it as contact with the seafloor or another iceberg |
| `dvorak-largo.s16` | 60-68 s | Public-domain Musopen recording | orchestral mixture with sustained and overlapping partials |
| `bartok-sonatina.s16` | 10-18 s | Composition public domain; recording CC BY-SA 3.0, La Pianista | solo piano transients, decay and harmonically dense chords |
| `russolo-corale.s16` | 0-8 s | Public domain, Luigi/Antonio Russolo historical recording | early intonarumori: orchestral tone plus machine-like noise |
| `russolo-serenata.s16` | 0-8 s | Public domain, Luigi/Antonio Russolo historical recording | strings/woodwinds interrupted by intonarumori noise |

Sources:

- Thunder and rain on a veranda: https://commons.wikimedia.org/wiki/File:Thunder_and_rain_on_a_v.ogg
- NOAA PMEL Cryogenic (Ice), `Iceberg Harmonic Tremor`: https://pmel.noaa.gov/acoustics/specs_cryogenic.html
- Dvořák, Symphony No. 9, II. Largo: https://commons.wikimedia.org/wiki/File:Antonin_Dvorak_-_symphony_no._9_in_e_minor_%27from_the_new_world%27%2C_op._95_-_ii._largo.ogg
- Bartók, Sonatina, performed by La Pianista: https://commons.wikimedia.org/wiki/File:Bartok_-_Sonatina.ogg
- Luigi and Antonio Russolo, `Corale` and `Serenata`, 1921: https://archive.org/details/russolo-luigi-corale-serenata-1921
- PennSound discovery page for the Russolo recordings: https://writing.upenn.edu/pennsound/linking-page/Marinetti.php

PennSound is useful as a discovery archive, but it is not generally a public-domain corpus. Its site-wide notices normally limit recordings to noncommercial and educational use and retain rights in the author or estate. Historical items therefore need individual rights checking. The Russolo recordings above are included because Public Domain Review independently identifies `Corale` and `Serenata` as public-domain audio and points to the Internet Archive copies.

Chris Watson's Antarctic material on Touch is also a reference rather than an automated fixture. Touch Radio 49, `A Journey South` (2010), contains Watson's Antarctic hydrophone recordings, including the characteristic creaking and groaning ice: https://touchradio.org.uk/touch_radio_49_chris_watson.html . TouchRadio makes the MP3 free to listen/download but states that Touch content is published by Touch Music or is public domain and separately marks the site all rights reserved. That is not enough to establish this Watson recording itself as public domain. Keep it as a listening/comparison target unless Touch/Watson grants reuse permission. The NOAA iceberg-contact recording fills the automated ice-test role without copying it.

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

Requirements for this optional target are `curl`, `ffmpeg`, and `sha1sum`. Downloaded and converted files remain under `build/media` and are already excluded by the repository's build ignore rule.
