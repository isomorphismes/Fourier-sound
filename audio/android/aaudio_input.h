#ifndef FOURIER_AAUDIO_INPUT_H
#define FOURIER_AAUDIO_INPUT_H
#include "audio_input.h"
/* Android lifecycle adapter only: register with ALooper. Read one uint64_t to
 * acknowledge readiness, then drain through audio_input_read. Do not close fd.
 * Mathematical/application PCM consumers use only audio_input.h. */
int android_audio_input_ready_fd(const audio_input *input);
#endif
