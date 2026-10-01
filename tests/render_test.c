#include "complex_field.h"
#include "dft.h"
#include "ppm.h"
#include "wegert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define SAMPLE_COUNT 16U

static bool near(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

int main(int argc, char **argv)
{
    const double tau = 6.283185307179586476925286766559;
    float samples[SAMPLE_COUNT];
    for (size_t n = 0; n < SAMPLE_COUNT; ++n) {
        double t = (double)n / SAMPLE_COUNT;
        samples[n] = (float)(0.1 + 0.75 * cos(tau * 2.0 * t) +
                                   0.25 * sin(tau * 3.0 * t));
    }

    struct fourier_complex coefficients[SAMPLE_COUNT];
    assert(fourier_dft_real(samples, SAMPLE_COUNT, coefficients, SAMPLE_COUNT));
    assert(near(coefficients[0].real, 0.1, 1e-6));
    assert(near(coefficients[0].imaginary, 0.0, 1e-6));
    assert(near(coefficients[2].real, 0.375, 1e-6));
    assert(near(coefficients[14].real, 0.375, 1e-6));
    assert(near(coefficients[3].imaginary, -0.125, 1e-6));
    assert(near(coefficients[13].imaginary, 0.125, 1e-6));

    for (size_t k = 0; k < SAMPLE_COUNT; ++k) {
        if (k == 0 || k == 2 || k == 3 || k == 13 || k == 14) continue;
        assert(hypot(coefficients[k].real, coefficients[k].imaginary) < 1e-6);
    }

    struct fourier_complex simple[] = {{1.0, 0.0}, {2.0, 0.0}};
    struct fourier_complex at_i = fourier_polynomial_value(
        simple, 2, 2, (struct fourier_complex){0.0, 1.0});
    assert(near(at_i.real, 1.0, 1e-12));
    assert(near(at_i.imaginary, 2.0, 1e-12));

    struct wegert_rgb rgb;
    assert(wegert_phase_rgb((struct fourier_complex){1.0, 0.0}, &rgb));
    assert(rgb.red == 255 && rgb.green == 0 && rgb.blue == 0);
    assert(wegert_phase_rgb((struct fourier_complex){-1.0, 0.0}, &rgb));
    assert(rgb.red == 0 && rgb.green == 255 && rgb.blue == 255);
    assert(wegert_phase_rgb((struct fourier_complex){0.0, 0.0}, &rgb));
    assert(rgb.red == 0 && rgb.green == 0 && rgb.blue == 0);

    const char *path = argc > 1 ? argv[1] : "fourier-render.ppm";
    assert(fourier_render_ppm(path, coefficients, SAMPLE_COUNT, 8,
                              96, 96, -1.25, 1.25, -1.25, 1.25));

    printf("PASS full complex DFT coefficients, polynomial field, phase colour, PPM %s\n",
           path);
    return 0;
}
