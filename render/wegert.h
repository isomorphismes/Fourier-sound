#ifndef FOURIER_WEGERT_H
#define FOURIER_WEGERT_H

#include "complex_value.h"
#include "rgb24.h"

#include <stdbool.h>
#include <stddef.h>

bool wegert_color_from_phase_log_modulus(double phase, double log_modulus,
                                         struct rgb24 *rgb);
bool wegert_color_complex(struct complex_value value, struct rgb24 *rgb);
bool wegert_color_values(const struct complex_value *values, size_t count,
                         struct rgb24 *pixels, size_t capacity);

#endif
