#include "pcm_block.h"
#include "speaker_input.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100U
#define WINDOW 4096U

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
    for (size_t i = 0; i < frames; ++i) {
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

static double block_rms(const float *samples, size_t frames)
{
    double sum = 0.0;
    for (size_t i = 0; i < frames; ++i)
        sum += (double)samples[i] * samples[i];
    return sqrt(sum / (double)frames);
}

static size_t reference_spectrum(const float *samples, size_t frames,
                                 size_t *peak_bin, double *peak_energy)
{
    assert(frames == WINDOW);
    const double tau = 6.283185307179586476925286766559;
    double mean = 0.0;
    for (size_t i = 0; i < frames; ++i)
        mean += samples[i];
    mean /= (double)frames;

    double energy[WINDOW / 2U + 1U] = {0};
    double peak = 0.0;
    size_t peak_index = 0;
    for (size_t k = 1; k <= frames / 2U; ++k) {
        double real = 0.0;
        double imaginary = 0.0;
        for (size_t n = 0; n < frames; ++n) {
            double angle = tau * (double)k * (double)n / (double)frames;
            double value = (double)samples[n] - mean;
            real += value * cos(angle);
            imaginary -= value * sin(angle);
        }
        energy[k] = real * real + imaginary * imaginary;
        if (energy[k] > peak) {
            peak = energy[k];
            peak_index = k;
        }
    }

    assert(isfinite(peak) && peak > 0.0);
    size_t occupied = 0;
    double threshold = peak * 1e-5;
    for (size_t k = 1; k <= frames / 2U; ++k)
        if (energy[k] >= threshold) ++occupied;

    *peak_bin = peak_index;
    *peak_energy = peak;
    return occupied;
}

static void speaker_roundtrip(const float *source)
{
    struct speaker_input input = {0};
    assert(speaker_input_set(&input, source, WINDOW, false));

    struct audio_properties speaker = {SAMPLE_RATE, 2, AUDIO_SIGNED16};
    int16_t stereo[WINDOW * 2U] = {0};
    size_t produced = 0;
    assert(speaker_input_render(&input, speaker, stereo, WINDOW, &produced));
    assert(produced == WINDOW);
    assert(speaker_input_finished(&input));

    float recovered[WINDOW] = {0};
    assert(fourier_pcm_mono(speaker, stereo, WINDOW, recovered, WINDOW));
    for (size_t i = 0; i < WINDOW; ++i)
        assert(fabsf(recovered[i] - source[i]) <= 7e-5f);
}

static void test_fixture(const char *path)
{
    struct clip clip = read_s16le(path);
    assert(clip.frames >= WINDOW * 4U);

    struct audio_properties source = {SAMPLE_RATE, 1, AUDIO_SIGNED16};
    float block[WINDOW] = {0};
    float loudest[WINDOW] = {0};
    double best_rms = 0.0;
    double total_square = 0.0;
    size_t nonzero = 0;

    for (size_t i = 0; i < clip.frames; ++i) {
        double value = (double)clip.samples[i] / 32768.0;
        total_square += value * value;
        if (clip.samples[i] != 0) ++nonzero;
    }

    for (size_t offset = 0; offset + WINDOW <= clip.frames; offset += WINDOW) {
        assert(fourier_pcm_mono(source, clip.samples + offset,
                                WINDOW, block, WINDOW));
        double rms = block_rms(block, WINDOW);
        if (rms > best_rms) {
            best_rms = rms;
            for (size_t i = 0; i < WINDOW; ++i) loudest[i] = block[i];
        }
    }

    double overall_rms = sqrt(total_square / (double)clip.frames);
    assert(isfinite(overall_rms) && overall_rms > 0.0);
    assert(best_rms > 0.0);
    assert(nonzero > clip.frames / 100U);

    size_t peak_bin = 0;
    double peak_energy = 0.0;
    size_t occupied = reference_spectrum(loudest, WINDOW,
                                         &peak_bin, &peak_energy);
    assert(occupied >= 2U);
    speaker_roundtrip(loudest);

    double peak_hz = (double)peak_bin * SAMPLE_RATE / WINDOW;
    printf("PASS media %s frames=%zu rms=%.6f loudest=%.6f peak=%.1fHz bins=%zu energy=%.3f\n",
           path, clip.frames, overall_rms, best_rms, peak_hz, occupied,
           peak_energy);
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
