# S1 physical acceptance candidates

Source commit: `af507e9f6d7c0de0af22a55b2950a21082f604d4`.
Package: `org.isomorphismes.fouriersound.voice`, version code 6.
Each APK has its canonical packaging receipt beside it.

Both variants are signed with the existing public development signer.
`miro` is the primary A1 armeabi-v7a target; `c67` is native arm64-v8a.
Diagnostic builds audit actual GPU coefficients and framebuffer samples on
frame 1 and every twentieth live frame. Release builds validate at startup.

These are physical acceptance candidates, not promoted Cat Food packages.
The Flexible Pipes producer gate rejects the unregistered Fourier Voice
package/signer lane. Device results remain NOT_RUN until evidence arrives.
Artifact publication does not certify physical microphone, lifecycle, GPU,
or live-audio behavior, and does not publish or merge the implementation branch.

Replace-install preserves existing app data. Stop on signing/downgrade errors;
never uninstall to bypass them. Verify the APK hash against its receipt first.
