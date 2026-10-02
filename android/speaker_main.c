#define _POSIX_C_SOURCE 200809L
#include <android/log.h>
#include <android_native_app_glue.h>
#include "audio_output.h"
#include "speaker_input.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "FourierSpeaker", __VA_ARGS__)
#define BLOCK_FRAMES 512U

struct application {
    struct android_app *app;
    audio_output *output;
    struct speaker_input input;
    struct audio_properties properties;
    float *acceptance_signal;
    uint64_t target_frames;
    uint64_t written_frames;
    unsigned char pcm[BLOCK_FRAMES * 8U * sizeof(float)];
    size_t pending_frames;
    size_t pending_offset;
    bool resumed;
    bool focused;
    bool finished;
};

static size_t bytes_per_frame(struct audio_properties p)
{
    size_t sample = p.format == AUDIO_FLOAT32 ? sizeof(float) : sizeof(int16_t);
    return (size_t)p.channels * sample;
}

static bool close_output(struct application *a)
{
    if (!a->output) return true;
    enum audio_result stopped = audio_output_stop(a->output);
    enum audio_result closed = audio_output_close(&a->output);
    LOG("SPEAKER_CLOSE stop=%s close=%s",
        audio_result_text(stopped), audio_result_text(closed));
    return stopped == AUDIO_OK && closed == AUDIO_OK;
}

static void finish(struct application *a, const char *status)
{
    struct audio_error error = a->output
        ? audio_output_error(a->output)
        : (struct audio_error){AUDIO_OK, 0};
    bool clean = close_output(a);
    LOG("SPEAKER_RESULT status=%s rate=%u channels=%u format=%s frames=%llu clean_close=%d error=%s native_error=%d",
        status, a->properties.sample_rate, a->properties.channels,
        a->properties.format == AUDIO_FLOAT32 ? "float32" : "signed16",
        (unsigned long long)a->written_frames, clean,
        audio_result_text(error.code), error.native_code);
    a->finished = true;
    ANativeActivity_finish(a->app->activity);
}

static bool make_acceptance_input(struct application *a)
{
    size_t frames = a->properties.sample_rate;
    a->acceptance_signal = malloc(frames * sizeof(*a->acceptance_signal));
    if (!a->acceptance_signal) return false;

    /* Producer fixture only. The speaker itself consumes speaker_input. One
     * second contains an exact 440 cycles, so looping has no boundary click. */
    const double tau = 6.283185307179586476925286766559;
    for (size_t i = 0; i < frames; ++i) {
        double phase = tau * 440.0 * (double)i / (double)a->properties.sample_rate;
        a->acceptance_signal[i] = (float)(0.20 * sin(phase));
    }
    return speaker_input_set(&a->input, a->acceptance_signal, frames, true);
}

static void start_if_ready(struct application *a)
{
    if (!a->resumed || !a->focused || a->finished || a->output) return;

    struct audio_properties wanted = {44100, 2, AUDIO_SIGNED16};
    struct audio_error error;
    enum audio_result result = audio_output_open(&a->output, wanted, &error);
    if (result != AUDIO_OK) {
        LOG("SPEAKER_OPEN_ERROR %s native_error=%d",
            audio_result_text(result), error.native_code);
        a->finished = true;
        ANativeActivity_finish(a->app->activity);
        return;
    }

    a->properties = audio_output_properties(a->output);
    if (!make_acceptance_input(a)) {
        finish(a, "INPUT_ALLOCATION_ERROR");
        return;
    }

    result = audio_output_start(a->output);
    if (result != AUDIO_OK) {
        finish(a, "START_ERROR");
        return;
    }

    a->target_frames = (uint64_t)a->properties.sample_rate * 3U;
    a->written_frames = 0;
    a->pending_frames = 0;
    a->pending_offset = 0;
    LOG("SPEAKER_OPEN rate=%u channels=%u format=%s source=speaker_input acceptance=440Hz duration_seconds=3",
        a->properties.sample_rate, a->properties.channels,
        a->properties.format == AUDIO_FLOAT32 ? "float32" : "signed16");
}

static void command(struct android_app *app, int32_t code)
{
    struct application *a = app->userData;
    switch (code) {
    case APP_CMD_RESUME:
        a->resumed = true;
        start_if_ready(a);
        break;
    case APP_CMD_GAINED_FOCUS:
        a->focused = true;
        start_if_ready(a);
        break;
    case APP_CMD_LOST_FOCUS:
        a->focused = false;
        (void)close_output(a);
        free(a->acceptance_signal);
        a->acceptance_signal = NULL;
        a->pending_frames = 0;
        break;
    case APP_CMD_PAUSE:
        a->resumed = false;
        (void)close_output(a);
        free(a->acceptance_signal);
        a->acceptance_signal = NULL;
        a->pending_frames = 0;
        break;
    default:
        break;
    }
}

static void play_some(struct application *a)
{
    if (!a->output || a->finished) return;

    if (!a->pending_frames) {
        uint64_t remaining = a->target_frames - a->written_frames;
        size_t capacity = remaining < BLOCK_FRAMES ? (size_t)remaining : BLOCK_FRAMES;
        size_t produced = 0;
        if (!speaker_input_render(&a->input, a->properties, a->pcm,
                                  capacity, &produced) || !produced) {
            finish(a, "INPUT_ERROR");
            return;
        }
        a->pending_frames = produced;
        a->pending_offset = 0;
    }

    size_t frame_bytes = bytes_per_frame(a->properties);
    const void *next = a->pcm + a->pending_offset * frame_bytes;
    size_t written = 0;
    enum audio_result result =
        audio_output_write(a->output, next, a->pending_frames, 20, &written);
    if (result != AUDIO_OK) {
        finish(a, "WRITE_ERROR");
        return;
    }
    if (!written) return;

    a->pending_offset += written;
    a->pending_frames -= written;
    a->written_frames += written;

    if (a->written_frames >= a->target_frames)
        finish(a, "PLAYED");
}

void android_main(struct android_app *app)
{
    struct application a = {.app = app};
    app->userData = &a;
    app->onAppCmd = command;

    LOG("SPEAKER_TEST started; experiment data enters through speaker_input");
    while (!app->destroyRequested) {
        struct android_poll_source *source = NULL;
        int ident = ALooper_pollOnce(a.output ? 0 : -1, NULL, NULL, (void **)&source);
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
        if (ident == ALOOPER_POLL_ERROR) {
            if (a.output) finish(&a, "LOOPER_ERROR");
            break;
        }
        if (a.output) play_some(&a);
    }

    (void)close_output(&a);
    free(a.acceptance_signal);
}
