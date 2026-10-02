#ifndef FOURIER_SPEAKER_INPUT_H
#define FOURIER_SPEAKER_INPUT_H

#include "audio_input.h"
#include <stdbool.h>
#include <stddef.h>

/* Experiment-facing speaker input.
 *
 * The experiment supplies a mono float waveform. The speaker boundary owns no
 * Android types: it simply walks the supplied buffer, optionally loops it, and
 * renders frames in the actual format/channels accepted by audio_output.
 *
 * samples remain owned by the caller and must outlive playback.
 */
struct speaker_input {
    const float *samples;
    size_t frames;
    size_t cursor;
    bool loop;
};

bool speaker_input_set(struct speaker_input *input, const float *samples,
                       size_t frames, bool loop);
void speaker_input_reset(struct speaker_input *input);
bool speaker_input_finished(const struct speaker_input *input);

/* Render at most capacity output frames. Mono input is duplicated to every
 * output channel. Float samples outside [-1,1] are clipped for the hardware
 * sink. produced is a frame count.
 */
bool speaker_input_render(struct speaker_input *input,
                          struct audio_properties output,
                          void *pcm, size_t capacity, size_t *produced);

#endif
