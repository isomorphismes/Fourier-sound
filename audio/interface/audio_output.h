#ifndef FOURIER_AUDIO_OUTPUT_H
#define FOURIER_AUDIO_OUTPUT_H

#include "audio_input.h"
#include <stddef.h>

typedef struct audio_output audio_output;

/* Blocking owner-thread sink. requested describes the desired PCM stream;
 * properties() reports what the platform actually opened. PCM is interleaved
 * native-endian, one sample per channel per frame. Open does not start output.
 */
enum audio_result audio_output_open(audio_output **out,
                                    struct audio_properties requested,
                                    struct audio_error *error);
struct audio_properties audio_output_properties(const audio_output *output);
enum audio_result audio_output_start(audio_output *output);

/* Writes at most frames. timeout_ms is the maximum time this call may block.
 * written is a FRAME count, not a sample or byte count. Partial writes are
 * reported to the caller rather than hidden in the backend.
 */
enum audio_result audio_output_write(audio_output *output, const void *pcm,
                                     size_t frames, int timeout_ms,
                                     size_t *written);
struct audio_error audio_output_error(const audio_output *output);
enum audio_result audio_output_stop(audio_output *output);
enum audio_result audio_output_close(audio_output **output);

#endif
