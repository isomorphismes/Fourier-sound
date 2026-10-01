# Reference rendering path

The first executable visualization path is intentionally split into independent
layers:

```text
PCM
  -> fourier_pcm_mono
  -> reference complex DFT
  -> mathematical construction
  -> sampled complex values
  -> Wegert color
  -> RGB pixels
  -> PPM file sink
```

The renderer does not receive Fourier coefficients and does not know which
mathematical construction produced the complex values.

## DFT

`fourier/dft.c` is a direct O(N^2) reference transform using

```text
c[k] = (1/N) sum_n x[n] exp(-2 pi i k n / N).
```

It deliberately retains all N complex coefficients. It is for correctness,
experimentation, and testing an eventual fast implementation, not the live
phone hot path.

## First construction

`fourier/complex_field.c` currently supplies the deliberately simple
polynomial

```text
f(z) = c[0] + c[1] z + ... .
```

This is a smoke-test construction only. It is not claimed to reproduce Owen
Maresh's lacunary q-series experiment. A q-series or any other complex field can
replace it without changing the color or file-output layers.

## Wegert color

`render/wegert.c` is a CPU port of the renderer-independent color core in
`isomorphismes/wegert/code/wegert_color.glsl` at commit
`296fbc6e916341d680c6c473bb490e6ce41b18d8`.

It preserves that implementation's:

- phase -> circular HCL hue;
- chroma 45;
- logarithmic modulus band;
- lightness `66 + 4*modulus_band + 3*hue_band`;
- CIE L*u*v* -> D65 XYZ -> sRGB conversion.

The host test contains fixed RGB reference points so an accidental replacement
with a generic HSV wheel fails visibly. Exact zero/pole marker representation
remains outside this color core, matching the ownership boundary in Wegert.

## PPM

`render/ppm.c` receives only RGB pixels. It has no dependency on Fourier
coefficients, complex functions, or the Wegert mapping. PPM is only the first
dependency-free still-image sink; it is suitable for tests and for feeding the
separate movie-frame path.
