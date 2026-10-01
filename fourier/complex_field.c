#include "complex_field.h"

static struct fourier_complex multiply(struct fourier_complex a,
                                       struct fourier_complex b)
{
    return (struct fourier_complex){
        a.real * b.real - a.imaginary * b.imaginary,
        a.real * b.imaginary + a.imaginary * b.real
    };
}

struct fourier_complex fourier_polynomial_value(
    const struct fourier_complex *coefficients, size_t coefficient_count,
    size_t term_count, struct fourier_complex z)
{
    struct fourier_complex sum = {0.0, 0.0};
    struct fourier_complex power = {1.0, 0.0};
    if (!coefficients) return sum;
    if (term_count > coefficient_count) term_count = coefficient_count;

    for (size_t k = 0; k < term_count; ++k) {
        struct fourier_complex term = multiply(coefficients[k], power);
        sum.real += term.real;
        sum.imaginary += term.imaginary;
        power = multiply(power, z);
    }
    return sum;
}
