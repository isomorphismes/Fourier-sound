#include "complex_field.h"
#include "sparse_series.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

static bool near(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

static void dense_matches_polynomial(void)
{
    struct complex_value coefficients[] = {
        {1.0, 0.25},
        {-0.5, 0.75},
        {0.125, -0.25},
        {0.5, 0.0}
    };
    size_t exponents[] = {0U, 1U, 2U, 3U};
    struct complex_value points[] = {
        {0.0, 0.0},
        {0.5, 0.0},
        {0.0, 0.5},
        {-0.25, 0.75}
    };

    for (size_t index = 0U; index < 4U; ++index) {
        struct complex_value sparse = {99.0, 99.0};
        struct complex_value dense = fourier_polynomial_value(
            coefficients, 4U, 4U, points[index]);
        assert(fourier_sparse_series_value(
            coefficients, 4U, exponents, 4U, points[index], &sparse));
        assert(near(sparse.real, dense.real, 1e-12));
        assert(near(sparse.imaginary, dense.imaginary, 1e-12));
    }
}

static void explicit_lacunary_exponents(void)
{
    struct complex_value coefficients[] = {
        {1.0, 0.0},
        {2.0, 0.0},
        {3.0, 0.0}
    };
    size_t exponents[] = {0U, 2U, 5U};

    struct complex_value value;
    assert(fourier_sparse_series_value(
        coefficients, 3U, exponents, 3U,
        (struct complex_value){0.5, 0.0}, &value));
    assert(near(value.real, 1.59375, 1e-12));
    assert(near(value.imaginary, 0.0, 1e-12));

    assert(fourier_sparse_series_value(
        coefficients, 3U, exponents, 3U,
        (struct complex_value){0.0, 1.0}, &value));
    assert(near(value.real, -1.0, 1e-12));
    assert(near(value.imaginary, 3.0, 1e-12));
}

static void large_exponent(void)
{
    struct complex_value coefficient[] = {{1.0, 0.0}};
    size_t exponent[] = {1048576U};
    struct complex_value value;
    assert(fourier_sparse_series_value(
        coefficient, 1U, exponent, 1U,
        (struct complex_value){1.0, 0.0}, &value));
    assert(value.real == 1.0 && value.imaginary == 0.0);
}

static void rejected_inputs_leave_output_unchanged(void)
{
    struct complex_value coefficients[] = {
        {1.0, 0.0}, {2.0, 0.0}, {3.0, 0.0}
    };
    size_t unordered[] = {0U, 5U, 2U};
    size_t duplicate[] = {0U, 2U, 2U};
    struct complex_value sentinel = {17.0, -9.0};
    struct complex_value output = sentinel;

    assert(!fourier_sparse_series_value(
        coefficients, 3U, unordered, 3U,
        (struct complex_value){0.5, 0.0}, &output));
    assert(output.real == sentinel.real && output.imaginary == sentinel.imaginary);

    assert(!fourier_sparse_series_value(
        coefficients, 3U, duplicate, 3U,
        (struct complex_value){0.5, 0.0}, &output));
    assert(!fourier_sparse_series_value(
        coefficients, 2U, unordered, 3U,
        (struct complex_value){0.5, 0.0}, &output));
    assert(!fourier_sparse_series_value(
        coefficients, 3U, unordered, 0U,
        (struct complex_value){0.5, 0.0}, &output));

    coefficients[1].real = NAN;
    size_t ordered[] = {0U, 1U, 2U};
    assert(!fourier_sparse_series_value(
        coefficients, 3U, ordered, 3U,
        (struct complex_value){0.5, 0.0}, &output));

    coefficients[1].real = 2.0;
    assert(!fourier_sparse_series_value(
        coefficients, 3U, ordered, 3U,
        (struct complex_value){NAN, 0.0}, &output));

    struct complex_value huge[] = {{DBL_MAX, 0.0}};
    size_t square[] = {2U};
    assert(!fourier_sparse_series_value(
        huge, 1U, square, 1U,
        (struct complex_value){2.0, 0.0}, &output));

    assert(output.real == sentinel.real && output.imaginary == sentinel.imaginary);
}

int main(void)
{
    dense_matches_polynomial();
    explicit_lacunary_exponents();
    large_exponent();
    rejected_inputs_leave_output_unchanged();
    puts("PASS explicit sparse power-series exponents, dense equivalence and failure atomicity");
    return 0;
}
