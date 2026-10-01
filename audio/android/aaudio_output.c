#define _POSIX_C_SOURCE 200809L
#include "audio_output.h"

#include <aaudio/AAudio.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

struct audio_output {
    AAudioStream *stream;
    struct audio_properties properties;
    bool started;
    _Atomic int native_error;
};

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

static enum audio_result remember(audio_output *s, aaudio_result_t code)
{
    if (code < 0)
        atomic_store_explicit(&s->native_error, code, memory_order_relaxed);
    return translate(code);
}

static void failed(AAudioStream *stream, void *context, aaudio_result_t error)
{
    (void)stream;
    audio_output *s = context;
    atomic_store_explicit(&s->native_error, error, memory_order_relaxed);
}

struct audio_error audio_output_error(const audio_output *s)
{
    if (!s) return (struct audio_error){AUDIO_INVALID, 0};
    int code = atomic_load_explicit(&s->native_error, memory_order_relaxed);
    return (struct audio_error){translate(code), code};
}

static aaudio_format_t native_format(enum audio_sample_format format)
{
    switch (format) {
    case AUDIO_FLOAT32: return AAUDIO_FORMAT_PCM_FLOAT;
    case AUDIO_SIGNED16: return AAUDIO_FORMAT_PCM_I16;
    }
    return AAUDIO_FORMAT_INVALID;
}

enum audio_result audio_output_open(audio_output **out,
                                    struct audio_properties requested,
                                    struct audio_error *error)
{
    if (error) *error = (struct audio_error){AUDIO_OK, 0};
    if (!out || !requested.sample_rate || requested.channels < 1 ||
        requested.channels > 8 || native_format(requested.format) == AAUDIO_FORMAT_INVALID) {
        if (error) error->code = AUDIO_INVALID;
        return AUDIO_INVALID;
    }
    *out = NULL;

    audio_output *s = calloc(1, sizeof(*s));
    AAudioStreamBuilder *builder = NULL;
    aaudio_result_t native = AAUDIO_OK;
    enum audio_result result = AUDIO_NO_MEMORY;
    if (!s) goto failure;

    atomic_init(&s->native_error, AAUDIO_OK);
    if (!atomic_is_lock_free(&s->native_error)) {
        result = AUDIO_UNSUPPORTED;
        goto failure;
    }

    native = AAudio_createStreamBuilder(&builder);
    if (native != AAUDIO_OK) {
        result = translate(native);
        goto failure;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setChannelCount(builder, (int32_t)requested.channels);
    AAudioStreamBuilder_setSampleRate(builder, (int32_t)requested.sample_rate);
    AAudioStreamBuilder_setFormat(builder, native_format(requested.format));
    AAudioStreamBuilder_setErrorCallback(builder, failed, s);

    native = AAudioStreamBuilder_openStream(builder, &s->stream);
    AAudioStreamBuilder_delete(builder);
    builder = NULL;
    if (native != AAUDIO_OK) {
        result = translate(native);
        goto failure;
    }

    int32_t rate = AAudioStream_getSampleRate(s->stream);
    int32_t channels = AAudioStream_getChannelCount(s->stream);
    aaudio_format_t format = AAudioStream_getFormat(s->stream);
    if (rate <= 0 || channels < 1 || channels > 8 ||
        (format != AAUDIO_FORMAT_PCM_FLOAT && format != AAUDIO_FORMAT_PCM_I16)) {
        result = AUDIO_UNSUPPORTED;
        goto failure;
    }

    s->properties = (struct audio_properties){
        (uint32_t)rate,
        (uint32_t)channels,
        format == AAUDIO_FORMAT_PCM_FLOAT ? AUDIO_FLOAT32 : AUDIO_SIGNED16
    };
    *out = s;
    return AUDIO_OK;

failure:
    if (builder) AAudioStreamBuilder_delete(builder);
    if (s) {
        if (s->stream) AAudioStream_close(s->stream);
        free(s);
    }
    if (error) *error = (struct audio_error){result, native};
    return result;
}

struct audio_properties audio_output_properties(const audio_output *s)
{
    return s ? s->properties : (struct audio_properties){0, 0, 0};
}

enum audio_result audio_output_start(audio_output *s)
{
    if (!s) return AUDIO_INVALID;
    enum audio_result error = audio_output_error(s).code;
    if (error != AUDIO_OK) return error;
    if (s->started) return AUDIO_OK;

    aaudio_result_t result = AAudioStream_requestStart(s->stream);
    if (result == AAUDIO_OK) s->started = true;
    return remember(s, result);
}

enum audio_result audio_output_write(audio_output *s, const void *pcm,
                                     size_t frames, int timeout_ms,
                                     size_t *written)
{
    if (written) *written = 0;
    if (!s || !pcm || !written || timeout_ms < 0 || !s->started ||
        frames > INT32_MAX)
        return AUDIO_INVALID;

    enum audio_result error = audio_output_error(s).code;
    if (error != AUDIO_OK) return error;
    if (!frames) return AUDIO_OK;

    int64_t timeout_ns = (int64_t)timeout_ms * 1000000;
    aaudio_result_t result =
        AAudioStream_write(s->stream, pcm, (int32_t)frames, timeout_ns);
    if (result < 0) return remember(s, result);

    *written = (size_t)result;
    return AUDIO_OK;
}

static int64_t now_ns(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (int64_t)value.tv_sec * 1000000000 + value.tv_nsec;
}

enum audio_result audio_output_stop(audio_output *s)
{
    if (!s) return AUDIO_INVALID;
    if (!s->started) return AUDIO_OK;

    aaudio_result_t result = AAudioStream_requestStop(s->stream);
    if (result != AAUDIO_OK) return remember(s, result);

    int64_t deadline = now_ns() + 2000000000;
    aaudio_stream_state_t state = AAudioStream_getState(s->stream);
    while (state != AAUDIO_STREAM_STATE_STOPPED) {
        if (state == AAUDIO_STREAM_STATE_DISCONNECTED)
            return remember(s, AAUDIO_ERROR_DISCONNECTED);
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

enum audio_result audio_output_close(audio_output **handle)
{
    if (!handle) return AUDIO_INVALID;
    audio_output *s = *handle;
    if (!s) return AUDIO_OK;

    (void)audio_output_stop(s);
    aaudio_result_t result = AAudioStream_close(s->stream);
    if (result != AAUDIO_OK) return remember(s, result);

    free(s);
    *handle = NULL;
    return AUDIO_OK;
}
