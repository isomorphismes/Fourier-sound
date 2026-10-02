#define _POSIX_C_SOURCE 200809L
#include <android/log.h>
#include <android_native_app_glue.h>

#include "aaudio_input.h"
#include "framing.h"
#include "gpu_voice_pipeline.h"
#include "pcm_block.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define INPUT_READY (LOOPER_ID_USER + 1)
#define SAMPLE_COUNT 1024U
#define TERM_COUNT 24U
#define READ_FRAMES 512U
#define RENDER_WIDTH 96
#define RENDER_HEIGHT 192
#define FRAME_INTERVAL_MS 200
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "FourierVoice", __VA_ARGS__)

bool microphone_permission(ANativeActivity *activity);

struct application {
    struct android_app *app;
    audio_input *input;
    struct gpu_voice_pipeline *gpu;
    struct audio_properties properties;
    bool resumed;
    bool focused;
    bool window_ready;
    float recent[SAMPLE_COUNT];
    size_t recent_count;
    uint32_t dropped;
    int64_t last_render_ms;
    uint64_t frame_number;
};

static int64_t now_ms(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (int64_t)value.tv_sec * 1000 + value.tv_nsec / 1000000;
}

static void close_input(struct application *a)
{
    if (!a->input) return;
    (void)ALooper_removeFd(a->app->looper,
                           android_audio_input_ready_fd(a->input));
    enum audio_result stopped = audio_input_stop(a->input);
    enum audio_result closed = audio_input_close(&a->input);
    LOG("VOICE_MIC_CLOSE stop=%s close=%s",
        audio_result_text(stopped), audio_result_text(closed));
    a->recent_count = 0U;
    a->dropped = 0U;
}

static void close_gpu(struct application *a)
{
    if (!a->gpu) return;
    gpu_voice_pipeline_destroy(&a->gpu);
    LOG("VOICE_GPU_CLOSE");
}

static void remember_samples(struct application *a, const float *mono,
                             size_t count)
{
    if (count >= SAMPLE_COUNT) {
        memcpy(a->recent, mono + count - SAMPLE_COUNT, sizeof(a->recent));
        a->recent_count = SAMPLE_COUNT;
        return;
    }

    size_t keep = a->recent_count;
    if (keep > SAMPLE_COUNT - count) keep = SAMPLE_COUNT - count;
    if (keep && keep < a->recent_count) {
        memmove(a->recent, a->recent + (a->recent_count - keep),
                keep * sizeof(*a->recent));
    }
    memcpy(a->recent + keep, mono, count * sizeof(*mono));
    a->recent_count = keep + count;
}

static bool prepare_gpu(struct application *a)
{
    if (a->gpu) return true;
    if (!a->window_ready || !a->app->window) return false;

    a->gpu = gpu_voice_pipeline_create(
        a->app->window, SAMPLE_COUNT, TERM_COUNT,
        RENDER_WIDTH, RENDER_HEIGHT);
    if (!a->gpu) {
        LOG("VOICE_GPU_CREATE_ERROR");
        return false;
    }
    LOG("VOICE_GPU_PATH fft=compute-shader spectrum=SSBO "
        "renderer=fragment-shader spectrum_readback=none");
    return true;
}

static bool render_voice(struct application *a)
{
    float samples[SAMPLE_COUNT];
    memcpy(samples, a->recent, sizeof(samples));

    double input_rms = 0.0;
    double framed_rms = 0.0;
    if (!fourier_frame_rms(samples, SAMPLE_COUNT, &input_rms) ||
        !fourier_frame_remove_mean(samples, SAMPLE_COUNT) ||
        !fourier_frame_apply_hann_periodic(samples, SAMPLE_COUNT) ||
        !fourier_frame_rms(samples, SAMPLE_COUNT, &framed_rms))
        return false;

    if (!prepare_gpu(a)) return false;
    enum gpu_voice_result result =
        gpu_voice_pipeline_render(a->gpu, samples, SAMPLE_COUNT);
    if (result != GPU_VOICE_OK) {
        LOG("VOICE_GPU_RENDER_ERROR %s", gpu_voice_result_text(result));
        close_gpu(a);
        return false;
    }

    ++a->frame_number;
    LOG("VOICE_FRAME number=%" PRIu64
        " rate=%u samples=%u window_ms=%.3f terms=%u "
        "logical_size=%dx%d input_rms=%.7g framed_rms=%.7g "
        "spectrum_readback=none",
        a->frame_number, a->properties.sample_rate, SAMPLE_COUNT,
        1000.0 * (double)SAMPLE_COUNT / (double)a->properties.sample_rate,
        TERM_COUNT, RENDER_WIDTH, RENDER_HEIGHT, input_rms, framed_rms);
    return true;
}

static void start_if_ready(struct application *a)
{
    if (!a->resumed || !a->focused || !a->window_ready || a->input) return;
    if (!microphone_permission(a->app->activity)) {
        LOG("VOICE_PERMISSION microphone permission required");
        return;
    }
    if (!prepare_gpu(a)) return;

    struct audio_error error;
    enum audio_result result = audio_input_open(&a->input, &error);
    if (result != AUDIO_OK) {
        LOG("VOICE_MIC_OPEN_ERROR %s native_error=%d",
            audio_result_text(result), error.native_code);
        return;
    }

    a->properties = audio_input_properties(a->input);
    if (ALooper_addFd(a->app->looper,
                      android_audio_input_ready_fd(a->input),
                      INPUT_READY, ALOOPER_EVENT_INPUT, NULL, NULL) < 0) {
        LOG("VOICE_READINESS_ERROR");
        close_input(a);
        return;
    }

    result = audio_input_start(a->input);
    if (result != AUDIO_OK) {
        LOG("VOICE_MIC_START_ERROR %s", audio_result_text(result));
        close_input(a);
        return;
    }

    a->last_render_ms = 0;
    LOG("VOICE_STARTED rate=%u channels=%u format=%s samples=%u "
        "window_ms=%.3f render=%dx%d interval_ms=%d terms=%u "
        "fft=gpu-radix2 wegert=gpu spectrum_readback=none",
        a->properties.sample_rate, a->properties.channels,
        a->properties.format == AUDIO_FLOAT32 ? "float32" : "signed16",
        SAMPLE_COUNT,
        1000.0 * (double)SAMPLE_COUNT / (double)a->properties.sample_rate,
        RENDER_WIDTH, RENDER_HEIGHT, FRAME_INTERVAL_MS, TERM_COUNT);
}

static void command(struct android_app *app, int32_t code)
{
    struct application *a = app->userData;
    switch (code) {
    case APP_CMD_INIT_WINDOW:
        a->window_ready = true;
        start_if_ready(a);
        break;
    case APP_CMD_TERM_WINDOW:
        a->window_ready = false;
        close_input(a);
        close_gpu(a);
        break;
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
        close_input(a);
        break;
    case APP_CMD_PAUSE:
        a->resumed = false;
        close_input(a);
        break;
    default:
        break;
    }
}

static void consume(struct application *a)
{
    if (!a->input) return;

    uint64_t notification;
    ssize_t drained = read(android_audio_input_ready_fd(a->input),
                           &notification, sizeof(notification));
    (void)drained;

    for (unsigned int batch = 0U; batch < 16U; ++batch) {
        unsigned char raw[READ_FRAMES * 8U * sizeof(float)];
        float mono[READ_FRAMES];
        size_t received = 0U;
        enum audio_result result = audio_input_read(
            a->input, raw, READ_FRAMES, &received);
        if (result != AUDIO_OK) {
            LOG("VOICE_INPUT_ERROR %s", audio_result_text(result));
            close_input(a);
            return;
        }
        if (!received) break;
        if (!fourier_pcm_mono(a->properties, raw, received,
                              mono, READ_FRAMES)) {
            LOG("VOICE_PCM_ERROR");
            close_input(a);
            return;
        }

        uint32_t dropped = audio_input_dropped_frames(a->input);
        if (dropped != a->dropped) {
            LOG("VOICE_DISCONTINUITY dropped=%u", dropped);
            a->dropped = dropped;
            a->recent_count = 0U;
        }

        remember_samples(a, mono, received);
        int64_t now = now_ms();
        if (a->recent_count == SAMPLE_COUNT &&
            now - a->last_render_ms >= FRAME_INTERVAL_MS) {
            if (!render_voice(a)) {
                close_input(a);
                return;
            }
            a->last_render_ms = now;
        }
    }
}

void android_main(struct android_app *app)
{
    struct application a = {.app = app};
    app->userData = &a;
    app->onAppCmd = command;

    LOG("VOICE_APP started path=gpu-resident");
    while (!app->destroyRequested) {
        struct android_poll_source *source = NULL;
        int ident = ALooper_pollOnce(-1, NULL, NULL, (void **)&source);
        if (source) source->process(app, source);
        if (app->destroyRequested) break;
        if (ident == INPUT_READY) consume(&a);
        if (ident == ALOOPER_POLL_ERROR) {
            LOG("VOICE_LOOPER_ERROR");
            break;
        }
    }

    close_input(&a);
    close_gpu(&a);
    LOG("VOICE_APP stopped");
}
