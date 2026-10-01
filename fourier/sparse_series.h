#ifndef FOURIER_SPARSE_SERIES_H
#define FOURIER_SPARSE_SERIES_H

#include "complex_value.h"

#include <stdbool.h>
#include <stddef.h>

/* Evaluate sum_i coefficients[i] * q^exponents[i].
 *
 * exponents must be strictly increasing nonnegative integers. The caller owns
 * the exponent schedule: dense, lacunary, or otherwise. No schedule is inferred
 * from the Fourier coefficients. On failure, *value is left unchanged. */
bool fourier_sparse_series_value(
    const struct complex_value *coefficients, size_t coefficient_count,
    const size_t *exponents, size_t term_count,
    struct complex_value q, struct complex_value *value);

#endif
