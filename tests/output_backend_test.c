#include "audio_output.h"
#include <aaudio/AAudio.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct AAudioStreamBuilder {
    void (*error)(AAudioStream *, void *, aaudio_result_t);
    void *context;
    int32_t direction, sharing, rate, channels;
    aaudio_format_t format;
};
struct AAudioStream {
    struct AAudioStreamBuilder builder;
    int state;
};

static struct AAudioStream *live;
static int closes, writes, wait_count;
static int32_t write_limit = INT32_MAX;

aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **b)
{
    *b = calloc(1, sizeof(**b));
    return *b ? AAUDIO_OK : AAUDIO_ERROR_NO_MEMORY;
}
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b)
{
    free(b);
    return AAUDIO_OK;
}
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b, int32_t v)
{
    b->direction = v;
}
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *b, int32_t v)
{
    b->sharing = v;
}
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b, int32_t v)
{
    b->channels = v;
}
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b, int32_t v)
{
    b->rate = v;
}
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b, aaudio_format_t v)
{
    b->format = v;
}
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, data_callback f, void *c)
{
    (void)b; (void)f; (void)c;
}
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, error_callback f, void *c)
{
    b->error = f; b->context = c;
}
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **s)
{
    assert(b->direction == AAUDIO_DIRECTION_OUTPUT);
    assert(b->sharing == AAUDIO_SHARING_MODE_SHARED);
    *s = calloc(1, sizeof(**s));
    assert(*s);
    (*s)->builder = *b;
    live = *s;
    return AAUDIO_OK;
}
int32_t AAudioStream_getSampleRate(AAudioStream *s) { return s->builder.rate; }
int32_t AAudioStream_getChannelCount(AAudioStream *s) { return s->builder.channels; }
aaudio_format_t AAudioStream_getFormat(AAudioStream *s) { return s->builder.format; }
aaudio_result_t AAudioStream_requestStart(AAudioStream *s)
{
    s->state = AAUDIO_STREAM_STATE_STARTED;
    return AAUDIO_OK;
}
aaudio_result_t AAudioStream_requestStop(AAudioStream *s)
{
    s->state = AAUDIO_STREAM_STATE_STOPPING;
    return AAUDIO_OK;
}
aaudio_stream_state_t AAudioStream_getState(AAudioStream *s) { return s->state; }
aaudio_result_t AAudioStream_waitForStateChange(AAudioStream *s,
                                                aaudio_stream_state_t old,
                                                aaudio_stream_state_t *next,
                                                int64_t timeout)
{
    assert(old == AAUDIO_STREAM_STATE_STOPPING && timeout > 0);
    wait_count++;
    s->state = AAUDIO_STREAM_STATE_STOPPED;
    *next = s->state;
    return AAUDIO_OK;
}
aaudio_result_t AAudioStream_write(AAudioStream *s, const void *pcm,
                                  int32_t frames, int64_t timeout)
{
    assert(s == live && s->state == AAUDIO_STREAM_STATE_STARTED);
    assert(pcm && frames >= 0 && timeout >= 0);
    writes++;
    return frames < write_limit ? frames : write_limit;
}
aaudio_result_t AAudioStream_close(AAudioStream *s)
{
    free(s); live = NULL; closes++;
    return AAUDIO_OK;
}

int main(void)
{
    audio_output *out = NULL;
    struct audio_error error;
    struct audio_properties wanted = {44100, 2, AUDIO_SIGNED16};

    assert(audio_output_open(NULL, wanted, &error) == AUDIO_INVALID);
    assert(audio_output_open(&out, wanted, &error) == AUDIO_OK);
    struct audio_properties actual = audio_output_properties(out);
    assert(actual.sample_rate == 44100);
    assert(actual.channels == 2);
    assert(actual.format == AUDIO_SIGNED16);

    assert(audio_output_start(out) == AUDIO_OK);
    assert(audio_output_start(out) == AUDIO_OK);

    int16_t pcm[12] = {0};
    size_t written = 99;
    assert(audio_output_write(out, pcm, 6, 20, &written) == AUDIO_OK);
    assert(written == 6 && writes == 1);

    write_limit = 2;
    assert(audio_output_write(out, pcm, 6, 0, &written) == AUDIO_OK);
    assert(written == 2 && writes == 2);

    live->builder.error(live, live->builder.context, AAUDIO_ERROR_DISCONNECTED);
    assert(audio_output_error(out).code == AUDIO_DISCONNECTED);
    assert(audio_output_write(out, pcm, 1, 0, &written) == AUDIO_DISCONNECTED);
    assert(written == 0);

    assert(audio_output_close(&out) == AUDIO_OK);
    assert(!out && closes == 1);

    assert(audio_output_open(&out, wanted, &error) == AUDIO_OK);
    assert(audio_output_start(out) == AUDIO_OK);
    assert(audio_output_stop(out) == AUDIO_OK);
    assert(wait_count == 1);
    assert(audio_output_close(&out) == AUDIO_OK);

    puts("PASS AAudio output open, actual properties, write, partial write, disconnect, stop and close");
    return 0;
}
