#include "complex_field.h"
#include "fft.h"
#include "framing.h"
#include "pcm_block.h"
#include "ppm.h"
#include "sparse_series.h"
#include "speaker_input.h"
#include "wegert.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100U
#define WINDOW 4096U
#define TERMS 12U
#define IMAGE_SIDE 128U
#define IMAGE_PIXELS (IMAGE_SIDE * IMAGE_SIDE)

static const size_t dyadic_exponents[TERMS] = {
    0U, 1U, 2U, 4U, 8U, 16U,
    32U, 64U, 128U, 256U, 512U, 1024U
};

struct clip { int16_t *samples; size_t frames; };

static struct clip read_s16le(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) { perror(path); exit(2); }
    assert(fseek(file, 0, SEEK_END) == 0);
    long end = ftell(file);
    assert(end > 0 && (end % 2) == 0);
    assert(fseek(file, 0, SEEK_SET) == 0);

    size_t bytes_count = (size_t)end;
    unsigned char *bytes = malloc(bytes_count);
    assert(bytes);
    assert(fread(bytes, 1, bytes_count, file) == bytes_count);
    assert(fclose(file) == 0);

    size_t frames = bytes_count / 2U;
    int16_t *samples = malloc(frames * sizeof(*samples));
    assert(samples);
    for (size_t i = 0U; i < frames; ++i) {
        uint16_t word = (uint16_t)bytes[2U * i] |
                        (uint16_t)((uint16_t)bytes[2U * i + 1U] << 8U);
        int32_t signed_word = word <= INT16_MAX
            ? (int32_t)word : (int32_t)word - 65536;
        samples[i] = (int16_t)signed_word;
    }
    free(bytes);
    return (struct clip){samples, frames};
}

static void speaker_roundtrip(const float *source)
{
    struct speaker_input input = {0};
    assert(speaker_input_set(&input, source, WINDOW, false));
    struct audio_properties speaker = {SAMPLE_RATE, 2, AUDIO_SIGNED16};
    int16_t stereo[WINDOW * 2U] = {0};
    size_t produced = 0U;
    assert(speaker_input_render(&input, speaker, stereo, WINDOW, &produced));
    assert(produced == WINDOW && speaker_input_finished(&input));

    float recovered[WINDOW] = {0};
    assert(fourier_pcm_mono(speaker, stereo, WINDOW, recovered, WINDOW));
    for (size_t i = 0U; i < WINDOW; ++i)
        assert(fabsf(recovered[i] - source[i]) <= 7e-5f);
}

static size_t spectrum_summary(const struct complex_value *coefficients,
                               size_t *peak_bin, double *peak_energy)
{
    double peak = 0.0;
    size_t peak_index = 0U;
    double energy[WINDOW / 2U + 1U] = {0};
    for (size_t k = 1U; k <= WINDOW / 2U; ++k) {
        double real = coefficients[k].real;
        double imaginary = coefficients[k].imaginary;
        energy[k] = real * real + imaginary * imaginary;
        if (energy[k] > peak) { peak = energy[k]; peak_index = k; }
    }
    assert(isfinite(peak) && peak > 0.0);
    size_t occupied = 0U;
    double threshold = peak * 1e-5;
    for (size_t k = 1U; k <= WINDOW / 2U; ++k)
        if (energy[k] >= threshold) ++occupied;
    *peak_bin = peak_index;
    *peak_energy = peak;
    return occupied;
}

static char *output_path(const char *source_path, const char *suffix)
{
    size_t length = strlen(source_path) + strlen(suffix) + 1U;
    char *path = malloc(length);
    assert(path);
    int written = snprintf(path, length, "%s%s", source_path, suffix);
    assert(written >= 0 && (size_t)written < length);
    return path;
}

static size_t nonconstant_pixels(const struct rgb24 *pixels)
{
    size_t different = 0U;
    for (size_t i = 1U; i < IMAGE_PIXELS; ++i)
        if (pixels[i].red != pixels[0].red ||
            pixels[i].green != pixels[0].green ||
            pixels[i].blue != pixels[0].blue) ++different;
    return different;
}

static size_t compare_constructions(const char *source_path,
                                    const struct complex_value *coefficients,
                                    char **dense_path, char **dyadic_path,
                                    double *mean_rgb_difference)
{
    struct rgb24 *dense = malloc(IMAGE_PIXELS * sizeof(*dense));
    struct rgb24 *dyadic = malloc(IMAGE_PIXELS * sizeof(*dyadic));
    assert(dense && dyadic);

    for (size_t row = 0U; row < IMAGE_SIDE; ++row) {
        double y = 0.7 - 1.4 * (double)row / (double)(IMAGE_SIDE - 1U);
        for (size_t column = 0U; column < IMAGE_SIDE; ++column) {
            double x = -0.7 + 1.4 * (double)column / (double)(IMAGE_SIDE - 1U);
            size_t index = row * IMAGE_SIDE + column;
            struct complex_value q = {x, y};

            struct complex_value dense_value = fourier_polynomial_value(
                coefficients, WINDOW, TERMS, q);
            struct complex_value dyadic_value;
            assert(fourier_sparse_series_value(
                coefficients, WINDOW, dyadic_exponents,
                TERMS, q, &dyadic_value));

            assert(isfinite(dense_value.real) && isfinite(dense_value.imaginary));
            assert(isfinite(dyadic_value.real) && isfinite(dyadic_value.imaginary));
            assert(wegert_color_complex(dense_value, &dense[index]));
            assert(wegert_color_complex(dyadic_value, &dyadic[index]));
        }
    }

    assert(nonconstant_pixels(dense) > IMAGE_PIXELS / 10U);
    assert(nonconstant_pixels(dyadic) > IMAGE_PIXELS / 10U);

    size_t changed = 0U;
    uint64_t absolute_difference = 0U;
    for (size_t i = 0U; i < IMAGE_PIXELS; ++i) {
        int red = (int)dense[i].red - (int)dyadic[i].red;
        int green = (int)dense[i].green - (int)dyadic[i].green;
        int blue = (int)dense[i].blue - (int)dyadic[i].blue;
        if (red < 0) red = -red;
        if (green < 0) green = -green;
        if (blue < 0) blue = -blue;
        absolute_difference += (uint64_t)(red + green + blue);

        if (red || green || blue) ++changed;
    }
    *mean_rgb_difference =
        (double)absolute_difference / (double)(IMAGE_PIXELS * 3U);
    assert(changed > IMAGE_PIXELS / 5U);
    assert(*mean_rgb_difference > 1.0);

    *dense_path = output_path(source_path, ".dense.ppm");
    *dyadic_path = output_path(source_path, ".dyadic.ppm");
    assert(rgb24_write_ppm(*dense_path, dense, IMAGE_SIDE, IMAGE_SIDE, IMAGE_PIXELS));
    assert(rgb24_write_ppm(*dyadic_path, dyadic, IMAGE_SIDE, IMAGE_SIDE, IMAGE_PIXELS));

    free(dyadic);
    free(dense);
    return changed;
}

static void test_fixture(const char *path)
{
    struct clip clip = read_s16le(path);
    assert(clip.frames >= WINDOW * 4U);
    assert(clip.frames <= SIZE_MAX / sizeof(float));

    float *mono = malloc(clip.frames * sizeof(*mono));
    assert(mono);
    struct audio_properties source = {SAMPLE_RATE, 1, AUDIO_SIGNED16};
    assert(fourier_pcm_mono(source, clip.samples, clip.frames, mono, clip.frames));

    double total_square = 0.0;
    size_t nonzero = 0U;
    for (size_t i = 0U; i < clip.frames; ++i) {
        double value = mono[i];
        total_square += value * value;
        if (clip.samples[i] != 0) ++nonzero;
    }

    float block[WINDOW];
    double best_rms = 0.0;
    size_t best_offset = 0U;
    for (size_t offset = 0U; offset + WINDOW <= clip.frames; offset += WINDOW) {
        double rms = 0.0;
        assert(fourier_frame_copy(mono, clip.frames, offset, WINDOW, block, WINDOW));
        assert(fourier_frame_rms(block, WINDOW, &rms));
        if (rms > best_rms) { best_rms = rms; best_offset = offset; }
    }

    double overall_rms = sqrt(total_square / (double)clip.frames);
    assert(isfinite(overall_rms) && overall_rms > 0.0);
    assert(best_rms > 0.0 && nonzero > clip.frames / 100U);

    float loudest[WINDOW];
    float analysis[WINDOW];
    assert(fourier_frame_copy(mono, clip.frames, best_offset, WINDOW, loudest, WINDOW));
    assert(fourier_frame_copy(mono, clip.frames, best_offset, WINDOW, analysis, WINDOW));
    speaker_roundtrip(loudest);

    assert(fourier_frame_remove_mean(analysis, WINDOW));
    assert(fourier_frame_apply_hann_periodic(analysis, WINDOW));

    struct complex_value coefficients[WINDOW];
    assert(fourier_fft_real_radix2(analysis, WINDOW, coefficients, WINDOW));

    size_t peak_bin = 0U;
    double peak_energy = 0.0;
    size_t occupied = spectrum_summary(coefficients, &peak_bin, &peak_energy);
    assert(occupied >= 2U);

    char *dense_path = NULL;
    char *dyadic_path = NULL;
    double mean_rgb_difference = 0.0;
    size_t changed = compare_constructions(
        path, coefficients, &dense_path, &dyadic_path, &mean_rgb_difference);

    double peak_hz = (double)peak_bin * SAMPLE_RATE / WINDOW;
    printf("PASS compare %s peak=%.1fHz bins=%zu changed=%zu/%u "
           "rgb_mae=%.3f dense=%s dyadic=%s\n",
           path, peak_hz, occupied, changed, IMAGE_PIXELS,
           mean_rgb_difference, dense_path, dyadic_path);

    free(dyadic_path);
    free(dense_path);
    free(mono);
    free(clip.samples);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s fixture.s16 [fixture.s16 ...]\n", argv[0]);
        return 2;
    }
    for (int i = 1; i < argc; ++i) test_fixture(argv[i]);
    return 0;
}
