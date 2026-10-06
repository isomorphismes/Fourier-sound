#ifndef FOURIER_COMPLEX_PLOT_H
#define FOURIER_COMPLEX_PLOT_H
#include "complex_value.h"
#include "rgb24.h"
#include <stdbool.h>
#include <stddef.h>

/* A map C -> C borrows its mathematical state; plotting does not own it. */
struct complex_mapping {
    const void *state;
    struct complex_value (*evaluate)(const void *state, struct complex_value point);
};
struct complex_plot_domain { double x_radius, y_radius; size_t width, height; };
bool complex_plot_raster(struct complex_mapping mapping, struct complex_plot_domain domain,
                         struct rgb24 *pixels, size_t capacity);
#endif
