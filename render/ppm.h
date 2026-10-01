#ifndef FOURIER_PPM_H
#define FOURIER_PPM_H

#include "wegert.h"

#include <stdbool.h>
#include <stddef.h>

/* File sink only: no Fourier or complex-function assumptions. */
bool rgb24_write_ppm(const char *path, const struct wegert_rgb *pixels,
                     size_t width, size_t height, size_t capacity);

#endif
