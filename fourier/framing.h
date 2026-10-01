#ifndef FOURIER_FRAMING_H
#define FOURIER_FRAMING_H

#include <stdbool.h>
#include <stddef.h>

/* Copy one explicit observation from an existing mono sample stream.
 * start + frame_count must lie inside the source; raw samples are never
 * modified. Choosing successive start values is how the caller chooses hop
 * size and overlap. */
bool fourier_frame_copy(const float *samples, size_t sample_count,
                        size_t start, size_t frame_count,
                        float *frame, size_t capacity);

/* Optional framing operations. Each is explicit so experiments can choose
 * rectangular/no-window, mean retention/removal, and gain independently. */
bool fourier_frame_remove_mean(float *frame, size_t count);
bool fourier_frame_scale(float *frame, size_t count, double gain);
bool fourier_frame_apply_hann(float *frame, size_t count);
bool fourier_frame_rms(const float *frame, size_t count, double *rms);

#endif
