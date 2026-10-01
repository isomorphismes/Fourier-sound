#ifndef FOURIER_CYLINDER_STATIC_H
#define FOURIER_CYLINDER_STATIC_H

#include "complex_value.h"
#include "rgb24.h"

#include <stdbool.h>
#include <stddef.h>

bool fourier_noise_power_from_quiet_frames(
    const float *samples, size_t sample_count,
    size_t frame_count, size_t quiet_frame_count,
    double *noise_power, size_t noise_capacity);

double fourier_noise_candidate_weight(
    struct complex_value coefficient, double noise_power);

bool fourier_noise_candidate_coefficients(
    const struct complex_value *coefficients,
    const double *noise_power, size_t count,
    struct complex_value *candidates, size_t candidate_capacity,
    double *weights, size_t weight_capacity);

struct rgb24 rgb24_overlay(struct rgb24 base, struct rgb24 overlay,
                           double alpha);

#endif
