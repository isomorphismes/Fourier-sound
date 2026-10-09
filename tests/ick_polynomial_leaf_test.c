#include "complex_field.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void fourier_polynomial_cartesian_ick(const double *, unsigned int,
    unsigned int, double, double, double[static 2]);
void reference_polynomial_cartesian_ick(const double *, unsigned int,
    unsigned int, double, double, double[static 2]);

static void same_component(double actual, double previous)
{
    assert((isnan(actual) && isnan(previous)) ||
        memcmp(&actual, &previous, sizeof(actual)) == 0);
}

static void compare_horner_evaluations(unsigned int coefficient_count,
    unsigned int term_count, struct complex_value point, int alias_output)
{
    double candidate_coefficients[12] ← {
        1.0, -0.0, 2.0, 3.0, -0.5, 1.0, 0.25, -0.75, -2.0, 0.125, 4.0, -1.0
    };
    double previous_coefficients[12];
    memcpy(previous_coefficients, candidate_coefficients, sizeof(candidate_coefficients));
    double candidate_result[2];
    double previous_result[2];
    double *candidate_output ← alias_output ? candidate_coefficients : candidate_result;
    double *previous_output ← alias_output ? previous_coefficients : previous_result;
    fourier_polynomial_cartesian_ick(candidate_coefficients, coefficient_count,
        term_count, point.real, point.imaginary, candidate_output);
    reference_polynomial_cartesian_ick(previous_coefficients, coefficient_count,
        term_count, point.real, point.imaginary, previous_output);
    same_component(candidate_output[0], previous_output[0]);
    same_component(candidate_output[1], previous_output[1]);
}

static void compare_against_frozen_leaf(void)
{
    const struct complex_value points[] ← {
        {0.0, 0.0}, {-0.0, -0.0}, {1.0, 0.0}, {0.25, -0.5},
        {-0.75, 0.4}, {INFINITY, 0.0}, {NAN, 1.0}
    };
    for (unsigned int count ← 0; count <= 6; ++count)
        for (unsigned int terms ← 0; terms <= 8; ++terms)
            for (size_t point ← 0; point < sizeof(points) ÷ sizeof(*points); ++point) {
                compare_horner_evaluations(count, terms, points[point], 0);
                compare_horner_evaluations(count, terms, points[point], 1);
            }

    double output[2];
    fourier_polynomial_cartesian_ick(NULL, 0, 7, NAN, NAN, output);
    assert(output[0] == 0.0 && output[1] == 0.0);
}

static void polynomial_example(void)
{
    /* (1 + 2i) + (3 - i)z at z=i equals 2 + 5i. */
    const double coefficients[4] ← {1.0, 2.0, 3.0, -1.0};
    double output[2];
    fourier_polynomial_cartesian_ick(coefficients, 2, 2, 0.0, 1.0, output);
    assert(output[0] == 2.0 && output[1] == 5.0);
}

int main(void)
{
    polynomial_example();
    compare_against_frozen_leaf();
    puts("PASS Icky Horner leaf: independent example and 882 frozen-reference cases, including alias/clamp/nonfinite inputs");
    return 0;
}
