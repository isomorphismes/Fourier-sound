#define _POSIX_C_SOURCE 200809L
#include <android/log.h>
#include <android_native_app_glue.h>

#include "aaudio_input.h"
#include "complex_field.h"
#include "fft.h"
#include "framing.h"
#include "native_window_output.h"
#include "pcm_block.h"
#include "wegert.h"

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define INPUT_READY (LOOPER_ID_USER + 1)
#define SAMPLE_COUNT 1024U
#define TERM_COUNT 24U
#define READ_FRAMES 512U
#define RENDER_WIDTH 96U
#define RENDER_HEIGHT 192U
#define RENDER_PIXELS (RENDER_WIDTH * RENDER_HEIGHT)
#define FRAME_INTERVAL_MS 200
#define PEAK_COUNT 5U
#define FIELD_X_RADIUS 0.44
#define FIELD_Y_RADIUS 0.88
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "FourierVoice", __VA_ARGS__)

bool microphone_permission(ANativeActivity *activity);

struct spectral_peak {
    size_t bin;
    double magnitude;
};

struct application {
    struct android_app *app;
    audio_input *input;
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

static bool near(double actual, double expected, double tolerance)
{
    return fabs(actual - expected) <= tolerance;
}

static bool coefficient_self_test(void)
{
    const double tau = 6.283185307179586476925286766559;
    float samples[SAMPLE_COUNT];
    struct complex_value coefficients[SAMPLE_COUNT];

    for (size_t n = 0U; n < SAMPLE_COUNT; ++n) {
        double phase = tau * (double)n / (double)SAMPLE_COUNT;
        samples[n] = (float)(
            0.1 +
            0.5 * cos(8.0 * phase) +
            0.25 * sin(16.0 * phase)
        );
    }

    bool transformed = fourier_fft_real_radix2(
        samples, SAMPLE_COUNT, coefficients, SAMPLE_COUNT);
    bool passed = transformed &&
        near(coefficients[0].real, 0.1, 2e-6) &&
        near(coefficients[0].imaginary, 0.0, 2e-6) &&
        near(coefficients[8].real, 0.25, 2e-6) &&
        near(coefficients[8].imaginary, 0.0, 2e-6) &&
        near(coefficients[16].real, 0.0, 2e-6) &&
        near(coefficients[16].imaginary, -0.125, 2e-6) &&
        near(coefficients[SAMPLE_COUNT - 8U].real, 0.25, 2e-6) &&
        near(coefficients[SAMPLE_COUNT - 8U].imaginary, 0.0, 2e-6) &&
        near(coefficients[SAMPLE_COUNT - 16U].real, 0.0, 2e-6) &&
        near(coefficients[SAMPLE_COUNT - 16U].imaginary, 0.125, 2e-6);

    if (!transformed) {
        LOG("VOICE_SELF_TEST status=FAIL transform=0");
        return false;
    }

    LOG("VOICE_SELF_TEST status=%s test_rate=48000 samples=%u "
        "bin8_hz=375 c8=(%.9g,%.9g) bin16_hz=750 c16=(%.9g,%.9g)",
        passed ? "PASS" : "FAIL", SAMPLE_COUNT,
        coefficients[8].real, coefficients[8].imaginary,
        coefficients[16].real, coefficients[16].imaginary);
    return passed;
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

static void strongest_positive_bins(
    const struct complex_value *coefficients,
    struct spectral_peak peaks[PEAK_COUNT])
{
    for (size_t index = 0U; index < PEAK_COUNT; ++index)
        peaks[index] = (struct spectral_peak){0U, -1.0};

    for (size_t bin = 1U; bin < SAMPLE_COUNT / 2U; ++bin) {
        double magnitude = hypot(
            coefficients[bin].real, coefficients[bin].imaginary);
        for (size_t position = 0U; position < PEAK_COUNT; ++position) {
            if (magnitude <= peaks[position].magnitude) continue;
            for (size_t shift = PEAK_COUNT - 1U; shift > position; --shift)
                peaks[shift] = peaks[shift - 1U];
            peaks[position] = (struct spectral_peak){bin, magnitude};
            break;
        }
    }
}

static void log_spectrum(
    const struct application *a,
    const struct complex_value *coefficients,
    double input_rms, double framed_rms, uint64_t frame_number)
{
    struct spectral_peak peaks[PEAK_COUNT];
    strongest_positive_bins(coefficients, peaks);

    double frequency[PEAK_COUNT];
    double phase[PEAK_COUNT];
    for (size_t index = 0U; index < PEAK_COUNT; ++index) {
        frequency[index] =
            (double)a->properties.sample_rate *
            (double)peaks[index].bin / (double)SAMPLE_COUNT;
        phase[index] = atan2(
            coefficients[peaks[index].bin].imaginary,
            coefficients[peaks[index].bin].real);
    }

    LOG("VOICE_SPECTRUM frame=%" PRIu64
        " input_rms=%.7g framed_rms=%.7g "
        "p1=%zu:%.3fHz:%.7g:%.5frad "
        "p2=%zu:%.3fHz:%.7g:%.5frad "
        "p3=%zu:%.3fHz:%.7g:%.5frad "
        "p4=%zu:%.3fHz:%.7g:%.5frad "
        "p5=%zu:%.3fHz:%.7g:%.5frad",
        frame_number, input_rms, framed_rms,
        peaks[0].bin, frequency[0], peaks[0].magnitude, phase[0],
        peaks[1].bin, frequency[1], peaks[1].magnitude, phase[1],
        peaks[2].bin, frequency[2], peaks[2].magnitude, phase[2],
        peaks[3].bin, frequency[3], peaks[3].magnitude, phase[3],
        peaks[4].bin, frequency[4], peaks[4].magnitude, phase[4]);
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

    struct complex_value coefficients[SAMPLE_COUNT];
    if (!fourier_fft_real_radix2(
            samples, SAMPLE_COUNT, coefficients, SAMPLE_COUNT))
        return false;
    coefficients[0] = (struct complex_value){0.0, 0.0};

    uint64_t frame_number = a->frame_number + 1U;
    log_spectrum(a, coefficients, input_rms, framed_rms, frame_number);

    struct rgb24 pixels[RENDER_PIXELS];
    for (size_t row = 0U; row < RENDER_HEIGHT; ++row) {
        double y = FIELD_Y_RADIUS -
            2.0 * FIELD_Y_RADIUS * (double)row /
            (double)(RENDER_HEIGHT - 1U);
        for (size_t column = 0U; column < RENDER_WIDTH; ++column) {
            double x = -FIELD_X_RADIUS +
                2.0 * FIELD_X_RADIUS * (double)column /
                (double)(RENDER_WIDTH - 1U);
            struct complex_value value = fourier_polynomial_value(
                coefficients, SAMPLE_COUNT, TERM_COUNT,
                (struct complex_value){x, y});
            size_t pixel = row * RENDER_WIDTH + column;
            if (!wegert_color_complex(value, &pixels[pixel])) return false;
        }
    }

    enum native_window_output_result result = android_window_present_rgb24(
        a->app->window, pixels, RENDER_WIDTH, RENDER_HEIGHT, RENDER_PIXELS);
    if (result != NATIVE_WINDOW_OUTPUT_OK) {
        LOG("VOICE_DISPLAY_ERROR %s", native_window_output_result_text(result));
        return false;
    }

    a->frame_number = frame_number;
    LOG("VOICE_FRAME number=%" PRIu64
        " rate=%u samples=%u window_ms=%.3f terms=%u size=%ux%u "
        "field_x=%.2f field_y=%.2f field_max_radius=%.6f",
        a->frame_number, a->properties.sample_rate, SAMPLE_COUNT,
        1000.0 * (double)SAMPLE_COUNT / (double)a->properties.sample_rate,
        TERM_COUNT, RENDER_WIDTH, RENDER_HEIGHT,
        FIELD_X_RADIUS, FIELD_Y_RADIUS,
        hypot(FIELD_X_RADIUS, FIELD_Y_RADIUS));
    return true;
}

static void start_if_ready(struct application *a)
{
    if (!a->resumed || !a->focused || !a->window_ready || a->input) return;
    if (!microphone_permission(a->app->activity)) {
        LOG("VOICE_PERMISSION microphone permission required");
        return;
    }

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
        "window_ms=%.3f render=%ux%u interval_ms=%d terms=%u "
        "field_max_radius=%.6f noise_gate=none",
        a->properties.sample_rate, a->properties.channels,
        a->properties.format == AUDIO_FLOAT32 ? "float32" : "signed16",
        SAMPLE_COUNT,
        1000.0 * (double)SAMPLE_COUNT / (double)a->properties.sample_rate,
        RENDER_WIDTH, RENDER_HEIGHT, FRAME_INTERVAL_MS, TERM_COUNT,
        hypot(FIELD_X_RADIUS, FIELD_Y_RADIUS));
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

    LOG("VOICE_APP started");
    (void)coefficient_self_test();

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
    LOG("VOICE_APP stopped");
}
