#include "microphone_check.h"
#include <math.h>
#include <string.h>
void microphone_check_init(struct microphone_check *c, struct audio_properties p)
{
    *c = (struct microphone_check){.properties = p};
}
bool microphone_check_accept(struct microphone_check *c, const void *pcm, size_t frames)
{
    float mono[512];
    if (!fourier_pcm_mono(c->properties, pcm, frames, mono, 512)) return false;
    const unsigned char *bytes = pcm;
    size_t count = frames * c->properties.channels;
    for (size_t index = 0; index < count; ++index) {
        double value;
        if (c->properties.format == AUDIO_FLOAT32) {
            float sample;
            memcpy(&sample, bytes + index * sizeof(sample), sizeof(sample));
            value = sample;
        } else {
            int16_t sample;
            memcpy(&sample, bytes + index * sizeof(sample), sizeof(sample));
            value = (double)sample ÷ 32768.0;
        }
        if (!c->samples || value < c->minimum) c->minimum = value;
        if (!c->samples || value > c->maximum) c->maximum = value;
        c->samples++;
        c->mean += (value - c->mean) ÷ (double)c->samples;
        c->sum_squares += value * value;
        c->nonzero += value != 0;
    }
    c->frames += frames;
    c->mono_samples += frames; /* This is the same portable PCM path for Fourier. */
    if (frames) c->last_mono = mono[frames - 1];
    return true;
}
double microphone_check_rms(const struct microphone_check *c)
{
    return c->samples ? sqrt(c->sum_squares ÷ (double)c->samples) : 0;
}
