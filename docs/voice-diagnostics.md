# Fourier Voice diagnostic pass

The first MIRO A1 hardware run established that microphone acquisition, coefficient construction, Wegert coloring, native-window presentation, and the application lifecycle all work together. It also exposed two confounded effects: microphone/recording noise and high-degree polynomial growth outside the unit disk.

This branch changes only what is needed to separate those effects.

- The live analysis frame is 1024 samples. At 48 kHz this is 21.333 ms and the bin spacing is 46.875 Hz.
- The live transform is the radix-2 FFT already checked against the direct DFT.
- Mean removal and a periodic Hann window are explicit framing operations.
- The displayed polynomial still uses the first 24 coefficients. No noise gate, threshold, denoiser, normalization, or automatic gain is introduced.
- The 96 x 192 render rectangle is [-0.44, 0.44] x [-0.88, 0.88]. Its corners have modulus about 0.984, so high powers cannot explode merely because the screen samples large |z|.
- Every displayed frame logs input RMS, framed RMS, and the five strongest positive-frequency bins with bin number, frequency, magnitude, and phase.
- On startup the phone runs a microphone-independent coefficient check at a nominal 48 kHz: a 375 Hz cosine lands in bin 8 and a 750 Hz sine lands in bin 16 of the 1024-point FFT. The expected normalized coefficients are checked numerically and the result is logged as VOICE_SELF_TEST status=PASS or FAIL.

The self-test is intentionally before microphone interpretation. If it passes on the ARMv7 phone, coefficient arithmetic is separated from whatever noise or frequency content the microphone is actually producing.
