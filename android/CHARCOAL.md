# Charcoal application windows

All five owned Android manifests explicitly use Theme.Charcoal from the shared
android-NDK presentation source 3e682fb1175beef5625dbd2418730220e49fc9bd. Every
existing APK recipe packages the same resources and tracks their XML files as
incremental prerequisites. Package, SDK, version and permanent signing metadata
are unchanged; the compiler and native packager remain their existing owners.

Microphone and speaker diagnostics have no image producer. Their small native
window adapter posts one charcoal frame at APP_CMD_INIT_WINDOW, respecting row
stride and RGBA byte order. It adds no allocation or periodic drawing and does
not affect acquisition, permissions, timing or sample handling. Actual Android
execution is required to qualify this owned Icky C path and its posted pixels.

CPU/GPU Fourier Voice and the static render diagnostic retain the mathematical
image and Wegert color mapping. Framework permission dialogs are Android-owned
external content. No generated Java/Kotlin/DEX or new rendering framework is used.

The inherited source-built ABI, simulated audio, startup validation and real
recording tests remain required. Signed packages alone do not qualify live
microphone/speaker behavior or API29/34 both-mode startup/rotation appearance.
Physical C67 checks are NOT_RUN; no application is installed or published.

The inherited7c61ee packager ignores ANDROID_RES_DIR, as demonstrated by the
same source-material consumer failure in Mostow run41968006546. All three
Fourier package acquisition workflows and ci/platforms.tsv now explicitly pin
the existing resource-capable42f04d7654d54309ec3f8787bfd2bc40730399d7 owner.
Its receipt schema remains v1; signer, package and SDK requirements are preserved.
New actual Fourier packaging evidence is required after this material change.
