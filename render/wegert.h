#ifndef FOURIER_WEGERT_H
#define FOURIER_WEGERT_H

#include "dft.h"
#include <stdbool.h>
#include <stdint.h>

struct wegert_rgb {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

/* Minimal Wegert-style phase plot: argument is encoded by a circular hue map.
 * Near-zero and nonfinite values are black because their phase is undefined. */
bool wegert_phase_rgb(struct fourier_complex value, struct wegert_rgb *rgb);

#endif
