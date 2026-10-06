#ifndef FOURIER_FFT_H
#define FOURIER_FFT_H

#include "complex_value.h"

#include <stdbool.h>
#include <stddef.h>

/* Iterative radix-2 Cooley-Tukey reference-fast backend.
 *
 * Produces the same full N complex coefficients and 1/N normalization as
 * fourier_dft_real. sample_count must be a power of two; this function does
 * not pad, window, remove the mean, or otherwise choose framing policy. */
bool fourier_fft_real_radix2(const float *samples, size_t sample_count,
                             struct complex_value *coefficients,
                             size_t capacity);

#endif
