#include "speaker_input.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

bool speaker_input_set(struct speaker_input *input, const float *samples,
                       size_t frames, bool loop)
{
    if (!input || !samples || !frames) return false;
    for (size_t i = 0; i < frames; ++i)
        if (!isfinite(samples[i])) return false;

    *input = (struct speaker_input){
        .samples = samples,
        .frames = frames,
        .cursor = 0,
        .loop = loop
    };
    return true;
}

void speaker_input_reset(struct speaker_input *input)
{
    if (input) input->cursor = 0;
}

bool speaker_input_finished(const struct speaker_input *input)
{
    return !input || !input->frames ||
           (!input->loop && input->cursor >= input->frames);
}

static float clip(float value)
{
    if (value > 1.0f) return 1.0f;
    if (value < -1.0f) return -1.0f;
    return value;
}

static int16_t signed16(float value)
{
    value = clip(value);
    if (value >= 1.0f) return INT16_MAX;
    if (value <= -1.0f) return INT16_MIN;
    return (int16_t)lrintf(value * 32767.0f);
}

bool speaker_input_render(struct speaker_input *input,
                          struct audio_properties output,
                          void *pcm, size_t capacity, size_t *produced)
{
    if (produced) *produced = 0;
    if (!input || !pcm || !produced || !output.sample_rate ||
        !output.channels || output.channels > 8 ||
        (output.format != AUDIO_FLOAT32 && output.format != AUDIO_SIGNED16))
        return false;
    if (!capacity || speaker_input_finished(input)) return true;

    unsigned char *bytes = pcm;
    size_t frames = 0;
    while (frames < capacity) {
        if (input->cursor >= input->frames) {
            if (!input->loop) break;
            input->cursor = 0;
        }

        float value = clip(input->samples[input->cursor++]);
        for (uint32_t channel = 0; channel < output.channels; ++channel) {
            size_t index = frames * output.channels + channel;
            if (output.format == AUDIO_FLOAT32) {
                memcpy(bytes + index * sizeof(float), &value, sizeof(value));
            } else {
                int16_t sample = signed16(value);
                memcpy(bytes + index * sizeof(sample), &sample, sizeof(sample));
            }
        }
        ++frames;
    }

    *produced = frames;
    return true;
}
