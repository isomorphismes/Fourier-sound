# Native speaker output

This branch adds the first Fourier-sound output sink. It is deliberately small:
plain interleaved PCM enters `audio_output_write`, and the Android backend writes
that PCM through AAudio.

On the MIRO A1, the route under investigation is:

```text
Fourier-sound
    |
audio_output
    |
AAudio
    |
AudioFlinger / Android audio policy
    |
android.hardware.audio 7.1 service
    |
libsprdaudiohalv7.so
    |
audio.primary.normal.so
    |
libtinyalsa.so
    |
/dev/snd
    |
speaker
```

The lower half is device evidence, not part of the portable API. In particular,
Fourier code must not include Spreadtrum mixer names, ALSA card/device numbers,
HIDL types, or private Android HAL headers.

## Why this is the first HAL hook

AAudio is the supported native application boundary. It lets us exercise the real
vendor HAL and speaker route without requiring Fourier-sound itself to become an
Android platform component. That is the shortest route to an audible acceptance
test and is compatible with an ordinary NDK build.

A separate direct-HAL backend can still be useful on the rooted MIRO A1. That
backend will need platform/vendor headers and privileges which are intentionally
not assumed by this interface. It should implement the same sink contract rather
than leaking `audio.primary.normal.so` into the mathematical layers.

## Initial stream

The acceptance target for the MIRO A1 is 44100 Hz, stereo, signed 16-bit PCM,
matching the primary-output properties observed while tracing the vendor audio
stack. The generic interface reports the properties actually opened by AAudio;
callers must not assume the request was granted unchanged.

The backend uses blocking owner-thread writes. It does not run Fourier work from
an audio callback. Partial writes are exposed as frame counts so a caller can
retry or treat them as an acceptance failure.

Next physical acceptance step: feed a deterministic tone or the existing
Beethoven PCM through this sink, log the actual opened properties and frame
counts, and verify audible speaker output. Only after that should direct
`audio.primary.normal.so` / TinyALSA work be treated as necessary for the basic
speaker test.
