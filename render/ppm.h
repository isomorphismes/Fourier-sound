#ifndef FOURIER_PPM_H
#define FOURIER_PPM_H

#include "dft.h"
#include <stdbool.h>
#include <stddef.h>

bool fourier_render_ppm(const char *path,
                        const struct fourier_complex *coefficients,
                        size_t coefficient_count, size_t term_count,
                        size_t width, size_t height,
                        double xmin, double xmax, double ymin, double ymax);

#endif
