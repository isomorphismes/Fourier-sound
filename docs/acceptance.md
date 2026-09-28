# Build and physical acceptance

Host: run `make test` from the repository, or `make -C /absolute/checkout test`.
Tests include deterministic ring wrap/overflow and cursor rollover, a 200000-frame
producer/consumer ordering test, PCM conversion, silence/statistics, and a test
double for AAudio negotiation/lifecycle/error cleanup. That double is not an
Android runtime, emulator or microphone test.

Clean Android builds run in `.github/workflows/native-audio.yml`, with NDK
27.2.12479018, SDK 36/build-tools 36.0.0, minimum API 26, and ARMv7 Thumb, AArch64
and x86_64 packages. Local equivalent: `make android` with `ANDROID_HOME` pointing
to those installed tools and `ANDROID_KEYSTORE` to the same persistent test key.
The pinned signer checkout is specified in the workflow. Never create a fresh
temporary key or uninstall to solve a signer conflict. Java is used by Android's
SDK signing utility only; no app Java/Kotlin/DEX or Gradle build is involved.

The workflow verifies native-only APK contents, ELF identity, APK alignment and
the existing public test signer fingerprint, then uploads APKs, SHA-256 files and
the exact source commit in one artifact. The package is
`org.isomorphismes.fouriersound`, version code 1. ABI-specific APKs share identity.

## MIRO A1

Download the `armeabi-v7a` APK and its matching SHA-256 file from the exact workflow
artifact linked in the delivery receipt. Preserve `source-commit.txt`. Use the
already installed Grease runtime and adb connection; no device build is needed.

Copy-paste invocation after replacing the three explicit inputs:

```text
grease /absolute/checkout/acceptance/miro-a1.grease EXACT_ADB_SERIAL /absolute/download/fourier-microphone-armeabi-v7a.apk EXPECTED_SHA256
```

The script checks model, ABI and digest before replacement installation. It grants
RECORD_AUDIO, launches this activity, and streams its tagged logs. It uses explicit
paths and device serial and never clears logs or uninstalls. Grease syntax follows
the canonical repository's `examples/accelerometer-inspection.ysh`; execution on
the actual Grease/adb host still requires physical acceptance.

1. Run once in a quiet room, then again while speaking or tapping near the mic.
   Match logs by the new `MIC_TEST` line, timestamp and process. Previous entries
   may be present because the script deliberately preserves device logs.
2. Save `MIC_OPEN`, `MIC_CLOSE` and `MIC_RESULT`. Expect five seconds worth of
   frames, samples = frames × channels, mono_samples = frames, dropped = 0,
   clean_close = 1. Rate/format are granted stream properties, not requested ones.
3. `SAMPLES_RECEIVED` with changed RMS/min/max between quiet and audible runs
   supports live acoustic acquisition. `ALL_ZERO_INCONCLUSIVE` proves delivered
   zero buffers only. Check microphone privacy toggle and permission, then repeat.
   TIMEOUT, OVERFLOW, INPUT_ERROR or any unclean closure fails acceptance.
4. Check normal permission flow separately: revoke RECORD_AUDIO in Android app
   settings and launch by tapping the icon; allow the prompt. Repeat with denial:
   no MIC_OPEN or recording should start. Grant in settings and reopen to recover.
5. During capture press Home. Expect stop/close and no continuing capture. Return
   to restart a fresh five-second run. Force-stop/relaunch to verify reuse; install
   the same APK with replacement again to verify package identity preservation.

Record model, build fingerprint, ABI, APK digest/size, source commit, installed
version, permission/foreground transitions, actual statistics and observed sound
response. Build/host success cannot substitute for this receipt. TAB_P10 requires
its own model/ABI check and receipt; the MIRO script refuses that device.
