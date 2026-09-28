#ifndef FOURIER_MICROPHONE_CHECK_H
#define FOURIER_MICROPHONE_CHECK_H
#include "pcm_block.h"
struct microphone_check {
    struct audio_properties properties;
    uint64_t frames, samples, nonzero, mono_samples;
    double minimum, maximum, mean, sum_squares;
    float last_mono;
};
void microphone_check_init(struct microphone_check *check, struct audio_properties properties);
bool microphone_check_accept(struct microphone_check *check, const void *pcm, size_t frames);
double microphone_check_rms(const struct microphone_check *check);
#endif
