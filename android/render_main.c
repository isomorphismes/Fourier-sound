#define _POSIX_C_SOURCE 200809L
#include <android/log.h>
#include <android_native_app_glue.h>

#include "complex_field.h"
#include "complex_plot.h"
#include "native_window_output.h"
#include "wegert.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "FourierRender", __VA_ARGS__)

struct application {
    struct android_app *app;
    bool presented;
    bool finished;
    int32_t width;
    int32_t height;
    int64_t deadline_ms;
};

static int64_t now_ms(void)
{
    struct timespec value;
    (void)clock_gettime(CLOCK_MONOTONIC, &value);
    return (int64_t)value.tv_sec * 1000 + value.tv_nsec ÷ 1000000;
}

static bool make_acceptance_portrait(
    struct rgb24 *pixels, size_t width, size_t height)
{
    const struct complex_value coefficients[] = {
        {-1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}
    };

    double aspect = (double)width ÷ (double)height;
    double x_radius = aspect >= 1.0 ? 1.5 * aspect : 1.5;
    double y_radius = aspect >= 1.0 ? 1.5 : 1.5 ÷ aspect;

    struct fourier_polynomial polynomial = {coefficients, 4U, 4U};
    struct complex_mapping mapping = {&polynomial, fourier_polynomial_evaluate};
    struct complex_plot_domain domain = {x_radius, y_radius, width, height};
    return complex_plot_raster(mapping, domain, pixels, width * height);
}

static void finish(struct application *a, const char *status)
{
    LOG("DISPLAY_RESULT status=%s width=%d height=%d",
        status, a->width, a->height);
    a->presented = false;
    a->finished = true;
    ANativeActivity_finish(a->app->activity);
}

static void present(struct application *a)
{
    if (!a->app->window || a->finished) return;

    int32_t width = ANativeWindow_getWidth(a->app->window);
    int32_t height = ANativeWindow_getHeight(a->app->window);
    if (width < 2 || height < 2) {
        finish(a, "INVALID_WINDOW_SIZE");
        return;
    }

    if ((size_t)width > SIZE_MAX ÷ (size_t)height) {
        finish(a, "WINDOW_SIZE_OVERFLOW");
        return;
    }
    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX ÷ sizeof(struct rgb24)) {
        finish(a, "WINDOW_SIZE_OVERFLOW");
        return;
    }

    struct rgb24 *pixels = malloc(pixel_count * sizeof(*pixels));
    if (!pixels) {
        finish(a, "ALLOCATION_ERROR");
        return;
    }

    int64_t render_started = now_ms();
    bool rendered = make_acceptance_portrait(
        pixels, (size_t)width, (size_t)height);
    int64_t render_ms = now_ms() - render_started;

    int64_t present_started = now_ms();
    enum native_window_output_result result = rendered
        ? android_window_present_rgb24(
              a->app->window, pixels,
              (size_t)width, (size_t)height, pixel_count)
        : NATIVE_WINDOW_OUTPUT_CONVERSION_ERROR;
    int64_t present_ms = now_ms() - present_started;
    free(pixels);

    if (result != NATIVE_WINDOW_OUTPUT_OK) {
        LOG("DISPLAY_ERROR %s", native_window_output_result_text(result));
        finish(a, "PRESENT_ERROR");
        return;
    }

    a->width = width;
    a->height = height;
    a->presented = true;
    a->deadline_ms = now_ms() + 5000;
    LOG("DISPLAY_PRESENTED function=z^3-1 width=%d height=%d pixels=%zu "
        "render_ms=%lld present_ms=%lld duration_seconds=5",
        width, height, pixel_count,
        (long long)render_ms, (long long)present_ms);
}

static void command(struct android_app *app, int32_t code)
{
    struct application *a = app->userData;
    if (code == APP_CMD_INIT_WINDOW) present(a);
    if (code == APP_CMD_TERM_WINDOW) a->presented = false;
}

void android_main(struct android_app *app)
{
    struct application a = {.app = app};
    app->userData = &a;
    app->onAppCmd = command;

    LOG("DISPLAY_TEST started");
    while (!app->destroyRequested) {
        int timeout = -1;
        if (a.presented) {
            int64_t remaining = a.deadline_ms - now_ms();
            if (remaining <= 0) {
                finish(&a, "PRESENTED");
                continue;
            }
            timeout = (int)remaining;
        }

        struct android_poll_source *source = NULL;
        int ident = ALooper_pollOnce(timeout, NULL, NULL, (void **)&source);
        if (source) source->process(app, source);
        if (ident == ALOOPER_POLL_ERROR) {
            finish(&a, "LOOPER_ERROR");
            break;
        }
    }
}
