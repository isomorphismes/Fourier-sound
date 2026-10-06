#include "complex_field.h"

struct complex_value fourier_polynomial_evaluate(const void *state, struct complex_value point)
{
    const struct fourier_polynomial *polynomial = state;
    return fourier_polynomial_value(polynomial->coefficients, polynomial->coefficient_count,
                                    polynomial->term_count, point);
}

static struct complex_value multiply(struct complex_value a,
                                     struct complex_value b)
{
    return (struct complex_value){
        a.real * b.real - a.imaginary * b.imaginary,
        a.real * b.imaginary + a.imaginary * b.real
    };
}

struct complex_value fourier_polynomial_value(
    const struct complex_value *coefficients, size_t coefficient_count,
    size_t term_count, struct complex_value z)
{
    struct complex_value sum = {0.0, 0.0};
    struct complex_value power = {1.0, 0.0};
    if (!coefficients) return sum;
    if (term_count > coefficient_count) term_count = coefficient_count;

    for (size_t k = 0; k < term_count; ++k) {
        struct complex_value term = multiply(coefficients[k], power);
        sum.real += term.real;
        sum.imaginary += term.imaginary;
        power = multiply(power, z);
    }
    return sum;
}
