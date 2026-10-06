# Native microphone boundary

## Recovered precedent

Inspected `Ashtray-Archer/utilities-android-phone-user` at
`a8722e10318c63ea656729ee2c2cce526a423754`:

- [Shared acquisition adapter](https://github.com/Ashtray-Archer/utilities-android-phone-user/tree/a8722e10318c63ea656729ee2c2cce526a423754/accelerometer/android)
- [NativeActivity application](https://github.com/Ashtray-Archer/utilities-android-phone-user/blob/a8722e10318c63ea656729ee2c2cce526a423754/accelerometer/app/src/main/c/native_main.c)
- [Build](https://github.com/Ashtray-Archer/utilities-android-phone-user/blob/a8722e10318c63ea656729ee2c2cce526a423754/accelerometer/build-apk.sh)
- [Host tests](https://github.com/Ashtray-Archer/utilities-android-phone-user/tree/a8722e10318c63ea656729ee2c2cce526a423754/accelerometer/tests)
- [Hardware and physical acceptance boundary](https://github.com/Ashtray-Archer/utilities-android-phone-user/blob/a8722e10318c63ea656729ee2c2cce526a423754/hardware/README.md)

| Part | Accelerometer | Audio implementation |
| --- | --- | --- |
| APK | C NativeActivity, NDK glue, no app DEX | Same |
| Lifecycle | Open adapter, enable on focus, disable on pause/lost focus, close on destroy | Open/start with permission and resumed focus; stop/close on pause/lost focus; close on destroy |
| Acquisition | ASensorManager event queue, drained by ALooper application thread | AAudio callback copies into bounded SPSC buffer; eventfd wakes ALooper consumer |
| Semantic boundary | Timestamp and three Android-reported floats, shared by APK and CLI; adapter header contains Android types | Generic opaque source and interleaved PCM properties; no Android types in public interface |
| Processing | Model conversion and rendering after queue drain | PCM conversion/statistics after buffer drain, never in the callback |
| Permission | Ordinary accelerometer requires none | Manifest RECORD_AUDIO plus runtime request through two framework methods via JNI |
| Build | NDK clang, aapt2, zipalign, apksigner, persistent public test key, ARMv7 Thumb | Same packaging tools, key and ABI targets; direct Make recipes because the repository had no build files |
| Evidence | Model tests, packaged APK checks, separate emulator/phone receipts | Ring/model tests plus explicitly simulated AAudio lifecycle; actual NDK APK build and physical microphone remain separate gates |

The repository was empty at initial inspection. While implementation proceeded,
main acquired the project notes through `23b91f1f7e48290a5e88c06a6e50b2dd2c066dee`;
the audio branch is based on that history and preserves its architectural notes.
No pre-existing executable Fourier mathematics or visualization could be wired
in. Instead `fourier/pcm_block`
is the actual exercised PCM entry point: it converts granted float32/signed16
interleaved frames into ordinary mono float samples for future mathematical code.
The acceptance program consumes captured audio through this same path.

## Ownership and timing

One ordinary application thread owns open, properties, read, wait, start, stop
and close. One AAudio data-callback producer owns the ring write cursor; the
consumer owns its read cursor. Release/acquire publication protects sample
storage. Every atomic used by either callback is checked for lock-free operation
at open, including on ARMv7. Ring storage and stream allocation happen before
start. No allocation, Fourier work, rendering, file I/O, locks, or logging occurs
in the data callback. Its only syscall is one nonblocking eventfd write.

The fixed 32768-frame ring holds about 0.68 seconds at 48 kHz. Full buffers drop
incoming frames and increment a saturating drop counter. The acceptance mode
fails on any observed drop, so it cannot silently analyze a discontinuous block.
Future analysis must discard/reset its window on a changed counter. This first
interface promises ordered frames, not timestamps, gap positions or clock sync.
The broader architecture's conceptual timestamp field remains future work; this
slice does not label consumer delivery time as microphone capture time.

Actual sample rate, channels and representation are read from the opened stream.
Preferred request: shared input, mono, 48000 Hz, float32. Unsupported rate retries
with an unspecified rate; unsupported float retries signed16. Unsupported actual
formats and channel counts outside 1–8 fail explicitly. Samples are not assumed
to be unprocessed ADC readings, and no claim about physical sample rate or
manufacturer microphone routing follows from AAudio stream properties.

`audio/android/aaudio_input.h` exposes only a borrowed readiness descriptor to the
Android lifecycle harness. Portable consumers can use `audio_input_wait` instead.
Neither mathematical code nor generic headers contain AAudio or Android types.
Later Linux/SoC/device implementations can implement the same functions as
separate build selections. No unused backend directories or dispatch framework
are required now.

Error callbacks only retain an error and signal readiness. The owner closes the
stream after disconnect; it never closes in either AAudio callback. Stop waits
for STOPPED with a two-second deadline. Close calls the platform close before
freeing callback storage. Restart after a clean stop resets buffered samples and
overflow counts. Failed/disconnected streams require close and reopen.

## Permission and acceptance UI

The NDK glue entry point is renamed only at compilation. A small exported C
wrapper calls it and requests RECORD_AUDIO on the framework UI thread. The
application thread rechecks permission on resume/focus before opening audio.
Denial leaves recording stopped and writes a clear log message; the app does not
repeatedly prompt within the same activity. Grant in settings and reopen if
needed. No Java/Kotlin source or app DEX is generated. Framework JNI is confined
to permission checking and presentation; it does not acquire audio.

This is a log-based native acceptance activity, with no visualization yet. It
captures five seconds by actual frame count, times out after ten wall-clock
seconds, reports normalized statistics across all channels, closes, and exits.
All-zero PCM is explicitly inconclusive: microphone privacy controls, muting,
silence or a broken path can all produce it. Compare quiet and speaking/tapping
runs; nonzero samples alone do not prove an acoustic response.

References: [AAudio guide](https://developer.android.com/ndk/guides/audio/aaudio/aaudio),
[AAudio API](https://developer.android.com/ndk/reference/group/audio),
[Activity permission API](https://developer.android.com/reference/android/app/Activity#requestPermissions(java.lang.String[],int)).
