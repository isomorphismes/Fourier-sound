# Native speaker output

This branch adds the first Fourier-sound output sink and the experiment-facing
speaker input that feeds it.

```text
experiment / Fourier / sensor logic
    |
speaker_input
    |
format + channel rendering
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

## Speaker input

`speaker_input` is deliberately above the hardware sink. Experiment code supplies
a mono float waveform buffer and chooses whether it loops. The buffer can come
from a tone generator, a transformed microphone block, sensor-driven synthesis,
a saved waveform, or later experiment machinery.

The boundary provides:

- `speaker_input_set`: select an arbitrary caller-owned waveform and loop policy;
- `speaker_input_reset`: rewind it;
- `speaker_input_finished`: detect completion of a non-looping buffer;
- `speaker_input_render`: convert the waveform into the actual output stream
  channel count and PCM representation.

Rendering clips hardware-bound samples to [-1,1] and duplicates mono data across
the actual speaker channels. It does not perform Fourier analysis, generate a
tone, own Android objects, or choose an experiment.

The current structure is intentionally simple and same-thread. A future
continuously changing experiment can replace the selected buffer or add a
streaming/ring producer without changing `audio_output`.

## Why AAudio is the first HAL hook

AAudio is the supported native application boundary. It exercises the real
vendor HAL and speaker route without requiring Fourier-sound itself to become an
Android platform component. That is the shortest route to an audible acceptance
test and is compatible with an ordinary NDK build.

A separate direct-HAL backend can still be useful on the rooted MIRO A1. That
backend will need platform/vendor headers and privileges which are intentionally
not assumed by this interface. It should implement the same sink contract rather
than leaking `audio.primary.normal.so` into the mathematical layers.

## Initial runnable app

`fourier-speaker-<abi>.apk` is a separate NativeActivity application and requires
no microphone permission. Its acceptance producer allocates a one-second 440 Hz
mono float buffer at the stream's actual sample rate, installs that buffer into
`speaker_input` with looping enabled, and plays three seconds through
`audio_output`.

The 440 Hz source is only a deterministic producer fixture. The application path
does not depend on a built-in tone: replacing that buffer with experiment output
uses the same `speaker_input_set` call.

The requested MIRO A1 stream is 44100 Hz, stereo, signed 16-bit PCM, matching the
primary-output properties observed while tracing the vendor audio stack. The
generic interface records the properties actually opened by AAudio; callers do
not assume the request was granted unchanged.

The backend uses blocking owner-thread writes. Partial writes are exposed as
frame counts and the application retains and retries the unwritten tail rather
than dropping experiment data.

Physical acceptance remains the next gate: install the ARMv7 speaker APK on the
MIRO A1, launch it, hear the three-second tone, and confirm
`SPEAKER_RESULT status=PLAYED` with the actual stream properties. Only if that
fails below AAudio do we need direct `audio.primary.normal.so` / TinyALSA work
for the basic speaker test.
