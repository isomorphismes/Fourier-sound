#include "audio_input.h"
#include <aaudio/AAudio.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct AAudioStreamBuilder { data_callback data; error_callback error; void *context; int32_t rate, format; };
struct AAudioStream { struct AAudioStreamBuilder builder; int state; };
static struct AAudioStream *live;
static int format = AAUDIO_FORMAT_PCM_FLOAT, channels = 2, fail_rate, fail_format, closes;
static int stop_timeout, close_failure, waited;
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **b) { *b = calloc(1, sizeof(**b)); return *b ? AAUDIO_OK : AAUDIO_ERROR_NO_MEMORY; }
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b) { free(b); return AAUDIO_OK; }
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b, int32_t v) { (void)b; assert(v == AAUDIO_DIRECTION_INPUT); }
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *b, int32_t v) { (void)b; assert(v == AAUDIO_SHARING_MODE_SHARED); }
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b, int32_t v) { (void)b; assert(v == 1); }
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b, int32_t v) { b->rate = v; }
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b, aaudio_format_t v) { b->format = v; }
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, data_callback f, void *c) { b->data = f; b->context = c; }
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, error_callback f, void *c) { b->error = f; assert(b->context == c); }
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **s)
{
    *s = NULL;
    if (fail_rate && b->rate != AAUDIO_UNSPECIFIED) return AAUDIO_ERROR_INVALID_RATE;
    if (fail_format && b->format != AAUDIO_FORMAT_PCM_I16) return AAUDIO_ERROR_INVALID_FORMAT;
    *s = calloc(1, sizeof(**s)); assert(*s);
    (*s)->builder = *b; live = *s; return AAUDIO_OK;
}
int32_t AAudioStream_getSampleRate(AAudioStream *s) { (void)s; return 44100; }
int32_t AAudioStream_getChannelCount(AAudioStream *s) { (void)s; return channels; }
aaudio_format_t AAudioStream_getFormat(AAudioStream *s) { (void)s; return format; }
aaudio_result_t AAudioStream_requestStart(AAudioStream *s) { s->state = AAUDIO_STREAM_STATE_STARTED; return AAUDIO_OK; }
aaudio_result_t AAudioStream_requestStop(AAudioStream *s) { s->state = AAUDIO_STREAM_STATE_STOPPING; return AAUDIO_OK; }
aaudio_stream_state_t AAudioStream_getState(AAudioStream *s) { return s->state; }
aaudio_result_t AAudioStream_waitForStateChange(AAudioStream *s, aaudio_stream_state_t old, aaudio_stream_state_t *next, int64_t time)
{
    assert(old == AAUDIO_STREAM_STATE_STOPPING && time > 0); waited++;
    if (stop_timeout) return AAUDIO_ERROR_TIMEOUT;
    s->state = AAUDIO_STREAM_STATE_STOPPED; *next = s->state; return AAUDIO_OK;
}
aaudio_result_t AAudioStream_close(AAudioStream *s)
{
    if (close_failure) return AAUDIO_ERROR_INVALID_STATE;
    free(s); live = NULL; closes++; return AAUDIO_OK;
}
static void send(void *pcm, int frames)
{
    assert(live && live->state == AAUDIO_STREAM_STATE_STARTED);
    assert(live->builder.data(live, live->builder.context, pcm, frames) == AAUDIO_CALLBACK_RESULT_CONTINUE);
}
int main(void)
{
    audio_input *s = NULL;
    struct audio_error error;
    assert(audio_input_open(NULL, &error) == AUDIO_INVALID && error.code == AUDIO_INVALID);
    assert(audio_input_open(&s, &error) == AUDIO_OK);
    struct audio_properties p = audio_input_properties(s);
    assert(p.sample_rate == 44100 && p.channels == 2 && p.format == AUDIO_FLOAT32);
    assert(audio_input_wait(s, 0) == AUDIO_INVALID);
    assert(audio_input_start(s) == AUDIO_OK && audio_input_start(s) == AUDIO_OK);
    assert(audio_input_wait(s, 0) == AUDIO_TIMEOUT);
    float samples[] = {0.25f, -0.5f, 0.75f, -1.0f}, output[4];
    send(samples, 2);
    assert(audio_input_wait(s, 0) == AUDIO_OK);
    size_t count;
    assert(audio_input_read(s, output, 2, &count) == AUDIO_OK && count == 2);
    assert(!memcmp(samples, output, sizeof(samples)));
    send(samples, 2);
    assert(audio_input_stop(s) == AUDIO_OK && audio_input_stop(s) == AUDIO_OK);
    assert(waited == 1);
    assert(audio_input_start(s) == AUDIO_OK);
    assert(audio_input_read(s, output, 2, &count) == AUDIO_OK && !count);
    float *large = calloc(40000 * 2, sizeof(float)); assert(large);
    send(large, 40000); free(large);
    assert(audio_input_dropped_frames(s) == 40000 - 32768);
    live->builder.error(live, live->builder.context, AAUDIO_ERROR_DISCONNECTED);
    assert(audio_input_wait(s, 0) == AUDIO_DISCONNECTED);
    assert(audio_input_read(s, output, 2, &count) == AUDIO_DISCONNECTED && count == 0);
    assert(audio_input_start(s) == AUDIO_DISCONNECTED);
    assert(audio_input_error(s).native_code == AAUDIO_ERROR_DISCONNECTED);
    assert(audio_input_close(&s) == AUDIO_OK && !s);
    assert(audio_input_close(&s) == AUDIO_OK);
    format = AAUDIO_FORMAT_PCM_I16; channels = 1; fail_rate = fail_format = 1;
    assert(audio_input_open(&s, &error) == AUDIO_OK);
    assert(audio_input_properties(s).format == AUDIO_SIGNED16);
    assert(live->builder.rate == 0 && live->builder.format == AAUDIO_FORMAT_PCM_I16);
    assert(audio_input_start(s) == AUDIO_OK);
    int16_t signed_pcm[] = {-32768, 0, 32767}, signed_output[3];
    send(signed_pcm, 3);
    assert(audio_input_read(s, signed_output, 3, &count) == AUDIO_OK && count == 3);
    assert(!memcmp(signed_pcm, signed_output, sizeof(signed_pcm)));
    assert(audio_input_close(&s) == AUDIO_OK);
    assert(audio_input_open(&s, &error) == AUDIO_OK);
    assert(audio_input_start(s) == AUDIO_OK);
    stop_timeout = 1;
    assert(audio_input_stop(s) == AUDIO_TIMEOUT);
    close_failure = 1;
    assert(audio_input_close(&s) == AUDIO_INVALID && s != NULL);
    stop_timeout = close_failure = 0;
    assert(audio_input_close(&s) == AUDIO_OK && s == NULL);
    format = 999;
    assert(audio_input_open(&s, &error) == AUDIO_UNSUPPORTED && !s);
    assert(closes == 4);
    puts("PASS simulated AAudio actual properties, rate/format negotiation, readiness, copy, restart, overflow, disconnect and cleanup");
    return 0;
}
