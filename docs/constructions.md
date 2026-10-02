# Mathematical constructions

Transform output and visualization remain separate. A Fourier transform produces
coefficients; a construction decides what those coefficients mean as a complex
function or geometry.

## Dense polynomial smoke test

`fourier/complex_field.c` evaluates the existing simple construction

```text
c[0] + c[1] z + c[2] z^2 + ...
```

It remains useful as a minimal end-to-end test but is not presented as the
voice-driven q-series from Owen Maresh's video.

## Explicit sparse power / q-series

`fourier/sparse_series.c` evaluates

```text
sum_i c[i] q^(e[i])
```

for a caller-supplied strictly increasing integer exponent list `e`.

This supports dense series and lacunary experiments without guessing the gap
schedule. For example, an experiment may choose `0, 1, 2, 4, 8, ...`, but the
library does not make that choice automatically.

The evaluator uses integer exponentiation by squaring, detects nonfinite
intermediate results, and leaves the output unchanged on failure.

## Maresh boundary

The public `graveolensa/tsungfruve` tree contains substantial q-series,
q-Pochhammer, lacunary-function, and phase-portrait experiments, including
`qpochpoly.py` and `complex-geography/mpmath/zetlacun.py`. Inspection did
not recover the audio/FFT driver or the exact exponent/coefficient mapping used
in the voice video.

Therefore Fourier-sound should keep the exact Maresh construction as an open
research question. The sparse-series primitive exists so candidate mappings can
be tested explicitly rather than smuggled in as an assumed reproduction.
