# ICK path for Fourier Voice

The MIRO A1 application remains a native Android NativeActivity. There is still
no DEX or Java/Kotlin application code.

This branch begins using ICK inside the real Fourier Voice render path without
pretending ICK is already a complete Android linker/sysroot driver.

The split is:

```text
Fourier Voice C
  -> ICK arm-linux-gnueabi-gcc
       -march=armv7-a -mthumb -mfpu=neon -mfloat-abi=softfp
       -O2 -fPIC -ffreestanding -nostdinc
  -> fourier_voice_leaf.o            (ARMv7 Thumb-2)

remaining Android/native C
  -> Android NDK Clang

ICK object + NDK objects
  -> Android NDK lld
  -> libfourier_voice.so
  -> APK
```

The first ICK-owned production leaf is the polynomial evaluator used for every
rendered pixel. Its boundary contains only doubles, unsigned integers, and
arrays; no `_Complex` value crosses between ICK and Clang.

The app retains the ordinary C evaluator for a startup cross-check. A hardware
run should log `VOICE_ICK_SELF_TEST status=PASS` before interpreting the
display.

ICK is pinned by commit in the workflow. The compiler is rebuilt from ICK's
owned source plus pinned GCC reference during CI; no opaque compiler binary is
checked into Fourier-sound.
