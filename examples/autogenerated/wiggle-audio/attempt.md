# Audio-window adapter

Input: a generic evaluated wiggle-v1 timeline containing integer
`audio.window_end` source-sampled states, a retained `clip.s16` fixture (48000 Hz,
mono signed 16-bit little endian), and an empty output frame directory.
Output: numbered PPMs and all 128 Fourier coefficients at each exact sample
endpoint, using the existing Nightingale prototype's window, DFT, polynomial
and Wegert functions without changing their algorithm or colors.

The adapter is deliberately a thin existing-C-engine boundary. Idriç's typed
control/interpolation model lives in Kitchen's generated wiggle-timeline
example; filesystem/process orchestration is checked Ithon. No replacement
Fourier renderer is proposed. Build through the repository's admitted ICK
compiler with `fourier`, `math`, and `render` include paths and the original
DFT/complex-field/Wegert/PPM translation units. Compiler is unavailable in the
current workspace: compilation and adapter execution are NOT_RUN. Generic cc
has not been substituted. Source presence is not a movie or math receipt.

The output is offline prototype evidence only. It says nothing about microphone,
AAudio/HAL, Android window drawing, or physical handset execution. The original
prototype analyses windows ending one frame interval ahead of video frame
onset; preserve and declare that 1/5-second lookahead rather than silently
changing the historical algorithm.
