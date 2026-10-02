#define _POSIX_C_SOURCE 200809L
#include <android/log.h>
#include <android_native_app_glue.h>
#include "aaudio_input.h"
#include "microphone_check.h"
#include <inttypes.h>
#include <time.h>
#include <unistd.h>

#define INPUT_READY (LOOPER_ID_USER + 1)
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "FourierMic", __VA_ARGS__)
bool microphone_permission(ANativeActivity *activity);
struct application {
    struct android_app *app;
    audio_input *input;
    struct microphone_check check;
    bool resumed, focused, finished, pending_pcm;
    int64_t deadline;
};
static int64_t now_ms(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (int64_t)value.tv_sec * 1000 + value.tv_nsec / 1000000;
}
static bool close_input(struct application *a)
{
    a->pending_pcm = false;
    if (!a->input) return true;
    (void)ALooper_removeFd(a->app->looper, android_audio_input_ready_fd(a->input));
    enum audio_result stopped = audio_input_stop(a->input);
    enum audio_result closed = audio_input_close(&a->input);
    LOG("MIC_CLOSE stop=%s close=%s", audio_result_text(stopped), audio_result_text(closed));
    return stopped == AUDIO_OK && closed == AUDIO_OK;
}
static void finish(struct application *a, const char *status)
{
    uint32_t dropped = audio_input_dropped_frames(a->input);
    struct audio_error error = a->input ? audio_input_error(a->input) : (struct audio_error){AUDIO_OK, 0};
    bool clean = close_input(a);
    struct microphone_check *c = &a->check;
    LOG("MIC_RESULT status=%s rate=%u channels=%u format=%s frames=%" PRIu64
        " samples=%" PRIu64 " nonzero=%" PRIu64 " min=%.9g max=%.9g mean=%.9g rms=%.9g"
        " mono_samples=%" PRIu64 " dropped=%u clean_close=%d error=%s native_error=%d",
        status, c->properties.sample_rate, c->properties.channels,
        c->properties.format == AUDIO_FLOAT32 ? "float32" : "signed16",
        c->frames, c->samples, c->nonzero, c->minimum, c->maximum, c->mean,
        microphone_check_rms(c), c->mono_samples, dropped, clean,
        audio_result_text(error.code), error.native_code);
    a->finished = true;
    ANativeActivity_finish(a->app->activity);
}
static void start_if_ready(struct application *a)
{
    if (!a->resumed || !a->focused || a->finished || a->input) return;
    if (!microphone_permission(a->app->activity)) {
        LOG("MIC_PERMISSION microphone permission required; allow it or grant it in app settings, then reopen");
        return;
    }
    struct audio_error error;
    enum audio_result result = audio_input_open(&a->input, &error);
    if (result != AUDIO_OK) {
        LOG("MIC_OPEN_ERROR %s native_error=%d", audio_result_text(result), error.native_code);
        a->finished = true; ANativeActivity_finish(a->app->activity); return;
    }
    struct audio_properties p = audio_input_properties(a->input);
    microphone_check_init(&a->check, p);
    LOG("MIC_OPEN rate=%u channels=%u format=%s duration_seconds=5; speak or tap near the microphone",
        p.sample_rate, p.channels, p.format == AUDIO_FLOAT32 ? "float32" : "signed16");
    if (ALooper_addFd(a->app->looper, android_audio_input_ready_fd(a->input),
                     INPUT_READY, ALOOPER_EVENT_INPUT, NULL, NULL) < 0) {
        finish(a, "READINESS_ERROR"); return;
    }
    result = audio_input_start(a->input);
    if (result != AUDIO_OK) { finish(a, "START_ERROR"); return; }
    a->deadline = now_ms() + 10000;
}
static void command(struct android_app *app, int32_t code)
{
    struct application *a = app->userData;
    switch (code) {
    case APP_CMD_RESUME: a->resumed = true; start_if_ready(a); break;
    case APP_CMD_GAINED_FOCUS: a->focused = true; start_if_ready(a); break;
    case APP_CMD_LOST_FOCUS: a->focused = false; (void)close_input(a); break;
    case APP_CMD_PAUSE: a->resumed = false; (void)close_input(a); break;
    default: break;
    }
}
static void consume(struct application *a)
{
    a->pending_pcm = false;
    if (!a->input) return;
    uint64_t notification;
    ssize_t drained = read(android_audio_input_ready_fd(a->input), &notification, sizeof(notification));
    (void)drained;
    /* Bounded work per looper turn preserves lifecycle responsiveness. */
    for (unsigned int batch = 0; batch < 16; ++batch) {
        float pcm[512 * 8]; /* Also enough aligned storage for signed16 PCM. */
        uint64_t target = (uint64_t)a->check.properties.sample_rate * 5;
        size_t capacity = target - a->check.frames < 512 ? (size_t)(target - a->check.frames) : 512;
        size_t received;
        enum audio_result result = audio_input_read(a->input, pcm, capacity, &received);
        if (result != AUDIO_OK) { finish(a, "INPUT_ERROR"); return; }
        if (!received) return;
        if (!microphone_check_accept(&a->check, pcm, received)) { finish(a, "INVALID_PCM"); return; }
        if (audio_input_dropped_frames(a->input)) { finish(a, "OVERFLOW"); return; }
        if (a->check.frames >= target) {
            finish(a, a->check.nonzero ? "SAMPLES_RECEIVED" : "ALL_ZERO_INCONCLUSIVE");
            return;
        }
    }
    a->pending_pcm = true;
}
void android_main(struct android_app *app)
{
    struct application a = {.app = app};
    app->userData = &a; app->onAppCmd = command;
    LOG("MIC_TEST started; no PCM is saved to disk");
    while (!app->destroyRequested) {
        int timeout = -1;
        if (a.input) {
            int64_t remaining = a.deadline - now_ms();
            if (remaining <= 0) { finish(&a, "TIMEOUT"); continue; }
            timeout = a.pending_pcm ? 0 : (int)remaining;
        }
        struct android_poll_source *source = NULL;
        int ident = ALooper_pollOnce(timeout, NULL, NULL, (void **)&source);
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
        if (ident == INPUT_READY || a.pending_pcm) consume(&a);
        if (ident == ALOOPER_POLL_ERROR) { if (a.input) finish(&a, "LOOPER_ERROR"); break; }
    }
    (void)close_input(&a);
}
