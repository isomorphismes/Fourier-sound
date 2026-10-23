#ifndef FOURIER_GPU_VOICE_PIPELINE_H
#define FOURIER_GPU_VOICE_PIPELINE_H

#include <android/native_window.h>

#include <stddef.h>

enum gpu_voice_result {
    GPU_VOICE_OK = 0,
    GPU_VOICE_INVALID,
    GPU_VOICE_EGL_ERROR,
    GPU_VOICE_GL_ERROR,
    GPU_VOICE_SWAP_ERROR
};

struct gpu_voice_pipeline;
#ifdef FOURIER_GPU_DIAGNOSTICS
#include <stdbool.h>
bool gpu_voice_pipeline_read_coefficients(struct gpu_voice_pipeline *, float *, size_t);
#endif

struct gpu_voice_pipeline *gpu_voice_pipeline_create(
    ANativeWindow *window,
    size_t sample_count,
    size_t term_count,
    int requested_width,
    int requested_height);

void gpu_voice_pipeline_destroy(struct gpu_voice_pipeline **pipeline);

enum gpu_voice_result gpu_voice_pipeline_render(
    struct gpu_voice_pipeline *pipeline,
    const float *samples,
    size_t sample_count);

const char *gpu_voice_result_text(enum gpu_voice_result result);

#endif
