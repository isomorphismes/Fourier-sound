#include "cylinder_static.h"

#include "complex_field.h"
#include "fft.h"
#include "framing.h"
#include "pcm_block.h"
#include "ppm.h"
#include "wegert.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100U
#define WINDOW 4096U
#define QUIET_FRAMES 10U
#define TERMS 64U
#define IMAGE_SIDE 256U
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

static size_t loudest_frame(const float *mono, size_t count)
{
    float frame[WINDOW];
    double best_rms = -1.0;
    size_t best_offset = 0U;

    for (size_t offset = 0U; offset + WINDOW <= count; offset += WINDOW) {
        double rms = 0.0;
        assert(fourier_frame_copy(mono, count, offset,
                                  WINDOW, frame, WINDOW));
        assert(fourier_frame_rms(frame, WINDOW, &rms));
        if (rms > best_rms) {
            best_rms = rms;
            best_offset = offset;
        }
    }
    return best_offset;
}

static void analyze_frame(const float *mono, size_t count, size_t offset,
                          struct complex_value *coefficients)
{
    float frame[WINDOW];
    assert(fourier_frame_copy(mono, count, offset,
                              WINDOW, frame, WINDOW));
    assert(fourier_frame_remove_mean(frame, WINDOW));
    assert(fourier_frame_apply_hann_periodic(frame, WINDOW));
    assert(fourier_fft_real_radix2(frame, WINDOW,
                                   coefficients, WINDOW));
}

static void make_path(char *path, size_t capacity,
                      const char *prefix, const char *suffix)
{
    int written = snprintf(path, capacity, "%s%s", prefix, suffix);
    assert(written > 0 && (size_t)written < capacity);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s nightingale.s16 output-prefix\n", argv[0]);
        return 2;
    }

    struct clip clip = read_s16le(argv[1]);
    float *mono = malloc(clip.frames * sizeof(*mono));
    assert(mono);
    struct audio_properties source = {SAMPLE_RATE, 1, AUDIO_SIGNED16};
    assert(fourier_pcm_mono(source, clip.samples, clip.frames,
                            mono, clip.frames));

    double noise_power[WINDOW];
    assert(fourier_noise_power_from_quiet_frames(
        mono, clip.frames, WINDOW, QUIET_FRAMES,
        noise_power, WINDOW));

    size_t target_offset = loudest_frame(mono, clip.frames);
    struct complex_value coefficients[WINDOW];
    analyze_frame(mono, clip.frames, target_offset, coefficients);

    struct complex_value candidates[WINDOW];
    double weights[WINDOW];
    assert(fourier_noise_candidate_coefficients(
        coefficients, noise_power, WINDOW,
        candidates, WINDOW, weights, WINDOW));

    size_t strong_candidates = 0U;
    for (size_t k = 0U; k < TERMS; ++k)
        if (weights[k] >= 0.5) ++strong_candidates;
    assert(strong_candidates > 0U);

    struct rgb24 *base = malloc(IMAGE_PIXELS * sizeof(*base));
    struct rgb24 *overlay = malloc(IMAGE_PIXELS * sizeof(*overlay));
    assert(base && overlay);

    const struct rgb24 static_color = {20U, 20U, 24U};
    double alpha_sum = 0.0;
    double alpha_max = 0.0;

    for (size_t row = 0U; row < IMAGE_SIDE; ++row) {
        double y = 0.7 - 1.4 * (double)row / (double)(IMAGE_SIDE - 1U);
        for (size_t column = 0U; column < IMAGE_SIDE; ++column) {
            double x = -0.7 +
                1.4 * (double)column / (double)(IMAGE_SIDE - 1U);
            size_t index = row * IMAGE_SIDE + column;
            struct complex_value z = {x, y};

            struct complex_value full = fourier_polynomial_value(
                coefficients, WINDOW, TERMS, z);
            struct complex_value candidate = fourier_polynomial_value(
                candidates, WINDOW, TERMS, z);

            assert(wegert_color_complex(full, &base[index]));
            double full_magnitude = hypot(full.real, full.imaginary);
            double candidate_magnitude =
                hypot(candidate.real, candidate.imaginary);
            double fraction = candidate_magnitude /
                (full_magnitude + candidate_magnitude + 1.0e-12);

            /*
             * A low candidate fraction is present almost everywhere because
             * a polynomial term has global support.  Showing that as opacity
             * merely tints the whole Wegert image.  Reserve the annotation
             * for places where the candidate-static field is at least 14% of
             * the combined local magnitude, then expand the remaining range.
             * The cutoff is deliberately visual: it suppresses the nearly
             * uniform global tint while preserving the localized high-ratio
             * regions of this particular candidate field.
             */
            double alpha = 0.0;
            if (fraction > 0.14) {
                double visible = (fraction - 0.14) / 0.86;
                alpha = 0.80 * sqrt(visible);
                if (alpha > 0.80) alpha = 0.80;
            }

            overlay[index] =
                rgb24_overlay(base[index], static_color, alpha);
            alpha_sum += alpha;
            if (alpha > alpha_max) alpha_max = alpha;
        }
    }

    assert(alpha_max > 0.0);
    assert(alpha_sum > 0.0);

    char baseline_path[1024];
    char overlay_path[1024];
    make_path(baseline_path, sizeof(baseline_path),
              argv[2], "-baseline.ppm");
    make_path(overlay_path, sizeof(overlay_path),
              argv[2], "-static-overlay.ppm");

    assert(rgb24_write_ppm(baseline_path, base,
                           IMAGE_SIDE, IMAGE_SIDE, IMAGE_PIXELS));
    assert(rgb24_write_ppm(overlay_path, overlay,
                           IMAGE_SIDE, IMAGE_SIDE, IMAGE_PIXELS));

    printf("Nightingale target frame %.3f s; %zu/%u first terms >= 0.5 "
           "candidate-static weight; overlay alpha mean %.4f max %.4f\n",
           (double)target_offset / SAMPLE_RATE,
           strong_candidates, TERMS,
           alpha_sum / IMAGE_PIXELS, alpha_max);
    printf("baseline=%s\noverlay=%s\n",
           baseline_path, overlay_path);

    free(overlay);
    free(base);
    free(mono);
    free(clip.samples);
    return 0;
}
