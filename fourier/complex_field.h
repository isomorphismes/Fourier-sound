#ifndef FOURIER_COMPLEX_FIELD_H
#define FOURIER_COMPLEX_FIELD_H

#include "complex_value.h"

#include <stddef.h>

struct complex_value fourier_polynomial_value(
    const struct complex_value *coefficients, size_t coefficient_count,
    size_t term_count, struct complex_value z);

#endif
