#ifndef FOURIER_COMPLEX_FIELD_H
#define FOURIER_COMPLEX_FIELD_H

#include "complex_value.h"

#include <stddef.h>

struct fourier_polynomial {
    const struct complex_value *coefficients;
    size_t coefficient_count, term_count;
};
struct complex_value fourier_polynomial_evaluate(const void *state, struct complex_value point);

struct complex_value fourier_polynomial_value(
    const struct complex_value *coefficients, size_t coefficient_count,
    size_t term_count, struct complex_value z);

#endif
