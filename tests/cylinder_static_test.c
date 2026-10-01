#include "cylinder_static.h"

#include "fft.h"
#include "framing.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#define N 1024U
#define FRAMES 6U

static void transform(float *frame, struct complex_value *coefficients)
{
    assert(fourier_frame_remove_mean(frame, N));
    assert(fourier_frame_apply_hann_periodic(frame, N));
    assert(fourier_fft_real_radix2(frame, N, coefficients, N));
}

int main(void)
{
    const double tau = 6.283185307179586476925286766559;
    float samples[N * FRAMES];

    for (size_t frame = 0U; frame < FRAMES; ++frame) {
        bool loud = frame == 2U || frame == 3U;
        for (size_t n = 0U; n < N; ++n) {
            double phase = tau * (double)n / (double)N;
            double noise =
                0.020 * sin(37.0 * phase) +
                0.010 * cos(91.0 * phase);
            double voice = loud ? 0.40 * sin(7.0 * phase) : 0.0;
            samples[frame * N + n] = (float)(noise + voice);
        }
    }

    double noise_power[N];
    assert(fourier_noise_power_from_quiet_frames(
        samples, N * FRAMES, N, 2U, noise_power, N));
    assert(noise_power[37] > noise_power[7] * 100.0);
    assert(noise_power[91] > noise_power[7] * 10.0);

    float frame[N];
    assert(fourier_frame_copy(samples, N * FRAMES, 2U * N,
                              N, frame, N));
    struct complex_value coefficients[N];
    transform(frame, coefficients);

    struct complex_value original[N];
    memcpy(original, coefficients, sizeof(original));

    struct complex_value candidates[N];
    double weights[N];
    assert(fourier_noise_candidate_coefficients(
        coefficients, noise_power, N, candidates, N, weights, N));

    assert(memcmp(original, coefficients, sizeof(original)) == 0);
    assert(weights[7] < 0.20);
    assert(weights[37] > 0.70);
    assert(weights[91] > 0.70);

    struct rgb24 base = {200U, 100U, 50U};
    struct rgb24 gray = {40U, 40U, 48U};
    struct rgb24 unchanged = rgb24_overlay(base, gray, 0.0);
    struct rgb24 replaced = rgb24_overlay(base, gray, 1.0);
    struct rgb24 half = rgb24_overlay(base, gray, 0.5);

    assert(unchanged.red == base.red &&
           unchanged.green == base.green &&
           unchanged.blue == base.blue);
    assert(replaced.red == gray.red &&
           replaced.green == gray.green &&
           replaced.blue == gray.blue);
    assert(half.red == 120U);
    assert(half.green == 70U);
    assert(half.blue == 49U);

    return 0;
}
