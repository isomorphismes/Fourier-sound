#include "dft.h"
#include "fft.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#define COMPARE_MAX 256U
#define REALISTIC_COUNT 4096U

static void compare(const float *samples, size_t count, double tolerance)
{
    struct complex_value direct[COMPARE_MAX];
    struct complex_value fast[COMPARE_MAX];

    assert(count <= COMPARE_MAX);
    assert(fourier_dft_real(samples, count, direct, COMPARE_MAX));
    assert(fourier_fft_real_radix2(samples, count, fast, COMPARE_MAX));

    for (size_t k = 0U; k < count; ++k) {
        assert(fabs(direct[k].real - fast[k].real) <= tolerance);
        assert(fabs(direct[k].imaginary - fast[k].imaginary) <= tolerance);
    }
}

static void deterministic_signals(void)
{
    float one[] = {0.375f};
    compare(one, 1U, 1e-12);

    float impulse[16] = {0};
    impulse[0] = 1.0f;
    compare(impulse, 16U, 1e-12);

    float shifted_impulse[16] = {0};
    shifted_impulse[5] = 1.0f;
    compare(shifted_impulse, 16U, 1e-12);

    float constant[32];
    for (size_t n = 0U; n < 32U; ++n) constant[n] = -0.125f;
    compare(constant, 32U, 1e-12);

    const double tau = 6.283185307179586476925286766559;
    float harmonics[64];
    for (size_t n = 0U; n < 64U; ++n) {
        double t = (double)n / 64.0;
        harmonics[n] = (float)(
            0.15 +
            0.7 * cos(tau * 7.0 * t) -
            0.2 * sin(tau * 11.0 * t) +
            0.05 * cos(tau * 16.0 * t)
        );
    }
    compare(harmonics, 64U, 2e-12);
}

static void pseudo_random_signals(void)
{
    unsigned state = 0x12345678U;
    float samples[COMPARE_MAX];

    for (size_t count = 2U; count <= COMPARE_MAX; count <<= 1U) {
        for (size_t n = 0U; n < count; ++n) {
            state = state * 1664525U + 1013904223U;
            unsigned mantissa = state >> 8U;
            double unit = (double)mantissa / 16777215.0;
            samples[n] = (float)(2.0 * unit - 1.0);
        }
        compare(samples, count, 2e-11);
    }
}

static void realistic_block(void)
{
    static float samples[REALISTIC_COUNT];
    static struct complex_value coefficients[REALISTIC_COUNT];
    const double tau = 6.283185307179586476925286766559;

    for (size_t n = 0U; n < REALISTIC_COUNT; ++n) {
        double t = (double)n / (double)REALISTIC_COUNT;
        samples[n] = (float)(
            0.1 +
            0.7 * cos(tau * 123.0 * t) -
            0.2 * sin(tau * 777.0 * t)
        );
    }

    assert(fourier_fft_real_radix2(samples, REALISTIC_COUNT,
                                   coefficients, REALISTIC_COUNT));

    assert(fabs(coefficients[0].real - 0.1) < 1e-7);
    assert(fabs(coefficients[0].imaginary) < 1e-10);
    assert(fabs(coefficients[123].real - 0.35) < 1e-7);
    assert(fabs(coefficients[123].imaginary) < 1e-7);
    assert(fabs(coefficients[777].real) < 1e-7);
    assert(fabs(coefficients[777].imaginary - 0.1) < 1e-7);

    double sample_energy = 0.0;
    double coefficient_energy = 0.0;
    for (size_t n = 0U; n < REALISTIC_COUNT; ++n)
        sample_energy += (double)samples[n] * (double)samples[n];
    sample_energy /= (double)REALISTIC_COUNT;

    for (size_t k = 0U; k < REALISTIC_COUNT; ++k) {
        coefficient_energy +=
            coefficients[k].real * coefficients[k].real +
            coefficients[k].imaginary * coefficients[k].imaginary;
        size_t mirror = (REALISTIC_COUNT - k) % REALISTIC_COUNT;
        assert(fabs(coefficients[k].real - coefficients[mirror].real) < 2e-9);
        assert(fabs(coefficients[k].imaginary +
                    coefficients[mirror].imaginary) < 2e-9);
    }
    assert(fabs(sample_energy - coefficient_energy) < 2e-9);
}

static void rejected_inputs(void)
{
    struct complex_value output[8];
    float samples[8] = {0};
    assert(!fourier_fft_real_radix2(NULL, 8U, output, 8U));
    assert(!fourier_fft_real_radix2(samples, 0U, output, 8U));
    assert(!fourier_fft_real_radix2(samples, 3U, output, 8U));
    assert(!fourier_fft_real_radix2(samples, 8U, output, 7U));
    samples[3] = NAN;
    assert(!fourier_fft_real_radix2(samples, 8U, output, 8U));
}

int main(void)
{
    deterministic_signals();
    pseudo_random_signals();
    realistic_block();
    rejected_inputs();
    puts("PASS radix-2 FFT matches DFT, harmonics, conjugacy and Parseval");
    return 0;
}
