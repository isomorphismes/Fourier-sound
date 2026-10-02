#include "fft.h"
#include "framing.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

int main(void)
{
    float source[] = {0, 1, 2, 3, 4, 5, 6, 7};
    float frame[4];
    assert(fourier_frame_copy(source, 8U, 2U, 4U, frame, 4U));
    float expected[] = {2, 3, 4, 5};
    assert(memcmp(frame, expected, sizeof(frame)) == 0);

    assert(fourier_frame_remove_mean(frame, 4U));
    double rms = 0.0;
    assert(fourier_frame_rms(frame, 4U, &rms));
    assert(near(rms, sqrt(1.25), 1e-12));

    float periodic[4] = {1, 1, 1, 1};
    assert(fourier_frame_apply_hann_periodic(periodic, 4U));
    assert(near(periodic[0], 0.0, 1e-7));
    assert(near(periodic[1], 0.5, 1e-7));
    assert(near(periodic[2], 1.0, 1e-7));
    assert(near(periodic[3], 0.5, 1e-7));

    const double tau = 6.283185307179586476925286766559;
    float tone[64];
    struct complex_value coefficients[64];
    for (size_t n = 0U; n < 64U; ++n)
        tone[n] = (float)(2.5 + 0.8 * cos(tau * 8.0 * (double)n / 64.0));
    assert(fourier_frame_remove_mean(tone, 64U));
    assert(fourier_fft_real_radix2(tone, 64U, coefficients, 64U));
    assert(near(coefficients[8].real, 0.4, 1e-6));
    assert(fabs(coefficients[8].imaginary) < 1e-6);

    puts("PASS framing copy, mean, RMS, Hann and FFT composition");
    return 0;
}
