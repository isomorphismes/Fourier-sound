#ifndef FOURIER_DFT_H
#define FOURIER_DFT_H

#include "complex_value.h"

#include <stdbool.h>
#include <stddef.h>

bool fourier_dft_real(const float *samples, size_t sample_count,
                      struct complex_value *coefficients, size_t capacity);

#endif
