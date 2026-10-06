#ifndef FOURIER_FRAMING_H
#define FOURIER_FRAMING_H

#include <stdbool.h>
#include <stddef.h>

bool fourier_frame_copy(const float *samples, size_t sample_count,
                        size_t start, size_t frame_count,
                        float *frame, size_t capacity);

bool fourier_frame_remove_mean(float *frame, size_t count);
bool fourier_frame_scale(float *frame, size_t count, double gain);
bool fourier_frame_rms(const float *frame, size_t count, double *rms);

/* Periodic Hann: 0.5 - 0.5*cos(2*pi*n/N), for a frame treated as one
 * period by a DFT/FFT. Symmetric Hann uses N-1 and has zero endpoints.
 * A singleton frame is left unchanged by either convention. */
bool fourier_frame_apply_hann_periodic(float *frame, size_t count);
bool fourier_frame_apply_hann_symmetric(float *frame, size_t count);

#endif
