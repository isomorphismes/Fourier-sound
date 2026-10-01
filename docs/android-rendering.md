# Android window output

The first Android display boundary consumes already-rendered RGB24 pixels. It
does not know about Fourier transforms, q-series, microphone input, or complex
functions.

`render/rgb24_rgba8888.c` converts RGB24 to RGBA8888 while respecting Android
row stride and leaving padding pixels untouched. This conversion has an
Android-free host test.

`render/android/native_window_output.c` is the thin platform sink:

```text
RGB24 pixels
  -> request RGBA8888 ANativeWindow buffer
  -> lock
  -> stride-aware copy
  -> unlock and post
```

The acceptance APK generates a fixed portrait of `z^3 - 1` using the existing
complex-field and canonical Wegert color code, presents it at the current
window size, holds it for five seconds, and logs `DISPLAY_RESULT`.

This is intentionally a static acceptance producer. Once the MIRO A1 screen
path is physically verified, live sound-driven frames can replace the producer
without changing the window sink.
