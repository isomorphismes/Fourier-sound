# Framing primitives

Acquisition produces raw samples. Framing chooses the finite observation passed
to a decomposition. The implementation keeps those choices explicit instead
of hiding them inside an FFT call.

## Copy and overlap

`fourier_frame_copy` selects
`samples[start .. start + frame_count)` into caller-owned working storage and
never changes the raw sample buffer. Successive starts separated by
`hop_count < frame_count` therefore give overlap without a separate overlap
mode.

## Optional operations

The copied frame can independently receive mean removal, explicit gain, RMS
measurement, a periodic Hann window, or a symmetric Hann window. Doing nothing
gives a rectangular, mean-preserving, unit-gain frame.

The Hann convention is part of the function name rather than hidden policy:

- periodic: `0.5 - 0.5 cos(2 pi n / N)`, natural when the frame is treated as
  one period by a DFT/FFT;
- symmetric: denominator `N-1`, giving matching zero endpoints.

Singleton frames are unchanged by either convention.

Mean removal and gain are transactional: if a result cannot be represented as
finite `float`, the operation fails without partially mutating the frame.

This layer deliberately does not choose automatic voice normalization.
Maresh's observation that the visualization needed response tuning makes that
an experiment-level question. Sample-rate conversion is also separate; channel
selection already happens at the PCM-to-mono boundary.
