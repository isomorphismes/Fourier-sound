# The smallest microphone-to-graphics program

**Question:** How little executable code and runtime machinery does a phone need
to read microphone samples and turn them directly into a moving mathematical image?

Florence Nightingale's 1890 recording is a *repeatable sample*, not the aim of
the project. The aim is to explore the phone's **hardware abstraction layer
(HAL)**, distinguish essential services from historical convenience, and make
the executable smaller. A program with **no libc dependency** is an explicit
experimental target, not a currently delivered result.

## The existing working source path

`android/voice_main.c` already connects these components:

```text
phone microphone
    ↓
Android audio service / AAudio
    ↓
audio/android/aaudio_input.c       live PCM, bounded ring
    ↓
fourier/pcm_block.c                mono samples
    ↓
fourier/framing.c + fourier/fft.c  framed coefficients
    ↓
fourier/complex_field.c            complex polynomial from coefficients
    ↓
render/complex_plot.c + wegert.c  RGB phase plot
    ↓
render/android/native_window_output.c
    ↓
phone display
```

This is a native Android `NativeActivity` implementation with **no application
DEX**, not a claim to have removed every operating-system library. The Makefile
links the viewer against AAudio, Android, logging and math libraries. Its C
implementation also uses libc services (for example allocation, system calls,
and time). The currently built native `.so` is **not** a libc-free executable.
Host tests and Android builds do not by themselves prove live device microphone
and display behavior; those require a physical acceptance receipt.

The [Florence Nightingale movie](https://github.com/isomorphismes/Fourier-sound/blob/experiments/nightingale-fourier-movie/media/florence-nightingale-fourier-voice.mp4)
is generated **offline** from a historical sound file. It checks one voice →
mathematics → image route. It is **not** footage of a phone recording its
microphone or evidence of direct access to an audio HAL.

## Which hardware abstraction layer?

Keep distinct:

1. The small **application-owned** `audio_input` C interface. We control this.
2. The **Android audio API/service** (currently AAudio) used by the application.
3. The **vendor audio HAL** and underlying drivers chosen by Android for the
   particular device and stream.

AAudio does not mean direct access to the vendor HAL. An ordinary application
may not have permission to open the HAL or a microphone kernel device itself.
The actual chain is device- and mode-dependent; inspect it rather than asserting
a universal direct hardware route.

Experiment with alternative backends behind the same small PCM boundary when
they are genuinely usable. Do not change Fourier math, color mapping or UI
merely to try another acquisition path.

## The no-libc and small-executable experiment

For each candidate, retain a machine-readable receipt recording:

- Exact source and compiler revision; Android model/ABI/API where applicable.
- Stripped and unstripped ELF bytes, APK bytes, and separately retained debug
  bytes. Do not use APK compression to disguise native code size.
- `readelf -d` `NEEDED` entries, unresolved symbols and the real **transitive**
  dynamic-library closure. Distinguish `libc.so` in the program from libraries
  loaded by Android services.
- The audio route actually used, permitted microphone access, captured frame
  count and format, dropped-frame count, and rendered output.
- An actual sound-response test (quiet versus audible input) and a display
  receipt on the phone, not merely a successful compilation.

Follow the stages in order:

1. **Baseline:** measure the existing AAudio → PCM → Fourier → window path.
2. **Remove app-owned overhead:** compare a stripped minimal viewer with the
   existing one while retaining permission, lifecycle and error handling.
3. **Freestanding core:** compile and check a small mathematical/rendering leaf
   without libc; inspect its relocations and imports. A libc-free *leaf* does
   **not** prove that the host process or APK is libc-free.
4. **HAL exploration:** document which device audio interfaces are accessible
   to an ordinary app, which need a platform service or elevated permissions,
   and the costs of each. Never present a mock input stream as microphone data.
5. **No-libc end-to-end candidate:** claim success only if its actual launched
   ELF has no libc dependency and real microphone → image behavior is accepted
   on the target phone. If Android's supported microphone path makes that
   impossible for an ordinary app, document the boundary and keep the working
   AAudio implementation.

The goal is a genuinely simple program, not a small reported byte count that
hides essential code in another process or omits permissions, audio acquisition,
or drawing.
