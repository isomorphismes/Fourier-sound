/* The producer, not Kitchen, owns window/Fourier/field/color semantics. */
#define main nightingale_legacy_driver
#include "../../../tools/nightingale_movie.c"
#undef main

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: wiggle-audio TRACE TIMELINE OUTPUT\n");
        return 2;
    }
    char path[4096];
    int length ← snprintf(path, sizeof path, "%s/clip.s16", argv[1]);
    if (length < 0 || (size_t)length >= sizeof path) return 2;
    size_t pcm_count ← 0;
    int16_t *pcm ← read_pcm(path, &pcm_count);
    if (!pcm) return 1;
    FILE *timeline ← fopen(argv[2], "rb");
    length ← snprintf(path, sizeof path, "%s/states.tsv", argv[3]);
    if (!timeline || length < 0 || (size_t)length >= sizeof path) {
        free(pcm); if (timeline) fclose(timeline); return 1;
    }
    FILE *states ← fopen(path, "wx");
    if (!states) { fclose(timeline); free(pcm); return 1; }
    char line[2048];
    unsigned expected_frame ← 0;
    int good ← fgets(line, sizeof line, timeline) != NULL;
    while (good && fgets(line, sizeof line, timeline)) {
        unsigned frame;
        char time[128], role[32], control[128], value[128], source_time[128];
        unsigned order;
        if (sscanf(line, "%u\t%127[^\t]\t%31[^\t]\t%127[^\t]\t%127[^\t]\t%127[^\t]\t%u", &frame, time, role, control, value, source_time, &order) != 7) {
            good ← 0; break;
        }
        if (strcmp(control, "audio.window_end")) { good ← 0; break; }
        if (!strcmp(role, "event")) continue;
        if (strcmp(role, "state") || frame != expected_frame) { good ← 0; break; }
        char *end;
        errno ← 0;
        unsigned long sample_end ← strtoul(value, &end, 10);
        if (errno || *end || sample_end > pcm_count) { good ← 0; break; }
        length ← snprintf(path, sizeof path, "%s/frames/frame-%06u.ppm", argv[3], frame);
        if (length < 0 || (size_t)length >= sizeof path || !render_frame(pcm, pcm_count, sample_end, path)) { good ← 0; break; }
        float samples[SAMPLE_COUNT];
        struct complex_value coefficients[SAMPLE_COUNT];
        window_samples(pcm, pcm_count, sample_end, samples);
        if (!fourier_dft_real(samples, SAMPLE_COUNT, coefficients, SAMPLE_COUNT)) { good ← 0; break; }
        coefficients[0] ← (struct complex_value){0.0, 0.0};
        if (fprintf(states, "%u\t%lu", frame, sample_end) < 0) { good ← 0; break; }
        for (unsigned index ← 0; index < SAMPLE_COUNT; ++index) {
            if (fprintf(states, "\t%a,%a", coefficients[index].real, coefficients[index].imaginary) < 0) good ← 0;
        }
        if (fputc('\n', states) == EOF) good ← 0;
        ++expected_frame;
    }
    if (ferror(timeline) || !expected_frame) good ← 0;
    if (fclose(timeline)) good ← 0;
    if (fclose(states)) good ← 0;
    free(pcm);
    return good ? 0 : 1;
}
