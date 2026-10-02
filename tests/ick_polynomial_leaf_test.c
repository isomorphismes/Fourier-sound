#include "complex_field.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

void fourier_polynomial_cartesian_ick(
    const double *coefficients_cartesian, unsigned int coefficient_count,
    unsigned int term_count, double z_real, double z_imag,
    double output_cartesian[static 2]);

static int near(double left, double right)
{
    return fabs(left - right) <= 1e-12;
}

static void compare(struct complex_value z)
{
    const struct complex_value coefficients[] = {
        {1.0, 0.0},
        {2.0, 3.0},
        {-0.5, 1.0},
        {0.25, -0.75}
    };
    double packed[8];
    for (size_t index = 0U; index < 4U; ++index) {
        packed[index * 2U] = coefficients[index].real;
        packed[index * 2U + 1U] = coefficients[index].imaginary;
    }

    struct complex_value reference = fourier_polynomial_value(
        coefficients, 4U, 4U, z);
    double output[2];
    fourier_polynomial_cartesian_ick(
        packed, 4U, 4U, z.real, z.imaginary, output);

    assert(near(output[0], reference.real));
    assert(near(output[1], reference.imaginary));
}

int main(void)
{
    compare((struct complex_value){0.0, 0.0});
    compare((struct complex_value){1.0, 0.0});
    compare((struct complex_value){0.25, -0.5});
    compare((struct complex_value){-0.75, 0.4});
    puts("PASS ICK polynomial leaf matches reference complex-field evaluator");
    return 0;
}
