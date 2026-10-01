#ifndef FOURIER_WEGERT_H
#define FOURIER_WEGERT_H

#include "dft.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct wegert_rgb {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/* CPU port of the canonical color core in isomorphismes/wegert
 * code/wegert_color.glsl at 296fbc6e916341d680c6c473bb490e6ce41b18d8.
 *
 * Phase selects HCL hue. Logarithmic modulus supplies the repeating brightness
 * band used by the Wegert renderer. */
bool wegert_color_from_phase_log_modulus(double phase, double log_modulus,
                                         struct wegert_rgb *rgb);
bool wegert_color_complex(struct fourier_complex value,
                          struct wegert_rgb *rgb);

/* Color already-sampled complex values. The renderer does not know what
 * transform or mathematical construction produced them. */
bool wegert_color_values(const struct fourier_complex *values, size_t count,
                         struct wegert_rgb *pixels, size_t capacity);

#endif
