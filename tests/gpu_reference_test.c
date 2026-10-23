/* The shipping pipeline and shaders execute on a surfaceless host driver.
 * Diagnostic readback is compiled only in this test, never in the APK. */
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <GLES3/gl31.h>
#include "gpu_voice_pipeline.h"
#include "dft.h"
#include "framing.h"
#include "complex_field.h"
#include "wegert.h"

#define COUNT 1024U
#define WIDTH 32U
#define HEIGHT 64U

int __android_log_print(int priority, const char *tag, const char *format, ...)
{
    (void)priority;
    printf("%s ", tag);
    va_list arguments;
    va_start(arguments, format);
    int result = vprintf(format, arguments);
    va_end(arguments);
    putchar('\n');
    return result;
}
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *window,
    int32_t width, int32_t height, int32_t format)
{
    (void)window; (void)width; (void)height; (void)format;
    abort(); /* The host test must use its diagnostic pbuffer. */
}

static void check(struct gpu_voice_pipeline *pipeline, unsigned int fixture)
{
    const double tau = 6.283185307179586476925286766559;
    float samples[COUNT];
    uint32_t random = 0x51a7U;
    for (size_t n = 0U; n < COUNT; ++n) {
        double phase = tau * (double)n ÷ (double)COUNT;
        random = random * 1664525U + 1013904223U;
        switch (fixture) {
        case 0: samples[n] = 0.0f; break;
        case 1: samples[n] = 0.25f; break;
        case 2: samples[n] = n == 17U ? 1.0f : 0.0f; break;
        case 3: samples[n] = (float)(0.5 * cos(8.0 * phase)); break;
        case 4: samples[n] = (float)(0.25 * sin(16.0 * phase)); break;
        case 5: samples[n] = (float)(0.2 + 0.5 * cos(phase) + 0.25 * sin(3.0 * phase)); break;
        default: samples[n] = (float)((double)(random >> 8U) ÷ 16777216.0 - 0.5); break;
        }
    }
    if (fixture == 7U) {
        assert(fourier_frame_remove_mean(samples, COUNT));
        assert(fourier_frame_apply_hann_periodic(samples, COUNT));
    }
    struct complex_value reference[COUNT];
    assert(fourier_dft_real(samples, COUNT, reference, COUNT));
    assert(gpu_voice_pipeline_render(pipeline, samples, COUNT) == GPU_VOICE_OK);
    float pairs[COUNT * 2U];
    assert(gpu_voice_pipeline_read_coefficients(pipeline, pairs, COUNT * 2U));
    double max_error = 0.0;
    for (size_t bin = 0U; bin < COUNT; ++bin) {
        double error = hypot((double)pairs[bin * 2U] - reference[bin].real,
            (double)pairs[bin * 2U + 1U] - reference[bin].imaginary);
        if (error > max_error) max_error = error;
        assert(error < 2.0e-6);
    }
    unsigned char rgba[WIDTH * HEIGHT * 4U];
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    assert(glGetError() == GL_NO_ERROR);
    reference[0] = (struct complex_value){0.0, 0.0};
    size_t compared = 0U, excluded = 0U;
    unsigned int max_channel_error = 0U;
    for (size_t row = 0U; row < HEIGHT; ++row) {
        for (size_t column = 0U; column < WIDTH; ++column) {
            /* OpenGL readback row zero is the bottom; CPU raster row zero
             * is the top. Both include the rectangle's endpoints. */
            struct complex_value z = {
                -0.44 + 0.88 * (double)column ÷ (double)(WIDTH - 1U),
                -0.88 + 1.76 * (double)row ÷ (double)(HEIGHT - 1U)};
            struct complex_value value = fourier_polynomial_value(reference, COUNT, 24U, z);
            double magnitude = hypot(value.real, value.imaginary);
            double hue = atan2(value.imaginary, value.real) * 360.0 ÷ tau;
            if (hue < 0.0) hue += 360.0;
            double band = magnitude > 1.0e-12 ? log10(magnitude) : 0.0;
            if ((magnitude > 0.0 && magnitude < 5.0e-6) ||
                (magnitude > 1.0e-12 && fabs(band - nearbyint(band)) < 0.002) ||
                (magnitude > 1.0e-12 && fabs(hue ÷ 100.0 - nearbyint(hue ÷ 100.0)) < 0.002)) {
                ++excluded;
                continue;
            }
            struct rgb24 rgb;
            assert(wegert_color_complex(value, &rgb));
            const unsigned char expected[] = {rgb.red, rgb.green, rgb.blue};
            for (size_t channel = 0U; channel < 3U; ++channel) {
                int difference = (int)rgba[(row * WIDTH + column) * 4U + channel] - expected[channel];
                unsigned int error = (unsigned int)abs(difference);
                if (error > max_channel_error) max_channel_error = error;
            }
            ++compared;
        }
    }
    printf("HOST_TESTED fixture=%u coefficient_max_error=%.9g color_max_byte_error=%u compared=%zu boundary_or_small_value_exclusions=%zu\n",
           fixture, max_error, max_channel_error, compared, excluded);
    assert(max_channel_error <= 2U);
    if (fixture == 0U || fixture == 2U || fixture >= 5U) assert(compared > 100U);
}

int main(void)
{
    struct gpu_voice_pipeline *pipeline = gpu_voice_pipeline_create(NULL, COUNT, 24U, WIDTH, HEIGHT);
    assert(pipeline);
    for (unsigned int fixture = 0U; fixture < 8U; ++fixture) check(pipeline, fixture);
    gpu_voice_pipeline_destroy(&pipeline);
    assert(!pipeline);
    puts("PASS host driver FFT/SSBO/polynomial/Wegert comparison; physical PowerVR/Mali execution NOT_RUN");
    return 0;
}
