#include "fft.h"
#include "framing.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool near(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

static void copy_and_overlap(void)
{
    float source[] = {0, 1, 2, 3, 4, 5, 6, 7};
    float original[8];
    memcpy(original, source, sizeof(source));

    float first[4];
    float second[4];
    assert(fourier_frame_copy(source, 8U, 1U, 4U, first, 4U));
    assert(fourier_frame_copy(source, 8U, 3U, 4U, second, 4U));

    float expected_first[] = {1, 2, 3, 4};
    float expected_second[] = {3, 4, 5, 6};
    assert(memcmp(first, expected_first, sizeof(first)) == 0);
    assert(memcmp(second, expected_second, sizeof(second)) == 0);
    assert(first[2] == second[0] && first[3] == second[1]);
    assert(memcmp(source, original, sizeof(source)) == 0);
}

static void mean_gain_and_rms(void)
{
    float frame[] = {1, 2, 3, 4};
    assert(fourier_frame_remove_mean(frame, 4U));
    assert(near(frame[0], -1.5, 1e-7));
    assert(near(frame[1], -0.5, 1e-7));
    assert(near(frame[2], 0.5, 1e-7));
    assert(near(frame[3], 1.5, 1e-7));

    double rms = 0.0;
    assert(fourier_frame_rms(frame, 4U, &rms));
    assert(near(rms, sqrt(1.25), 1e-12));

    assert(fourier_frame_scale(frame, 4U, 2.0));
    assert(near(frame[0], -3.0, 1e-7));
    assert(near(frame[3], 3.0, 1e-7));
    assert(fourier_frame_rms(frame, 4U, &rms));
    assert(near(rms, 2.0 * sqrt(1.25), 1e-12));
}

static void transactional_failures(void)
{
    float scale_frame[] = {FLT_MAX, 1.0f, -2.0f};
    float scale_original[3];
    memcpy(scale_original, scale_frame, sizeof(scale_frame));
    assert(!fourier_frame_scale(scale_frame, 3U, 2.0));
    assert(memcmp(scale_frame, scale_original, sizeof(scale_frame)) == 0);

    float mean_frame[] = {FLT_MAX, -FLT_MAX, -FLT_MAX};
    float mean_original[3];
    memcpy(mean_original, mean_frame, sizeof(mean_frame));
    assert(!fourier_frame_remove_mean(mean_frame, 3U));
    assert(memcmp(mean_frame, mean_original, sizeof(mean_frame)) == 0);
}

static void hann_window(void)
{
    float frame[5] = {1, 1, 1, 1, 1};
    assert(fourier_frame_apply_hann(frame, 5U));
    assert(near(frame[0], 0.0, 1e-7));
    assert(near(frame[1], 0.5, 1e-7));
    assert(near(frame[2], 1.0, 1e-7));
    assert(near(frame[3], 0.5, 1e-7));
    assert(near(frame[4], 0.0, 1e-7));

    float singleton[] = {0.75f};
    assert(fourier_frame_apply_hann(singleton, 1U));
    assert(singleton[0] == 0.75f);
}

static void frame_then_fft(void)
{
    const double tau = 6.283185307179586476925286766559;
    float stream[96];
    for (size_t n = 0U; n < 96U; ++n) {
        double t = (double)n / 64.0;
        stream[n] = (float)(2.5 + 0.8 * cos(tau * 8.0 * t));
    }

    float frame[64];
    struct complex_value coefficients[64];
    assert(fourier_frame_copy(stream, 96U, 16U, 64U, frame, 64U));
    assert(fourier_frame_remove_mean(frame, 64U));
    assert(fourier_fft_real_radix2(frame, 64U, coefficients, 64U));

    assert(hypot(coefficients[0].real, coefficients[0].imaginary) < 1e-7);
    assert(near(coefficients[8].real, 0.4, 1e-6));
    assert(fabs(coefficients[8].imaginary) < 1e-6);
}

static void rejected_inputs(void)
{
    float source[4] = {0, 1, 2, 3};
    float frame[4] = {0};
    double rms = 0.0;

    assert(!fourier_frame_copy(NULL, 4U, 0U, 4U, frame, 4U));
    assert(!fourier_frame_copy(source, 4U, 0U, 0U, frame, 4U));
    assert(!fourier_frame_copy(source, 4U, 2U, 3U, frame, 4U));
    assert(!fourier_frame_copy(source, 4U, 0U, 4U, frame, 3U));
    assert(!fourier_frame_remove_mean(NULL, 4U));
    assert(!fourier_frame_scale(frame, 4U, NAN));
    assert(!fourier_frame_rms(frame, 0U, &rms));
    assert(!fourier_frame_rms(frame, 4U, NULL));

    source[2] = NAN;
    assert(!fourier_frame_copy(source, 4U, 0U, 4U, frame, 4U));
}

int main(void)
{
    copy_and_overlap();
    mean_gain_and_rms();
    transactional_failures();
    hann_window();
    frame_then_fft();
    rejected_inputs();
    puts("PASS framing copy/overlap, transactional mean/gain, RMS, Hann and FFT composition");
    return 0;
}
