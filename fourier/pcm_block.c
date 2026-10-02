#include "pcm_block.h"
#include <math.h>
#include <string.h>
bool fourier_pcm_mono(struct audio_properties p, const void *pcm,
                      size_t frames, float *mono, size_t capacity)
{
    if (!pcm || !mono || !p.sample_rate || !p.channels || p.channels > 8 ||
        frames > capacity || frames > SIZE_MAX / p.channels / sizeof(float) ||
        (p.format != AUDIO_FLOAT32 && p.format != AUDIO_SIGNED16)) return false;
    const unsigned char *bytes = pcm;
    for (size_t frame = 0; frame < frames; ++frame) {
        double sum = 0;
        for (uint32_t channel = 0; channel < p.channels; ++channel) {
            size_t index = frame * p.channels + channel;
            float value;
            if (p.format == AUDIO_FLOAT32) {
                memcpy(&value, bytes + index * sizeof(float), sizeof(float));
            } else {
                int16_t signed_sample;
                memcpy(&signed_sample, bytes + index * sizeof(int16_t), sizeof(int16_t));
                value = (float)signed_sample / 32768.0f;
            }
            if (!isfinite(value)) return false;
            sum += value;
        }
        mono[frame] = (float)(sum / p.channels);
    }
    return true;
}
