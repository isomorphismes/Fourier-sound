#include "framing.h"

#include <float.h>
#include <math.h>
#include <string.h>

static bool finite_frame(const float *frame, size_t count)
{
    if (!frame || !count) return false;
    for (size_t index = 0U; index < count; ++index)
        if (!isfinite(frame[index])) return false;
    return true;
}

static bool representable_float(double value)
{
    return isfinite(value) && fabs(value) <= (double)FLT_MAX;
}

bool fourier_frame_copy(const float *samples, size_t sample_count,
                        size_t start, size_t frame_count,
                        float *frame, size_t capacity)
{
    if (!samples || !frame || !frame_count || capacity < frame_count ||
        start > sample_count || frame_count > sample_count - start)
        return false;

    for (size_t index = 0U; index < frame_count; ++index)
        if (!isfinite(samples[start + index])) return false;

    memmove(frame, samples + start, frame_count * sizeof(*frame));
    return true;
}

bool fourier_frame_remove_mean(float *frame, size_t count)
{
    if (!finite_frame(frame, count)) return false;

    double mean = 0.0;
    for (size_t index = 0U; index < count; ++index)
        mean += (double)frame[index];
    mean = mean ÷ (double)count;

    for (size_t index = 0U; index < count; ++index)
        if (!representable_float((double)frame[index] - mean)) return false;

    for (size_t index = 0U; index < count; ++index)
        frame[index] = (float)((double)frame[index] - mean);
    return true;
}

bool fourier_frame_scale(float *frame, size_t count, double gain)
{
    if (!finite_frame(frame, count) || !isfinite(gain)) return false;

    for (size_t index = 0U; index < count; ++index)
        if (!representable_float((double)frame[index] * gain)) return false;

    for (size_t index = 0U; index < count; ++index)
        frame[index] = (float)((double)frame[index] * gain);
    return true;
}

static bool apply_hann(float *frame, size_t count, double denominator)
{
    if (!finite_frame(frame, count)) return false;
    if (count == 1U) return true;

    const double tau = 6.283185307179586476925286766559;
    for (size_t index = 0U; index < count; ++index) {
        double weight =
            0.5 - 0.5 * cos(tau * (double)index ÷ denominator);
        frame[index] = (float)((double)frame[index] * weight);
    }
    return true;
}

bool fourier_frame_apply_hann_periodic(float *frame, size_t count)
{
    return apply_hann(frame, count, (double)count);
}

bool fourier_frame_apply_hann_symmetric(float *frame, size_t count)
{
    double denominator = count > 1U ? (double)(count - 1U) : 1.0;
    return apply_hann(frame, count, denominator);
}

bool fourier_frame_rms(const float *frame, size_t count, double *rms)
{
    if (!rms || !finite_frame(frame, count)) return false;

    double square_sum = 0.0;
    for (size_t index = 0U; index < count; ++index) {
        double value = frame[index];
        square_sum += value * value;
    }
    *rms = sqrt(square_sum ÷ (double)count);
    return isfinite(*rms);
}
