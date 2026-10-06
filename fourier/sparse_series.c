#include "sparse_series.h"

#include <math.h>

static bool finite_complex(struct complex_value value)
{
    return isfinite(value.real) && isfinite(value.imaginary);
}

static bool multiply(struct complex_value a, struct complex_value b,
                     struct complex_value *out)
{
    double real = a.real * b.real - a.imaginary * b.imaginary;
    double imaginary = a.real * b.imaginary + a.imaginary * b.real;
    if (!isfinite(real) || !isfinite(imaginary)) return false;
    *out = (struct complex_value){real, imaginary};
    return true;
}

static bool integer_power(struct complex_value base, size_t exponent,
                          struct complex_value *out)
{
    struct complex_value result = {1.0, 0.0};
    struct complex_value power = base;

    while (exponent) {
        if (exponent & 1U) {
            struct complex_value next;
            if (!multiply(result, power, &next)) return false;
            result = next;
        }
        exponent >>= 1U;
        if (exponent) {
            struct complex_value next;
            if (!multiply(power, power, &next)) return false;
            power = next;
        }
    }

    *out = result;
    return true;
}

bool fourier_sparse_series_value(
    const struct complex_value *coefficients, size_t coefficient_count,
    const size_t *exponents, size_t term_count,
    struct complex_value q, struct complex_value *value)
{
    if (!coefficients || !exponents || !value || !term_count ||
        term_count > coefficient_count || !finite_complex(q))
        return false;

    for (size_t index = 0U; index < term_count; ++index) {
        if (!finite_complex(coefficients[index])) return false;
        if (index && exponents[index] <= exponents[index - 1U]) return false;
    }

    struct complex_value sum = {0.0, 0.0};
    for (size_t index = 0U; index < term_count; ++index) {
        struct complex_value power;
        struct complex_value term;
        if (!integer_power(q, exponents[index], &power) ||
            !multiply(coefficients[index], power, &term))
            return false;

        double real = sum.real + term.real;
        double imaginary = sum.imaginary + term.imaginary;
        if (!isfinite(real) || !isfinite(imaginary)) return false;
        sum = (struct complex_value){real, imaginary};
    }

    *value = sum;
    return true;
}
