#ifndef FOURIER_PCM_BLOCK_H
#define FOURIER_PCM_BLOCK_H
#include "audio_input.h"
/* Mathematical code receives ordinary mono float samples at the actual source
 * rate. Conversion runs on the consumer thread. No FFT or window policy here.
 * Returns false on invalid properties, insufficient space, or nonfinite PCM. */
#include <stdbool.h>
bool fourier_pcm_mono(struct audio_properties properties, const void *pcm,
                      size_t frames, float *mono, size_t capacity);
#endif
