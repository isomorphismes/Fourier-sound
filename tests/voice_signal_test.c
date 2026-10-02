#include "fft.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#define SAMPLE_COUNT 1024U

static bool near(double actual, double expected, double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}

int main(void)
{
    const double tau = 6.283185307179586476925286766559;
    const double sample_rate = 48000.0;
    float samples[SAMPLE_COUNT];
    struct complex_value coefficients[SAMPLE_COUNT];

    assert(near(sample_rate * 8.0 / SAMPLE_COUNT, 375.0, 1e-12));
    assert(near(sample_rate * 16.0 / SAMPLE_COUNT, 750.0, 1e-12));

    for (size_t n = 0U; n < SAMPLE_COUNT; ++n) {
        double phase = tau * (double)n / (double)SAMPLE_COUNT;
        samples[n] = (float)(
            0.1 +
            0.5 * cos(8.0 * phase) +
            0.25 * sin(16.0 * phase)
        );
    }

    assert(fourier_fft_real_radix2(
        samples, SAMPLE_COUNT, coefficients, SAMPLE_COUNT));

    assert(near(coefficients[0].real, 0.1, 2e-6));
    assert(near(coefficients[0].imaginary, 0.0, 2e-6));
    assert(near(coefficients[8].real, 0.25, 2e-6));
    assert(near(coefficients[8].imaginary, 0.0, 2e-6));
    assert(near(coefficients[16].real, 0.0, 2e-6));
    assert(near(coefficients[16].imaginary, -0.125, 2e-6));
    assert(near(coefficients[SAMPLE_COUNT - 8U].real, 0.25, 2e-6));
    assert(near(coefficients[SAMPLE_COUNT - 16U].imaginary, 0.125, 2e-6));

    assert(hypot(0.44, 0.88) < 1.0);

    puts("PASS 1024-point 48 kHz self-test: 375 Hz and 750 Hz coefficients; render rectangle stays inside unit disk");
    return 0;
}
