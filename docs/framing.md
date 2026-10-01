# Framing primitives

Acquisition produces raw samples. Framing chooses the finite observation passed
to a decomposition.

The first implementation keeps those choices explicit instead of hiding them
inside an FFT call.

## Copy and overlap

`fourier_frame_copy` selects

```text
samples[start .. start + frame_count)
```

into caller-owned working storage. It never changes the raw sample buffer.

Overlap is therefore not a mode inside the framing function. If successive
frames start at

```text
start_m = m * hop_count
```

then `hop_count < frame_count` gives overlap and
`hop_count == frame_count` gives adjacent frames.

## Optional operations

The copied frame can then independently receive:

- `fourier_frame_remove_mean`;
- `fourier_frame_scale` with an explicitly supplied gain;
- `fourier_frame_apply_hann`;
- `fourier_frame_rms` for measurement or an experiment's own normalization
  policy.

Doing nothing gives a rectangular, mean-preserving, unit-gain frame.

This intentionally does not choose an automatic voice normalization rule.
Maresh's observation that the visualization needed response tuning makes that
an experiment-level question; the framing layer supplies the measurements and
operations without deciding the target behavior.

Sample-rate conversion is not implemented here. Channel selection already
happens at the PCM-to-mono boundary and resampling should remain a separate
operation if an experiment needs it.

The tests also compose mean removal directly with the radix-2 FFT so this layer
is checked as part of the mathematical pipeline rather than only as isolated
array utilities.
