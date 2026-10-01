#include "complex_field.h"
#include "dft.h"
#include "ppm.h"
#include "wegert.h"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 48000U
#define FPS 5U
#define SAMPLE_COUNT 128U
#define TERM_COUNT 24U
#define WIDTH 96U
#define HEIGHT 192U
#define PIXELS (WIDTH * HEIGHT)
#define SAMPLES_PER_FRAME (SAMPLE_RATE / FPS)

static int16_t *read_pcm(const char *path, size_t *sample_count)
{
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long bytes = ftell(file);
    if (bytes < 0 || (bytes % (long)sizeof(int16_t)) != 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);

    *sample_count = (size_t)bytes / sizeof(int16_t);
    if (!*sample_count) {
        fclose(file);
        return NULL;
    }

    int16_t *samples = malloc(*sample_count * sizeof(*samples));
    if (!samples) {
        fclose(file);
        return NULL;
    }
    if (fread(samples, sizeof(*samples), *sample_count, file) != *sample_count) {
        free(samples);
        fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(samples);
        return NULL;
    }
    return samples;
}

static void window_samples(const int16_t *pcm, size_t pcm_count, size_t end,
                           float *samples)
{
    if (end > pcm_count) end = pcm_count;
    size_t available = end < SAMPLE_COUNT ? end : SAMPLE_COUNT;
    size_t start = end - available;
    size_t pad = SAMPLE_COUNT - available;

    double mean = 0.0;
    for (size_t i = 0; i < pad; ++i) samples[i] = 0.0f;
    for (size_t i = 0; i < available; ++i) {
        float value = (float)pcm[start + i] / 32768.0f;
        samples[pad + i] = value;
        mean += value;
    }
    mean /= (double)SAMPLE_COUNT;

    const double tau = 6.283185307179586476925286766559;
    for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
        double hann = 0.5 - 0.5 * cos(
            tau * (double)i / (double)(SAMPLE_COUNT - 1U));
        samples[i] = (float)(((double)samples[i] - mean) * hann);
    }
}

static int render_frame(const int16_t *pcm, size_t pcm_count, size_t end,
                        const char *path)
{
    float samples[SAMPLE_COUNT];
    struct complex_value coefficients[SAMPLE_COUNT];
    struct rgb24 pixels[PIXELS];

    window_samples(pcm, pcm_count, end, samples);
    if (!fourier_dft_real(samples, SAMPLE_COUNT,
                          coefficients, SAMPLE_COUNT)) return 0;
    coefficients[0] = (struct complex_value){0.0, 0.0};

    const double x_radius = 1.5;
    const double y_radius = 3.0;
    for (size_t row = 0; row < HEIGHT; ++row) {
        double y = y_radius - 2.0 * y_radius * (double)row /
            (double)(HEIGHT - 1U);
        for (size_t column = 0; column < WIDTH; ++column) {
            double x = -x_radius + 2.0 * x_radius * (double)column /
                (double)(WIDTH - 1U);
            struct complex_value value = fourier_polynomial_value(
                coefficients, SAMPLE_COUNT, TERM_COUNT,
                (struct complex_value){x, y});
            if (!wegert_color_complex(value,
                    &pixels[row * WIDTH + column])) return 0;
        }
    }

    return rgb24_write_ppm(path, pixels, WIDTH, HEIGHT, PIXELS);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s clip.s16 frames-directory\n", argv[0]);
        return 2;
    }

    size_t pcm_count = 0;
    int16_t *pcm = read_pcm(argv[1], &pcm_count);
    if (!pcm) {
        fprintf(stderr, "cannot read %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    size_t frame_count =
        (pcm_count + SAMPLES_PER_FRAME - 1U) / SAMPLES_PER_FRAME;

    for (size_t frame = 0; frame < frame_count; ++frame) {
        size_t end = (frame + 1U) * SAMPLES_PER_FRAME;
        if (end > pcm_count) end = pcm_count;

        char path[4096];
        int length = snprintf(path, sizeof(path), "%s/frame-%06zu.ppm",
                              argv[2], frame);
        if (length < 0 || (size_t)length >= sizeof(path) ||
            !render_frame(pcm, pcm_count, end, path)) {
            fprintf(stderr, "failed to render frame %zu\n", frame);
            free(pcm);
            return 1;
        }

        if ((frame % 25U) == 0U || frame + 1U == frame_count)
            fprintf(stderr, "rendered %zu/%zu\n", frame + 1U, frame_count);
    }

    printf("frames=%zu fps=%u duration=%.6f internal=%ux%u samples=%u terms=%u\n",
           frame_count, FPS, (double)pcm_count / SAMPLE_RATE,
           WIDTH, HEIGHT, SAMPLE_COUNT, TERM_COUNT);
    free(pcm);
    return 0;
}
