#ifndef FOURIER_DFT_H
#define FOURIER_DFT_H

#include <stdbool.h>
#include <stddef.h>

struct fourier_complex {
    double real;
    double imaginary;
};

/* Reference real-input DFT. The output has exactly sample_count coefficients,
 * using exp(-2*pi*i*k*n/N), normalized by 1/N. No windowing, mean removal,
 * truncation, or magnitude reduction is performed here. */
bool fourier_dft_real(const float *samples, size_t sample_count,
                      struct fourier_complex *coefficients, size_t capacity);

#endif
