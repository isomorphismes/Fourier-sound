#ifndef FOURIER_COMPLEX_FIELD_H
#define FOURIER_COMPLEX_FIELD_H

#include "dft.h"
#include <stddef.h>

/* Deliberately simple first construction: c[0] + c[1]z + ... .
 * term_count may select any prefix without changing the transform object. */
struct fourier_complex fourier_polynomial_value(
    const struct fourier_complex *coefficients, size_t coefficient_count,
    size_t term_count, struct fourier_complex z);

#endif
