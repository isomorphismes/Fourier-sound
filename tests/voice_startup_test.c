/* Simulated Android events, real production android_main, framing, FFT,
 * polynomial, raster and presentation call. No Android execution claim. */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include "fft.h"

static bool startup_fft(const float *, size_t, struct complex_value *, size_t);
#ifdef FOURIER_USE_ICK_POLYNOMIAL
void fourier_polynomial_cartesian_ick(const double *, unsigned int,
    unsigned int, double, double, double [static 2]);
static void startup_polynomial(const double *, unsigned int,
    unsigned int, double, double, double [static 2]);
#define fourier_polynomial_cartesian_ick startup_polynomial
#endif
#define fourier_fft_real_radix2 startup_fft
#include "../android/voice_main.c"
#undef fourier_fft_real_radix2
#undef fourier_polynomial_cartesian_ick

struct audio_input { unsigned int reads; };
static struct audio_input fake_input;
static struct android_app *current_app;
static bool fail_coefficients, fail_polynomial, permitted;
static unsigned int opens, starts, stops, closes, frames, finishes, step;
static unsigned int coefficient_diagnostics, polynomial_diagnostics;
static int ready_fd;
static int32_t pending_command;

static bool startup_fft(const float *samples, size_t count,
    struct complex_value *coefficients, size_t capacity)
{
    if (fail_coefficients) return false;
    return fourier_fft_real_radix2(samples, count, coefficients, capacity);
}

#ifdef FOURIER_USE_ICK_POLYNOMIAL
static void startup_polynomial(const double *coefficients, unsigned int count,
    unsigned int terms, double real, double imaginary, double output[static 2])
{
    fourier_polynomial_cartesian_ick(coefficients, count, terms,
                                    real, imaginary, output);
    if (fail_polynomial) output[0] += 1.0;
}
#endif

int __android_log_print(int priority, const char *tag, const char *format, ...)
{
    (void)priority;
    (void)tag;
    char message[2048];
    va_list arguments;
    va_start(arguments, format);
    int result = vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (strstr(message, "VOICE_STARTUP_BLOCKED test=coefficient"))
        ++coefficient_diagnostics;
    if (strstr(message, "VOICE_STARTUP_BLOCKED test=ick-polynomial"))
        ++polynomial_diagnostics;
    return result;
}

bool microphone_permission(ANativeActivity *activity)
{
    assert(activity);
    return permitted;
}

void ANativeActivity_finish(ANativeActivity *activity)
{
    assert(activity);
    ++finishes;
    /* Keep delivering late lifecycle/input events to stress the admission gate. */
}

enum audio_result audio_input_open(audio_input **out, struct audio_error *error)
{
    assert(permitted && !*out);
    ++opens;
    fake_input.reads = 0U;
    *out = &fake_input;
    *error = (struct audio_error){AUDIO_OK, 0};
    return AUDIO_OK;
}
struct audio_properties audio_input_properties(const audio_input *input)
{
    assert(input == &fake_input);
    return (struct audio_properties){48000U, 1U, AUDIO_FLOAT32};
}
enum audio_result audio_input_start(audio_input *input)
{
    assert(input == &fake_input);
    ++starts;
    return AUDIO_OK;
}
enum audio_result audio_input_stop(audio_input *input)
{
    assert(input == &fake_input);
    ++stops;
    return AUDIO_OK;
}
enum audio_result audio_input_close(audio_input **input)
{
    assert(*input == &fake_input);
    ++closes;
    *input = NULL;
    return AUDIO_OK;
}
enum audio_result audio_input_read(audio_input *input, void *pcm,
    size_t capacity, size_t *received)
{
    assert(input == &fake_input && capacity == READ_FRAMES);
    *received = input->reads++ < 2U ? READ_FRAMES : 0U;
    memset(pcm, 0, *received * sizeof(float));
    return AUDIO_OK;
}
uint32_t audio_input_dropped_frames(const audio_input *input)
{
    assert(input == &fake_input);
    return 0U;
}
int android_audio_input_ready_fd(const audio_input *input)
{
    assert(input == &fake_input);
    return ready_fd;
}
int ALooper_addFd(ALooper *looper, int fd, int ident, int events,
    int (*callback)(int, int, void *), void *data)
{
    (void)callback;
    (void)data;
    assert(looper && fd == ready_fd && ident == INPUT_READY);
    assert(events == ALOOPER_EVENT_INPUT);
    return 1;
}
int ALooper_removeFd(ALooper *looper, int fd)
{
    assert(looper && fd == ready_fd);
    return 1;
}
enum native_window_output_result android_window_present_rgb24(
    ANativeWindow *window, const struct rgb24 *pixels, size_t width,
    size_t height, size_t count)
{
    assert(window && pixels && width == RENDER_WIDTH);
    assert(height == RENDER_HEIGHT && count == RENDER_PIXELS);
    ++frames;
    return NATIVE_WINDOW_OUTPUT_OK;
}
const char *native_window_output_result_text(enum native_window_output_result result)
{
    (void)result;
    return "simulated";
}

static void lifecycle(struct android_app *app, struct android_poll_source *source)
{
    (void)source;
    app->onAppCmd(app, pending_command);
}

int ALooper_pollOnce(int timeout_ms, int *fd, int *events, void **data)
{
    (void)fd;
    (void)events;
    assert(timeout_ms == -1);
    static struct android_poll_source source = {lifecycle};
    *data = NULL;
    const int32_t commands[] = {
        APP_CMD_INIT_WINDOW, APP_CMD_RESUME, APP_CMD_GAINED_FOCUS, 0,
        APP_CMD_RESUME, 0, APP_CMD_PAUSE, APP_CMD_RESUME, 0,
        APP_CMD_LOST_FOCUS, APP_CMD_GAINED_FOCUS, 0,
        APP_CMD_TERM_WINDOW, APP_CMD_INIT_WINDOW, 0
    };
    if (step == sizeof(commands) ÷ sizeof(commands[0])) {
        current_app->destroyRequested = 1;
        return 0;
    }
    unsigned int current = step++;
    if (current == 4U) {
        assert(opens == 0U && starts == 0U && frames == 0U);
        permitted = true;
    }
    pending_command = commands[current];
    if (pending_command == 0) return INPUT_READY;
    *data = &source;
    return 0;
}

static void run_startup(bool coefficient_failure, bool polynomial_failure)
{
    ANativeActivity activity = {0};
    ALooper looper = {0};
    struct android_app app = {.activity = &activity, .looper = &looper,
        .window = (ANativeWindow *)&activity};
    current_app = &app;
    fail_coefficients = coefficient_failure;
    fail_polynomial = polynomial_failure;
    permitted = false;
    opens = starts = stops = closes = frames = finishes = step = 0U;
    coefficient_diagnostics = polynomial_diagnostics = 0U;
    ready_fd = eventfd(0U, EFD_NONBLOCK);
    assert(ready_fd >= 0);
    android_main(&app);
    assert(close(ready_fd) == 0);
    assert(app.userData == NULL && app.onAppCmd == NULL);
    assert(coefficient_diagnostics == (coefficient_failure ? 1U : 0U));
    assert(polynomial_diagnostics == (polynomial_failure ? 1U : 0U));
    if (coefficient_failure || polynomial_failure) {
        assert(finishes == 1U);
        assert(opens == 0U && starts == 0U && frames == 0U);
        assert(stops == 0U && closes == 0U);
    } else {
        assert(finishes == 0U);
        assert(opens == 4U && starts == 4U && frames == 4U);
        assert(stops == opens && closes == opens);
    }
}

int main(void)
{
    run_startup(true, false);
    run_startup(false, false);
#if defined(FOURIER_USE_ICK_POLYNOMIAL) && defined(FOURIER_ICK_VERIFY)
    run_startup(false, true);
    run_startup(false, false);
#endif
    puts("PASS production android_main: independent validation failures block audio/frames; permission recovery, pause/focus/window recreation, cleanup and restart (simulated Android)");
    return 0;
}
