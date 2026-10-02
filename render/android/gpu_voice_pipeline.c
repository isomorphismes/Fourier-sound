#include "gpu_voice_pipeline.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>
#include <android/log.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

#define GPU_LOG(...) \
    __android_log_print(ANDROID_LOG_INFO, "FourierVoiceGPU", __VA_ARGS__)
#define LOCAL_SIZE 64U

struct gpu_voice_pipeline {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    GLuint sample_buffer;
    GLuint spectrum[2];
    GLuint init_program;
    GLuint stage_program;
    GLuint render_program;
    GLint init_count;
    GLint init_bits;
    GLint stage_length;
    GLint stage_scale;
    GLint render_terms;
    GLint render_radius;
    size_t sample_count;
    size_t term_count;
    unsigned int fft_bits;
    int width;
    int height;
};

static const char *init_compute_source =
    "#version 310 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "layout(local_size_x = 64) in;\n"
    "layout(std430, binding = 0) readonly buffer InputSamples {\n"
    "    float samples[];\n"
    "};\n"
    "layout(std430, binding = 1) writeonly buffer OutputSpectrum {\n"
    "    vec2 spectrum[];\n"
    "};\n"
    "uniform uint uCount;\n"
    "uniform uint uBits;\n"
    "void main() {\n"
    "    uint index = gl_GlobalInvocationID.x;\n"
    "    if (index >= uCount) return;\n"
    "    uint destination = bitfieldReverse(index) >> (32u - uBits);\n"
    "    spectrum[destination] = vec2(samples[index], 0.0);\n"
    "}\n";

static const char *stage_compute_source =
    "#version 310 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "layout(local_size_x = 64) in;\n"
    "layout(std430, binding = 0) readonly buffer InputSpectrum {\n"
    "    vec2 inputSpectrum[];\n"
    "};\n"
    "layout(std430, binding = 1) writeonly buffer OutputSpectrum {\n"
    "    vec2 outputSpectrum[];\n"
    "};\n"
    "uniform uint uLength;\n"
    "uniform float uScale;\n"
    "const float TAU = 6.28318530717958647692;\n"
    "vec2 complexMultiply(vec2 a, vec2 b) {\n"
    "    return vec2(a.x * b.x - a.y * b.y,\n"
    "                a.x * b.y + a.y * b.x);\n"
    "}\n"
    "void main() {\n"
    "    uint pair = gl_GlobalInvocationID.x;\n"
    "    uint halfLength = uLength >> 1u;\n"
    "    uint pairCount = uint(inputSpectrum.length()) >> 1u;\n"
    "    if (pair >= pairCount) return;\n"
    "    uint block = (pair / halfLength) * uLength;\n"
    "    uint offset = pair % halfLength;\n"
    "    uint evenIndex = block + offset;\n"
    "    uint oddIndex = evenIndex + halfLength;\n"
    "    float angle = -TAU * float(offset) / float(uLength);\n"
    "    vec2 twiddle = vec2(cos(angle), sin(angle));\n"
    "    vec2 evenValue = inputSpectrum[evenIndex];\n"
    "    vec2 oddValue = complexMultiply(inputSpectrum[oddIndex], twiddle);\n"
    "    outputSpectrum[evenIndex] = (evenValue + oddValue) * uScale;\n"
    "    outputSpectrum[oddIndex] = (evenValue - oddValue) * uScale;\n"
    "}\n";

static const char *vertex_source =
    "#version 310 es\n"
    "precision highp float;\n"
    "out vec2 vUv;\n"
    "void main() {\n"
    "    vec2 position;\n"
    "    if (gl_VertexID == 0) position = vec2(-1.0, -1.0);\n"
    "    else if (gl_VertexID == 1) position = vec2(3.0, -1.0);\n"
    "    else position = vec2(-1.0, 3.0);\n"
    "    vUv = 0.5 * (position + vec2(1.0));\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

static const char *fragment_source =
    "#version 310 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "layout(std430, binding = 0) readonly buffer Spectrum {\n"
    "    vec2 coefficients[];\n"
    "};\n"
    "uniform uint uTermCount;\n"
    "uniform vec2 uRadius;\n"
    "in vec2 vUv;\n"
    "layout(location = 0) out vec4 fragmentColor;\n"
    "const float TAU = 6.28318530717958647692;\n"
    "const float LOG10 = 2.30258509299404568402;\n"
    "vec2 complexMultiply(vec2 a, vec2 b) {\n"
    "    return vec2(a.x * b.x - a.y * b.y,\n"
    "                a.x * b.y + a.y * b.x);\n"
    "}\n"
    "vec2 polynomial(vec2 z) {\n"
    "    vec2 value = vec2(0.0);\n"
    "    for (uint remaining = uTermCount; remaining > 0u; --remaining) {\n"
    "        uint k = remaining - 1u;\n"
    "        vec2 coefficient = k == 0u ? vec2(0.0) : coefficients[k];\n"
    "        value = complexMultiply(value, z) + coefficient;\n"
    "    }\n"
    "    return value;\n"
    "}\n"
    "float srgbComponent(float linearValue) {\n"
    "    float value = max(linearValue, 0.0);\n"
    "    if (value <= 0.0031308) return 12.92 * value;\n"
    "    return 1.055 * pow(value, 1.0 / 2.4) - 0.055;\n"
    "}\n"
    "vec3 hclToSrgb(float hueDegrees, float chroma, float lightness) {\n"
    "    const float whiteUPrime = 0.19783982482140777;\n"
    "    const float whiteVPrime = 0.46833630293240974;\n"
    "    float hue = radians(hueDegrees);\n"
    "    float uStar = chroma * cos(hue);\n"
    "    float vStar = chroma * sin(hue);\n"
    "    float y = lightness > 8.0\n"
    "        ? pow((lightness + 16.0) / 116.0, 3.0)\n"
    "        : lightness / 903.2962962962963;\n"
    "    float uPrime = uStar / (13.0 * lightness) + whiteUPrime;\n"
    "    float vPrime = vStar / (13.0 * lightness) + whiteVPrime;\n"
    "    float x = (9.0 * y * uPrime) / (4.0 * vPrime);\n"
    "    float z = y * (12.0 - 3.0 * uPrime - 20.0 * vPrime) /\n"
    "              (4.0 * vPrime);\n"
    "    vec3 linearRgb = vec3(\n"
    "         3.2404542 * x - 1.5371385 * y - 0.4985314 * z,\n"
    "        -0.9692660 * x + 1.8760108 * y + 0.0415560 * z,\n"
    "         0.0556434 * x - 0.2040259 * y + 1.0572252 * z);\n"
    "    return clamp(vec3(srgbComponent(linearRgb.r),\n"
    "                      srgbComponent(linearRgb.g),\n"
    "                      srgbComponent(linearRgb.b)), 0.0, 1.0);\n"
    "}\n"
    "vec3 wegertColor(vec2 value) {\n"
    "    float phase = atan(value.y, value.x);\n"
    "    float magnitude = max(length(value), 1.0e-12);\n"
    "    float hueDegrees = 360.0 * fract(phase / TAU);\n"
    "    float modulusBand = fract(log(magnitude) / LOG10);\n"
    "    float lightness = 66.0 + 4.0 * modulusBand\n"
    "        + 3.0 * fract(hueDegrees / 100.0);\n"
    "    return hclToSrgb(hueDegrees, 45.0, lightness);\n"
    "}\n"
    "void main() {\n"
    "    vec2 z = vec2(mix(-uRadius.x, uRadius.x, vUv.x),\n"
    "                  mix(-uRadius.y, uRadius.y, vUv.y));\n"
    "    fragmentColor = vec4(wegertColor(polynomial(z)), 1.0);\n"
    "}\n";

static bool power_of_two(size_t value)
{
    return value >= 2U && (value & (value - 1U)) == 0U;
}

static unsigned int bit_count(size_t value)
{
    unsigned int bits = 0U;
    while (value > 1U) {
        value >>= 1U;
        ++bits;
    }
    return bits;
}

static bool gl_ok(const char *where)
{
    bool ok = true;
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        GPU_LOG("GPU_GL_ERROR where=%s code=0x%x", where, (unsigned)error);
        ok = false;
    }
    return ok;
}

static GLuint compile_shader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    if (!shader) return 0U;

    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) return shader;

    char log[2048];
    GLsizei length = 0;
    glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &length, log);
    GPU_LOG("GPU_SHADER_COMPILE_FAIL type=0x%x log=%.*s",
            (unsigned)type, (int)length, log);
    glDeleteShader(shader);
    return 0U;
}

static GLuint link_program(GLuint first, GLuint second)
{
    GLuint program = glCreateProgram();
    if (!program) return 0U;

    glAttachShader(program, first);
    if (second) glAttachShader(program, second);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) return program;

    char log[2048];
    GLsizei length = 0;
    glGetProgramInfoLog(program, (GLsizei)sizeof(log), &length, log);
    GPU_LOG("GPU_PROGRAM_LINK_FAIL log=%.*s", (int)length, log);
    glDeleteProgram(program);
    return 0U;
}

static GLuint build_compute_program(const char *source)
{
    GLuint compute = compile_shader(GL_COMPUTE_SHADER, source);
    if (!compute) return 0U;
    GLuint program = link_program(compute, 0U);
    glDeleteShader(compute);
    return program;
}

static GLuint build_render_program(void)
{
    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (!vertex) return 0U;
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (!fragment) {
        glDeleteShader(vertex);
        return 0U;
    }

    GLuint program = link_program(vertex, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

static bool locate_uniforms(struct gpu_voice_pipeline *pipeline)
{
    pipeline->init_count =
        glGetUniformLocation(pipeline->init_program, "uCount");
    pipeline->init_bits =
        glGetUniformLocation(pipeline->init_program, "uBits");
    pipeline->stage_length =
        glGetUniformLocation(pipeline->stage_program, "uLength");
    pipeline->stage_scale =
        glGetUniformLocation(pipeline->stage_program, "uScale");
    pipeline->render_terms =
        glGetUniformLocation(pipeline->render_program, "uTermCount");
    pipeline->render_radius =
        glGetUniformLocation(pipeline->render_program, "uRadius");

    return pipeline->init_count >= 0 &&
           pipeline->init_bits >= 0 &&
           pipeline->stage_length >= 0 &&
           pipeline->stage_scale >= 0 &&
           pipeline->render_terms >= 0 &&
           pipeline->render_radius >= 0;
}

static bool initialize_egl(struct gpu_voice_pipeline *pipeline,
                           ANativeWindow *window,
                           int requested_width,
                           int requested_height)
{
    pipeline->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (pipeline->display == EGL_NO_DISPLAY) return false;

    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(pipeline->display, &major, &minor) != EGL_TRUE)
        return false;
    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) return false;

    const EGLint config_attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = NULL;
    EGLint config_count = 0;
    if (eglChooseConfig(pipeline->display, config_attributes,
                        &config, 1, &config_count) != EGL_TRUE ||
        config_count < 1)
        return false;

    EGLint format = 0;
    if (eglGetConfigAttrib(
            pipeline->display, config, EGL_NATIVE_VISUAL_ID, &format) != EGL_TRUE)
        return false;
    if (ANativeWindow_setBuffersGeometry(
            window, requested_width, requested_height, format) != 0)
        return false;

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    pipeline->context = eglCreateContext(
        pipeline->display, config, EGL_NO_CONTEXT, context_attributes);
    if (pipeline->context == EGL_NO_CONTEXT) return false;

    pipeline->surface =
        eglCreateWindowSurface(pipeline->display, config, window, NULL);
    if (pipeline->surface == EGL_NO_SURFACE) return false;

    if (eglMakeCurrent(pipeline->display, pipeline->surface,
                       pipeline->surface, pipeline->context) != EGL_TRUE)
        return false;

    EGLint width = 0;
    EGLint height = 0;
    if (eglQuerySurface(
            pipeline->display, pipeline->surface, EGL_WIDTH, &width) != EGL_TRUE ||
        eglQuerySurface(
            pipeline->display, pipeline->surface, EGL_HEIGHT, &height) != EGL_TRUE)
        return false;
    pipeline->width = width;
    pipeline->height = height;

    GLint gl_major = 0;
    GLint gl_minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
    glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
    if (gl_major < 3 || (gl_major == 3 && gl_minor < 1)) {
        GPU_LOG("GPU_GL_VERSION_UNSUPPORTED major=%d minor=%d", gl_major, gl_minor);
        return false;
    }

    GLint max_invocations = 0;
    GLint max_bindings = 0;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_invocations);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &max_bindings);
    GPU_LOG("GPU_READY egl=%d.%d gl=%d.%d renderer=%s surface=%dx%d "
            "max_compute_invocations=%d ssbo_bindings=%d",
            major, minor, gl_major, gl_minor,
            (const char *)glGetString(GL_RENDERER),
            pipeline->width, pipeline->height,
            max_invocations, max_bindings);
    return gl_ok("egl-init");
}

static bool initialize_gl(struct gpu_voice_pipeline *pipeline)
{
    pipeline->init_program = build_compute_program(init_compute_source);
    pipeline->stage_program = build_compute_program(stage_compute_source);
    pipeline->render_program = build_render_program();
    if (!pipeline->init_program ||
        !pipeline->stage_program ||
        !pipeline->render_program)
        return false;

    if (!locate_uniforms(pipeline)) {
        GPU_LOG("GPU_UNIFORM_LOOKUP_FAIL");
        return false;
    }

    glGenBuffers(1, &pipeline->sample_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pipeline->sample_buffer);
    glBufferData(
        GL_SHADER_STORAGE_BUFFER,
        (GLsizeiptr)(pipeline->sample_count * sizeof(float)),
        NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(2, pipeline->spectrum);
    for (size_t index = 0U; index < 2U; ++index) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pipeline->spectrum[index]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            (GLsizeiptr)(pipeline->sample_count * 2U * sizeof(float)),
            NULL, GL_DYNAMIC_DRAW);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0U);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glViewport(0, 0, pipeline->width, pipeline->height);
    return gl_ok("gl-init");
}

struct gpu_voice_pipeline *gpu_voice_pipeline_create(
    ANativeWindow *window,
    size_t sample_count,
    size_t term_count,
    int requested_width,
    int requested_height)
{
    if (!window || !power_of_two(sample_count) ||
        term_count == 0U || term_count > sample_count ||
        requested_width <= 0 || requested_height <= 0)
        return NULL;

    struct gpu_voice_pipeline *pipeline =
        calloc(1U, sizeof(*pipeline));
    if (!pipeline) return NULL;

    pipeline->display = EGL_NO_DISPLAY;
    pipeline->context = EGL_NO_CONTEXT;
    pipeline->surface = EGL_NO_SURFACE;
    pipeline->sample_count = sample_count;
    pipeline->term_count = term_count;
    pipeline->fft_bits = bit_count(sample_count);

    if (!initialize_egl(
            pipeline, window, requested_width, requested_height) ||
        !initialize_gl(pipeline)) {
        GPU_LOG("GPU_CREATE_FAIL egl_error=0x%x", (unsigned)eglGetError());
        gpu_voice_pipeline_destroy(&pipeline);
        return NULL;
    }

    GPU_LOG("GPU_PIPELINE sample_count=%zu fft_stages=%u terms=%zu "
            "spectrum_readback=none",
            pipeline->sample_count, pipeline->fft_bits, pipeline->term_count);
    return pipeline;
}

void gpu_voice_pipeline_destroy(struct gpu_voice_pipeline **pipeline_pointer)
{
    if (!pipeline_pointer || !*pipeline_pointer) return;
    struct gpu_voice_pipeline *pipeline = *pipeline_pointer;

    if (pipeline->display != EGL_NO_DISPLAY &&
        pipeline->context != EGL_NO_CONTEXT &&
        pipeline->surface != EGL_NO_SURFACE)
        (void)eglMakeCurrent(pipeline->display, pipeline->surface,
                             pipeline->surface, pipeline->context);

    if (pipeline->sample_buffer) glDeleteBuffers(1, &pipeline->sample_buffer);
    if (pipeline->spectrum[0] || pipeline->spectrum[1])
        glDeleteBuffers(2, pipeline->spectrum);
    if (pipeline->init_program) glDeleteProgram(pipeline->init_program);
    if (pipeline->stage_program) glDeleteProgram(pipeline->stage_program);
    if (pipeline->render_program) glDeleteProgram(pipeline->render_program);

    if (pipeline->display != EGL_NO_DISPLAY) {
        (void)eglMakeCurrent(
            pipeline->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (pipeline->surface != EGL_NO_SURFACE)
            (void)eglDestroySurface(pipeline->display, pipeline->surface);
        if (pipeline->context != EGL_NO_CONTEXT)
            (void)eglDestroyContext(pipeline->display, pipeline->context);
        (void)eglTerminate(pipeline->display);
    }

    free(pipeline);
    *pipeline_pointer = NULL;
}

static enum gpu_voice_result run_fft(
    struct gpu_voice_pipeline *pipeline,
    const float *samples)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, pipeline->sample_buffer);
    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER, 0,
        (GLsizeiptr)(pipeline->sample_count * sizeof(float)),
        samples);

    glUseProgram(pipeline->init_program);
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER, 0U, pipeline->sample_buffer);
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER, 1U, pipeline->spectrum[0]);
    glUniform1ui(pipeline->init_count, (GLuint)pipeline->sample_count);
    glUniform1ui(pipeline->init_bits, pipeline->fft_bits);
    GLuint init_groups =
        (GLuint)((pipeline->sample_count + LOCAL_SIZE - 1U) / LOCAL_SIZE);
    glDispatchCompute(init_groups, 1U, 1U);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    GLuint source = pipeline->spectrum[0];
    GLuint destination = pipeline->spectrum[1];
    for (size_t length = 2U; length <= pipeline->sample_count; length <<= 1U) {
        glUseProgram(pipeline->stage_program);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0U, source);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1U, destination);
        glUniform1ui(pipeline->stage_length, (GLuint)length);
        glUniform1f(
            pipeline->stage_scale,
            length == pipeline->sample_count
                ? 1.0f / (float)pipeline->sample_count
                : 1.0f);

        size_t pair_count = pipeline->sample_count / 2U;
        GLuint groups =
            (GLuint)((pair_count + LOCAL_SIZE - 1U) / LOCAL_SIZE);
        glDispatchCompute(groups, 1U, 1U);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        GLuint swap = source;
        source = destination;
        destination = swap;
    }

    pipeline->spectrum[0] = source;
    pipeline->spectrum[1] = destination;
    return gl_ok("fft") ? GPU_VOICE_OK : GPU_VOICE_GL_ERROR;
}

enum gpu_voice_result gpu_voice_pipeline_render(
    struct gpu_voice_pipeline *pipeline,
    const float *samples,
    size_t sample_count)
{
    if (!pipeline || !samples || sample_count != pipeline->sample_count)
        return GPU_VOICE_INVALID;

    enum gpu_voice_result result = run_fft(pipeline, samples);
    if (result != GPU_VOICE_OK) return result;

    glUseProgram(pipeline->render_program);
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER, 0U, pipeline->spectrum[0]);
    glUniform1ui(pipeline->render_terms, (GLuint)pipeline->term_count);
    glUniform2f(pipeline->render_radius, 0.44f, 0.88f);
    glViewport(0, 0, pipeline->width, pipeline->height);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
    if (!gl_ok("render")) return GPU_VOICE_GL_ERROR;
    if (eglSwapBuffers(pipeline->display, pipeline->surface) != EGL_TRUE)
        return GPU_VOICE_SWAP_ERROR;
    return GPU_VOICE_OK;
}

const char *gpu_voice_result_text(enum gpu_voice_result result)
{
    switch (result) {
    case GPU_VOICE_OK: return "ok";
    case GPU_VOICE_INVALID: return "invalid argument";
    case GPU_VOICE_EGL_ERROR: return "EGL error";
    case GPU_VOICE_GL_ERROR: return "OpenGL ES error";
    case GPU_VOICE_SWAP_ERROR: return "buffer swap error";
    }
    return "unknown GPU voice result";
}
