#include "cylinder_static.h"

#include "fft.h"
#include "framing.h"

#include <math.h>
#include <stdlib.h>

struct frame_score {
    size_t offset;
    double rms;
};

static int compare_frame_score(const void *left, const void *right)
{
    const struct frame_score *a = left;
    const struct frame_score *b = right;
    if (a->rms < b->rms) return -1;
    if (a->rms > b->rms) return 1;
    return 0;
}

bool fourier_noise_power_from_quiet_frames(
    const float *samples, size_t sample_count,
    size_t frame_count, size_t quiet_frame_count,
    double *noise_power, size_t noise_capacity)
{
    if (!samples || !noise_power || !frame_count || !quiet_frame_count ||
        noise_capacity < frame_count || sample_count < frame_count)
        return false;

    size_t frame_total = sample_count / frame_count;
    if (!frame_total) return false;
    if (quiet_frame_count > frame_total) quiet_frame_count = frame_total;

    struct frame_score *scores =
        malloc(frame_total * sizeof(*scores));
    float *frame = malloc(frame_count * sizeof(*frame));
    struct complex_value *coefficients =
        malloc(frame_count * sizeof(*coefficients));
    if (!scores || !frame || !coefficients) {
        free(coefficients);
        free(frame);
        free(scores);
        return false;
    }

    for (size_t index = 0U; index < frame_total; ++index) {
        size_t offset = index * frame_count;
        double rms = 0.0;
        if (!fourier_frame_copy(samples, sample_count, offset,
                                frame_count, frame, frame_count) ||
            !fourier_frame_rms(frame, frame_count, &rms)) {
            free(coefficients);
            free(frame);
            free(scores);
            return false;
        }
        scores[index] = (struct frame_score){offset, rms};
    }

    qsort(scores, frame_total, sizeof(*scores), compare_frame_score);
    for (size_t k = 0U; k < frame_count; ++k) noise_power[k] = 0.0;

    for (size_t index = 0U; index < quiet_frame_count; ++index) {
        if (!fourier_frame_copy(samples, sample_count, scores[index].offset,
                                frame_count, frame, frame_count) ||
            !fourier_frame_remove_mean(frame, frame_count) ||
            !fourier_frame_apply_hann_periodic(frame, frame_count) ||
            !fourier_fft_real_radix2(frame, frame_count,
                                     coefficients, frame_count)) {
            free(coefficients);
            free(frame);
            free(scores);
            return false;
        }

        for (size_t k = 0U; k < frame_count; ++k) {
            double real = coefficients[k].real;
            double imaginary = coefficients[k].imaginary;
            noise_power[k] += real * real + imaginary * imaginary;
        }
    }

    double scale = 1.0 / (double)quiet_frame_count;
    for (size_t k = 0U; k < frame_count; ++k)
        noise_power[k] *= scale;

    free(coefficients);
    free(frame);
    free(scores);
    return true;
}

double fourier_noise_candidate_weight(
    struct complex_value coefficient, double noise_power)
{
    if (!isfinite(coefficient.real) || !isfinite(coefficient.imaginary) ||
        !isfinite(noise_power) || noise_power < 0.0)
        return 0.0;

    double signal_amplitude = hypot(coefficient.real,
                                    coefficient.imaginary);
    double noise_amplitude = sqrt(noise_power);
    if (noise_amplitude == 0.0) return 0.0;
    if (signal_amplitude <= noise_amplitude) return 1.0;

    double weight = noise_amplitude / signal_amplitude;
    if (weight < 0.0) return 0.0;
    if (weight > 1.0) return 1.0;
    return weight;
}

bool fourier_noise_candidate_coefficients(
    const struct complex_value *coefficients,
    const double *noise_power, size_t count,
    struct complex_value *candidates, size_t candidate_capacity,
    double *weights, size_t weight_capacity)
{
    if (!coefficients || !noise_power || !candidates ||
        candidate_capacity < count ||
        (weights && weight_capacity < count))
        return false;

    for (size_t k = 0U; k < count; ++k) {
        double weight =
            fourier_noise_candidate_weight(coefficients[k], noise_power[k]);
        candidates[k].real = coefficients[k].real * weight;
        candidates[k].imaginary = coefficients[k].imaginary * weight;
        if (weights) weights[k] = weight;
    }
    return true;
}

static unsigned char blend_byte(unsigned char base, unsigned char overlay,
                                double alpha)
{
    if (!isfinite(alpha) || alpha <= 0.0) return base;
    if (alpha >= 1.0) return overlay;
    double value = (1.0 - alpha) * (double)base +
                   alpha * (double)overlay;
    if (value < 0.0) value = 0.0;
    if (value > 255.0) value = 255.0;
    return (unsigned char)lrint(value);
}

struct rgb24 rgb24_overlay(struct rgb24 base, struct rgb24 overlay,
                           double alpha)
{
    return (struct rgb24){
        blend_byte(base.red, overlay.red, alpha),
        blend_byte(base.green, overlay.green, alpha),
        blend_byte(base.blue, overlay.blue, alpha)
    };
}
