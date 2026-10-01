#include "complex_field.h"
#include "fft.h"
#include "framing.h"
#include "pcm_block.h"
#include "ppm.h"
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
#define TERMS 64U
#define IMAGE_SIDE 128U
#define IMAGE_PIXELS (IMAGE_SIDE * IMAGE_SIDE)

struct clip {
    int16_t *samples;
    size_t frames;
};

static struct clip read_s16le(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror(path);
        exit(2);
    }
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
            ? (int32_t)word
            : (int32_t)word - 65536;
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
    assert(produced == WINDOW);
    assert(speaker_input_finished(&input));

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
        if (energy[k] > peak) {
            peak = energy[k];
            peak_index = k;
        }
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

static char *render_fixture(const char *source_path,
                            const struct complex_value *coefficients)
{
    struct complex_value *values =
        malloc(IMAGE_PIXELS * sizeof(*values));
    struct rgb24 *pixels = malloc(IMAGE_PIXELS * sizeof(*pixels));
    assert(values && pixels);

    for (size_t row = 0U; row < IMAGE_SIDE; ++row) {
        double y = 0.7 - 1.4 * (double)row / (double)(IMAGE_SIDE - 1U);
        for (size_t column = 0U; column < IMAGE_SIDE; ++column) {
            double x = -0.7 +
                1.4 * (double)column / (double)(IMAGE_SIDE - 1U);
            size_t index = row * IMAGE_SIDE + column;
            values[index] = fourier_polynomial_value(
                coefficients, WINDOW, TERMS,
                (struct complex_value){x, y});
            assert(isfinite(values[index].real));
            assert(isfinite(values[index].imaginary));
        }
    }

    assert(wegert_color_values(values, IMAGE_PIXELS,
                               pixels, IMAGE_PIXELS));

    size_t different = 0U;
    for (size_t index = 1U; index < IMAGE_PIXELS; ++index)
        if (pixels[index].red != pixels[0].red ||
            pixels[index].green != pixels[0].green ||
            pixels[index].blue != pixels[0].blue)
            ++different;
    assert(different > IMAGE_PIXELS / 10U);

    size_t path_length = strlen(source_path) + 5U;
    char *output_path = malloc(path_length);
    assert(output_path);
    assert(snprintf(output_path, path_length, "%s.ppm", source_path) > 0);
    assert(rgb24_write_ppm(output_path, pixels,
                           IMAGE_SIDE, IMAGE_SIDE, IMAGE_PIXELS));

    free(pixels);
    free(values);
    return output_path;
}

static void test_fixture(const char *path)
{
    struct clip clip = read_s16le(path);
    assert(clip.frames >= WINDOW * 4U);
    assert(clip.frames <= SIZE_MAX / sizeof(float));

    float *mono = malloc(clip.frames * sizeof(*mono));
    assert(mono);

    struct audio_properties source = {SAMPLE_RATE, 1, AUDIO_SIGNED16};
    assert(fourier_pcm_mono(source, clip.samples, clip.frames,
                            mono, clip.frames));

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
    for (size_t offset = 0U;
         offset + WINDOW <= clip.frames;
         offset += WINDOW) {
        double rms = 0.0;
        assert(fourier_frame_copy(mono, clip.frames, offset,
                                  WINDOW, block, WINDOW));
        assert(fourier_frame_rms(block, WINDOW, &rms));
        if (rms > best_rms) {
            best_rms = rms;
            best_offset = offset;
        }
    }

    double overall_rms = sqrt(total_square / (double)clip.frames);
    assert(isfinite(overall_rms) && overall_rms > 0.0);
    assert(best_rms > 0.0);
    assert(nonzero > clip.frames / 100U);

    float loudest[WINDOW];
    float analysis[WINDOW];
    assert(fourier_frame_copy(mono, clip.frames, best_offset,
                              WINDOW, loudest, WINDOW));
    assert(fourier_frame_copy(mono, clip.frames, best_offset,
                              WINDOW, analysis, WINDOW));

    speaker_roundtrip(loudest);

    assert(fourier_frame_remove_mean(analysis, WINDOW));
    assert(fourier_frame_apply_hann(analysis, WINDOW));

    struct complex_value coefficients[WINDOW];
    assert(fourier_fft_real_radix2(analysis, WINDOW,
                                   coefficients, WINDOW));

    size_t peak_bin = 0U;
    double peak_energy = 0.0;
    size_t occupied =
        spectrum_summary(coefficients, &peak_bin, &peak_energy);
    assert(occupied >= 2U);

    char *image_path = render_fixture(path, coefficients);

    double peak_hz = (double)peak_bin * SAMPLE_RATE / WINDOW;
    printf("PASS media %s frames=%zu rms=%.6f loudest=%.6f "
           "peak=%.1fHz bins=%zu energy=%.8g render=%s\n",
           path, clip.frames, overall_rms, best_rms, peak_hz,
           occupied, peak_energy, image_path);

    free(image_path);
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
