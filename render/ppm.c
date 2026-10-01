#include "ppm.h"

#include "complex_field.h"
#include "wegert.h"

#include <stdio.h>

bool fourier_render_ppm(const char *path,
                        const struct fourier_complex *coefficients,
                        size_t coefficient_count, size_t term_count,
                        size_t width, size_t height,
                        double xmin, double xmax, double ymin, double ymax)
{
    if (!path || !coefficients || !coefficient_count || !term_count ||
        width < 2 || height < 2 || !(xmin < xmax) || !(ymin < ymax))
        return false;

    FILE *file = fopen(path, "wb");
    if (!file) return false;
    if (fprintf(file, "P6\n%zu %zu\n255\n", width, height) < 0) {
        fclose(file);
        return false;
    }

    bool ok = true;
    for (size_t row = 0; row < height && ok; ++row) {
        double y = ymax - (ymax - ymin) * (double)row / (double)(height - 1);
        for (size_t column = 0; column < width; ++column) {
            double x = xmin + (xmax - xmin) * (double)column /
                                  (double)(width - 1);
            struct fourier_complex z = {x, y};
            struct fourier_complex value = fourier_polynomial_value(
                coefficients, coefficient_count, term_count, z);
            struct wegert_rgb rgb;
            if (!wegert_phase_rgb(value, &rgb)) {
                ok = false;
                break;
            }
            unsigned char bytes[3] = {rgb.red, rgb.green, rgb.blue};
            if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
                ok = false;
                break;
            }
        }
    }

    if (fclose(file) != 0) ok = false;
    return ok;
}
