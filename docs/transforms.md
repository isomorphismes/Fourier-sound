# Fourier transform backends

Fourier-sound keeps mathematical transform semantics separate from framing and
algorithm choice.

Both current transforms implement

```text
c[k] = (1/N) sum_n x[n] exp(-2 pi i k n / N)
```

and return all N complex coefficients.

## Direct DFT

`fourier/dft.c` is the O(N^2) correctness oracle. It is intentionally simple
and remains the reference used to test faster implementations.

## Radix-2 FFT

`fourier/fft.c` is an iterative radix-2 Cooley-Tukey backend. It writes into
the caller-provided coefficient buffer, performs one trigonometric setup per
stage, and advances twiddle factors by complex multiplication inside each
stage.

It accepts only power-of-two sample counts. It does not silently pad data.

That rejection is deliberate: choosing block length, overlap, windowing,
normalization, resampling, or padding belongs to the framing layer, not to an
FFT implementation.

`tests/fft_test.c` compares the FFT coefficient-by-coefficient against the
direct DFT for impulses, shifted impulses, constants, mixed harmonics, and
deterministic pseudo-random signals through 256 samples. It also checks invalid
sizes and nonfinite input.

The FFT is now suitable as the first fast backend, but it is not yet selected by
a live microphone pipeline. That selection should happen only after the framing
contract is explicit.
