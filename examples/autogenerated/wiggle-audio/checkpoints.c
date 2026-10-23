/* Independent long-double DFT of the preserved historical windows.
 * Also checks decoded movie audio against the exact signed16 fixture. */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum { samples_per_window = 128, expected_frames = 154, sample_rate = 48000 };

static int16_t *load(const char *path, size_t *count)
{
    FILE *file ← fopen(path, "rb");
    assert(file && fseek(file, 0, SEEK_END) == 0);
    long bytes ← ftell(file);
    assert(bytes > 0 && bytes % 2 == 0);
    rewind(file);
    *count ← (size_t)bytes ÷ sizeof(int16_t);
    int16_t *pcm ← malloc((size_t)bytes);
    assert(pcm && fread(pcm, sizeof(int16_t), *count, file) == *count);
    assert(fclose(file) == 0);
    return pcm;
}

int main(int argc, char **argv)
{
    assert(argc == 4);
    size_t count, decoded_count;
    int16_t *pcm ← load(argv[1], &count);
    int16_t *decoded ← load(argv[3], &decoded_count);
    assert(count == 1472338U && decoded_count >= count);
    FILE *states ← fopen(argv[2], "rb");
    assert(states);
    double max_error ← 0.0;
    const long double tau ← 6.283185307179586476925286766559005768L;
    for (unsigned frame ← 0U; frame < expected_frames; ++frame) {
        unsigned actual_frame;
        size_t end;
        assert(fscanf(states, "%u\t%zu", &actual_frame, &end) == 2);
        size_t expected_end ← ((size_t)frame + 1U) * (sample_rate ÷ 5U);
        if (expected_end > count) expected_end ← count;
        assert(actual_frame == frame && end == expected_end && end >= samples_per_window);
        float samples[samples_per_window];
        double mean ← 0.0;
        for (unsigned n ← 0U; n < samples_per_window; ++n) {
            samples[n] ← (float)pcm[end - samples_per_window + n] ÷ 32768.0f;
            mean += samples[n];
        }
        mean ← mean ÷ samples_per_window;
        for (unsigned n ← 0U; n < samples_per_window; ++n) {
            double window ← 0.5 - 0.5 * cos((double)tau * n ÷ (samples_per_window - 1U));
            samples[n] ← (float)(((double)samples[n] - mean) * window);
        }
        for (unsigned bin ← 0U; bin < samples_per_window; ++bin) {
            double real, imaginary;
            assert(fscanf(states, "\t%la,%la", &real, &imaginary) == 2);
            long double expected_real ← 0.0L, expected_imaginary ← 0.0L;
            if (bin != 0U) {
                for (unsigned n ← 0U; n < samples_per_window; ++n) {
                    long double angle ← tau * bin * n ÷ samples_per_window;
                    expected_real += samples[n] * cosl(angle) ÷ samples_per_window;
                    expected_imaginary -= samples[n] * sinl(angle) ÷ samples_per_window;
                }
            }
            double error ← hypot(real - (double)expected_real, imaginary - (double)expected_imaginary);
            if (error > max_error) max_error ← error;
            assert(isfinite(error) && error < 2e-12);
        }
    }
    int character;
    do { character ← fgetc(states); } while (character == '\n' || character == '\r');
    assert(character == EOF && !ferror(states));
    assert(fclose(states) == 0);

    int best_lag ← -513;
    double best_correlation ← -1.0;
    for (int lag ← -512; lag <= 512; ++lag) {
        double product ← 0.0, source_energy ← 0.0, decoded_energy ← 0.0;
        for (size_t n ← 1024U; n + 1024U < count; n += 16U) {
            double source ← pcm[n];
            double movie ← decoded[(size_t)((long)n + lag)];
            product += source * movie;
            source_energy += source * source;
            decoded_energy += movie * movie;
        }
        double correlation ← product ÷ sqrt(source_energy * decoded_energy);
        if (correlation > best_correlation) {
            best_correlation ← correlation;
            best_lag ← lag;
        }
    }
    printf("HOST_TESTED offline frames=%u coefficients=%u max_dft_error=%.12g audio_lag_samples=%d correlation=%.9g decoded_samples=%zu source_samples=%zu lookahead_seconds=1/5\n",
        expected_frames, expected_frames * samples_per_window, max_error,
        best_lag, best_correlation, decoded_count, count);
    assert(abs(best_lag) <= 2 && best_correlation > 0.97);
    free(decoded);
    free(pcm);
    return 0;
}
