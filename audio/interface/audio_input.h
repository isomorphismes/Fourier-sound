#ifndef FOURIER_AUDIO_INPUT_H
#define FOURIER_AUDIO_INPUT_H
#include <stddef.h>
#include <stdint.h>

typedef struct audio_input audio_input;
enum audio_sample_format { AUDIO_FLOAT32 = 1, AUDIO_SIGNED16 = 2 };
enum audio_result {
    AUDIO_OK, AUDIO_TIMEOUT, AUDIO_INVALID, AUDIO_NO_MEMORY,
    AUDIO_PERMISSION, AUDIO_UNAVAILABLE, AUDIO_UNSUPPORTED,
    AUDIO_DISCONNECTED, AUDIO_SYSTEM_ERROR
};
struct audio_properties {
    uint32_t sample_rate;
    uint32_t channels;
    enum audio_sample_format format;
};
struct audio_error { enum audio_result code; int32_t native_code; };

/* One application thread owns every operation, including close. Backend-owned
 * acquisition may run concurrently. Returned samples are interleaved native-
 * endian PCM; a frame contains one sample per channel. No application callback
 * ever runs on an acquisition thread. Open does not start recording.
 * The first backend requests mono, 48000 Hz, float32; properties are ACTUAL.
 */
enum audio_result audio_input_open(audio_input **out, struct audio_error *error);
struct audio_properties audio_input_properties(const audio_input *input);
enum audio_result audio_input_start(audio_input *input);
/* Nonblocking; capacity and received are FRAME counts. Copy owns no memory. */
enum audio_result audio_input_read(audio_input *input, void *pcm,
                                  size_t capacity, size_t *received);
/* Readiness wait, not a periodic acquisition poll. timeout_ms must be >= 0. */
enum audio_result audio_input_wait(audio_input *input, int timeout_ms);
/* Overflow drops incoming frames, never overwrites unread PCM. Saturates at
 * UINT32_MAX. Treat any increase as a discontinuity before Fourier analysis. */
uint32_t audio_input_dropped_frames(const audio_input *input);
struct audio_error audio_input_error(const audio_input *input);
enum audio_result audio_input_stop(audio_input *input);
/* Stop completes before reuse; restart discards old PCM and drop counts.
 * Close stops acquisition and joins platform callbacks before releasing memory.
 * It accepts an already-NULL handle. A failed close retains the handle. */
enum audio_result audio_input_close(audio_input **input);
const char *audio_result_text(enum audio_result result);
#endif
