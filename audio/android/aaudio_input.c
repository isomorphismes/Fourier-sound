#define _POSIX_C_SOURCE 200809L
#include "audio_input.h"
#include "aaudio_input.h"
#include "pcm_ring.h"
#include <aaudio/AAudio.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_FRAMES 32768U
struct audio_input {
    AAudioStream *stream;
    struct audio_properties properties;
    struct pcm_ring ring;
    void *storage;
    int ready;
    bool started;
    _Atomic int native_error;
};
int android_audio_input_ready_fd(const audio_input *s)
{
    return s ? s->ready : -1;
}
static enum audio_result translate(aaudio_result_t result)
{
    switch (result) {
    case AAUDIO_OK: return AUDIO_OK;
    case AAUDIO_ERROR_DISCONNECTED: return AUDIO_DISCONNECTED;
    case AAUDIO_ERROR_NO_MEMORY: return AUDIO_NO_MEMORY;
    case AAUDIO_ERROR_UNAVAILABLE: return AUDIO_UNAVAILABLE;
    case AAUDIO_ERROR_INVALID_FORMAT:
    case AAUDIO_ERROR_INVALID_RATE: return AUDIO_UNSUPPORTED;
    case AAUDIO_ERROR_INVALID_STATE:
    case AAUDIO_ERROR_ILLEGAL_ARGUMENT: return AUDIO_INVALID;
    case AAUDIO_ERROR_TIMEOUT: return AUDIO_TIMEOUT;
    default: return AUDIO_SYSTEM_ERROR;
    }
}
static enum audio_result remember(audio_input *s, aaudio_result_t code)
{
    if (code != AAUDIO_OK) atomic_store_explicit(&s->native_error, code, memory_order_relaxed);
    return translate(code);
}
static void notify(audio_input *s)
{
    const uint64_t one = 1;
    /* Nonblocking eventfd. A full counter is already readable. EINTR retries
     * are deliberately excluded from the real-time callback; the next buffer
     * signals again and waits have a finite deadline. */
    ssize_t signaled = write(s->ready, &one, sizeof(one));
    (void)signaled;
}
static aaudio_data_callback_result_t receive(AAudioStream *stream, void *context,
                                            void *pcm, int32_t frames)
{
    (void)stream;
    audio_input *s = context;
    if (frames > 0) {
        (void)pcm_ring_write(&s->ring, pcm, (size_t)frames);
        notify(s);
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void failed(AAudioStream *stream, void *context, aaudio_result_t error)
{
    (void)stream;
    audio_input *s = context;
    atomic_store_explicit(&s->native_error, error, memory_order_relaxed);
    notify(s); /* Owner thread performs stop/close, never this callback. */
}
struct audio_error audio_input_error(const audio_input *s)
{
    if (!s) return (struct audio_error){AUDIO_INVALID, 0};
    int code = atomic_load_explicit(&s->native_error, memory_order_relaxed);
    return (struct audio_error){translate(code), code};
}
enum audio_result audio_input_open(audio_input **out, struct audio_error *error)
{
    if (error) *error = (struct audio_error){AUDIO_OK, 0};
    if (!out) {
        if (error) error->code = AUDIO_INVALID;
        return AUDIO_INVALID;
    }
    *out = NULL;
    audio_input *s = calloc(1, sizeof(*s));
    enum audio_result result = AUDIO_NO_MEMORY;
    aaudio_result_t native = AAUDIO_OK;
    AAudioStreamBuilder *builder = NULL;
    if (!s) goto failure;
    s->ready = -1;
    atomic_init(&s->native_error, AAUDIO_OK);
    if (!atomic_is_lock_free(&s->native_error)) { result = AUDIO_UNSUPPORTED; goto failure; }
    s->ready = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (s->ready < 0) { result = AUDIO_SYSTEM_ERROR; goto failure; }
    native = AAudio_createStreamBuilder(&builder);
    if (native != AAUDIO_OK) { result = translate(native); goto failure; }
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, 48000);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setDataCallback(builder, receive, s);
    AAudioStreamBuilder_setErrorCallback(builder, failed, s);
    native = AAudioStreamBuilder_openStream(builder, &s->stream);
    /* AAudio normally converts the requested rate. If this route cannot serve
     * it, permit a native-rate stream. Never retry permission/system failures. */
    if (native == AAUDIO_ERROR_INVALID_RATE || native == AAUDIO_ERROR_UNAVAILABLE) {
        AAudioStreamBuilder_setSampleRate(builder, AAUDIO_UNSPECIFIED);
        native = AAudioStreamBuilder_openStream(builder, &s->stream);
    }
    if (native == AAUDIO_ERROR_INVALID_FORMAT) {
        AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
        AAudioStreamBuilder_setSampleRate(builder, AAUDIO_UNSPECIFIED);
        native = AAudioStreamBuilder_openStream(builder, &s->stream);
    }
    AAudioStreamBuilder_delete(builder); builder = NULL;
    if (native != AAUDIO_OK) { result = translate(native); goto failure; }
    int32_t rate = AAudioStream_getSampleRate(s->stream);
    int32_t channels = AAudioStream_getChannelCount(s->stream);
    aaudio_format_t format = AAudioStream_getFormat(s->stream);
    if (rate <= 0 || channels < 1 || channels > 8 ||
        (format != AAUDIO_FORMAT_PCM_FLOAT && format != AAUDIO_FORMAT_PCM_I16)) {
        result = AUDIO_UNSUPPORTED; goto failure;
    }
    s->properties = (struct audio_properties){(uint32_t)rate, (uint32_t)channels,
        format == AAUDIO_FORMAT_PCM_FLOAT ? AUDIO_FLOAT32 : AUDIO_SIGNED16};
    size_t bytes = (size_t)channels * (format == AAUDIO_FORMAT_PCM_FLOAT ? sizeof(float) : sizeof(int16_t));
    s->storage = calloc(BUFFER_FRAMES, bytes);
    if (!s->storage) { result = AUDIO_NO_MEMORY; goto failure; }
    if (!pcm_ring_init(&s->ring, s->storage, BUFFER_FRAMES, bytes)) {
        result = AUDIO_UNSUPPORTED; goto failure;
    }
    *out = s;
    return AUDIO_OK;
failure:
    if (builder) AAudioStreamBuilder_delete(builder);
    if (s) {
        if (s->stream) AAudioStream_close(s->stream);
        if (s->ready >= 0) close(s->ready);
        free(s->storage); free(s);
    }
    if (error) *error = (struct audio_error){result, native};
    return result;
}
struct audio_properties audio_input_properties(const audio_input *s)
{
    return s ? s->properties : (struct audio_properties){0, 0, 0};
}
enum audio_result audio_input_start(audio_input *s)
{
    if (!s) return AUDIO_INVALID;
    enum audio_result error = audio_input_error(s).code;
    if (error != AUDIO_OK) return error;
    if (s->started) return AUDIO_OK;
    pcm_ring_reset(&s->ring);
    uint64_t pending;
    ssize_t drained = read(s->ready, &pending, sizeof(pending));
    (void)drained;
    aaudio_result_t result = AAudioStream_requestStart(s->stream);
    if (result == AAUDIO_OK) s->started = true;
    return remember(s, result);
}
enum audio_result audio_input_read(audio_input *s, void *pcm, size_t capacity, size_t *received)
{
    if (received) *received = 0;
    if (!s || !pcm || !received) return AUDIO_INVALID;
    enum audio_result error = audio_input_error(s).code;
    if (error != AUDIO_OK) return error;
    *received = pcm_ring_read(&s->ring, pcm, capacity);
    return AUDIO_OK;
}
enum audio_result audio_input_wait(audio_input *s, int timeout_ms)
{
    if (!s || timeout_ms < 0 || !s->started) return AUDIO_INVALID;
    enum audio_result error = audio_input_error(s).code;
    if (error != AUDIO_OK) return error;
    if (pcm_ring_available(&s->ring)) return AUDIO_OK;
    struct pollfd descriptor = {s->ready, POLLIN, 0};
    int result = poll(&descriptor, 1, timeout_ms);
    if (result < 0) return errno == EINTR ? AUDIO_TIMEOUT : AUDIO_SYSTEM_ERROR;
    if (!result) return AUDIO_TIMEOUT;
    if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) return AUDIO_SYSTEM_ERROR;
    uint64_t pending;
    ssize_t drained = read(s->ready, &pending, sizeof(pending));
    (void)drained;
    return audio_input_error(s).code;
}
uint32_t audio_input_dropped_frames(const audio_input *s)
{
    return s ? atomic_load_explicit(&s->ring.dropped, memory_order_relaxed) : 0;
}
static int64_t now_ns(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (int64_t)value.tv_sec * 1000000000 + value.tv_nsec;
}
enum audio_result audio_input_stop(audio_input *s)
{
    if (!s) return AUDIO_INVALID;
    if (!s->started) return AUDIO_OK;
    aaudio_result_t result = AAudioStream_requestStop(s->stream);
    if (result != AAUDIO_OK) return remember(s, result);
    int64_t deadline = now_ns() + 2000000000;
    aaudio_stream_state_t state = AAudioStream_getState(s->stream);
    while (state != AAUDIO_STREAM_STATE_STOPPED) {
        if (state == AAUDIO_STREAM_STATE_DISCONNECTED) return remember(s, AAUDIO_ERROR_DISCONNECTED);
        int64_t remaining = deadline - now_ns();
        if (remaining <= 0) return remember(s, AAUDIO_ERROR_TIMEOUT);
        aaudio_stream_state_t next;
        result = AAudioStream_waitForStateChange(s->stream, state, &next, remaining);
        if (result != AAUDIO_OK) return remember(s, result);
        state = next;
    }
    s->started = false;
    return AUDIO_OK;
}
enum audio_result audio_input_close(audio_input **handle)
{
    if (!handle) return AUDIO_INVALID;
    audio_input *s = *handle;
    if (!s) return AUDIO_OK;
    (void)audio_input_stop(s);
    aaudio_result_t result = AAudioStream_close(s->stream);
    if (result != AAUDIO_OK) return remember(s, result);
    close(s->ready);
    free(s->storage); free(s); *handle = NULL;
    return AUDIO_OK;
}
