#include "complex_field.h"
#include "dft.h"
#include "pcm_block.h"
#include "ppm.h"
#include "wegert.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SAMPLE_COUNT 16U
#define IMAGE_SIDE 33U
#define IMAGE_PIXELS (IMAGE_SIDE * IMAGE_SIDE)

static bool near(double a, double b, double tolerance)
{
    return fabs(a - b) <= tolerance;
}

static bool byte_near(uint8_t actual, uint8_t expected)
{
    int difference = (int)actual - (int)expected;
    return difference >= -1 && difference <= 1;
}

static void expect_rgb(struct fourier_complex value,
                       uint8_t red, uint8_t green, uint8_t blue)
{
    struct wegert_rgb rgb;
    assert(wegert_color_complex(value, &rgb));
    assert(byte_near(rgb.red, red));
    assert(byte_near(rgb.green, green));
    assert(byte_near(rgb.blue, blue));
}

static void verify_ppm(const char *path, const struct wegert_rgb *pixels)
{
    const char header[] = "P6\n33 33\n255\n";
    unsigned char bytes[IMAGE_PIXELS * 3U];

    FILE *file = fopen(path, "rb");
    assert(file);

    char actual_header[sizeof(header) - 1U];
    assert(fread(actual_header, 1, sizeof(actual_header), file) ==
           sizeof(actual_header));
    assert(memcmp(actual_header, header, sizeof(actual_header)) == 0);

    assert(fread(bytes, 1, sizeof(bytes), file) == sizeof(bytes));
    assert(fgetc(file) == EOF);
    assert(fclose(file) == 0);

    for (size_t index = 0; index < IMAGE_PIXELS; ++index) {
        assert(bytes[index * 3U] == pixels[index].red);
        assert(bytes[index * 3U + 1U] == pixels[index].green);
        assert(bytes[index * 3U + 2U] == pixels[index].blue);
    }
}

int main(int argc, char **argv)
{
    const double tau = 6.283185307179586476925286766559;
    float pcm[SAMPLE_COUNT];
    for (size_t n = 0; n < SAMPLE_COUNT; ++n) {
        double t = (double)n / SAMPLE_COUNT;
        pcm[n] = (float)(0.1 + 0.75 * cos(tau * 2.0 * t) +
                               0.25 * sin(tau * 3.0 * t));
    }

    float samples[SAMPLE_COUNT] = {0};
    struct audio_properties source = {16000, 1, AUDIO_FLOAT32};
    assert(fourier_pcm_mono(source, pcm, SAMPLE_COUNT,
                            samples, SAMPLE_COUNT));

    struct fourier_complex coefficients[SAMPLE_COUNT];
    assert(fourier_dft_real(samples, SAMPLE_COUNT,
                            coefficients, SAMPLE_COUNT));

    assert(near(coefficients[0].real, 0.1, 1e-6));
    assert(near(coefficients[0].imaginary, 0.0, 1e-6));
    assert(near(coefficients[2].real, 0.375, 1e-6));
    assert(near(coefficients[14].real, 0.375, 1e-6));
    assert(near(coefficients[3].imaginary, -0.125, 1e-6));
    assert(near(coefficients[13].imaginary, 0.125, 1e-6));

    for (size_t k = 0; k < SAMPLE_COUNT; ++k) {
        if (k == 0 || k == 2 || k == 3 || k == 13 || k == 14) continue;
        assert(hypot(coefficients[k].real,
                     coefficients[k].imaginary) < 1e-6);
    }

    struct fourier_complex simple[] = {{1.0, 0.0}, {2.0, 0.0}};
    struct fourier_complex at_i = fourier_polynomial_value(
        simple, 2, 2, (struct fourier_complex){0.0, 1.0});
    assert(near(at_i.real, 1.0, 1e-12));
    assert(near(at_i.imaginary, 2.0, 1e-12));

    /* Values independently calculated from the canonical Wegert GLSL port.
     * One-byte tolerance covers libm rounding without weakening the mapping. */
    expect_rgb((struct fourier_complex){1.0, 0.0}, 212, 141, 155);
    expect_rgb((struct fourier_complex){-1.0, 0.0}, 73, 184, 172);
    expect_rgb((struct fourier_complex){0.0, 1.0}, 167, 172, 106);
    expect_rgb((struct fourier_complex){0.0, -1.0}, 166, 160, 215);

    struct wegert_rgb decade_a;
    struct wegert_rgb decade_b;
    assert(wegert_color_complex((struct fourier_complex){2.0, 0.0},
                                &decade_a));
    assert(wegert_color_complex((struct fourier_complex){20.0, 0.0},
                                &decade_b));
    assert(byte_near(decade_a.red, decade_b.red));
    assert(byte_near(decade_a.green, decade_b.green));
    assert(byte_near(decade_a.blue, decade_b.blue));

    struct fourier_complex values[IMAGE_PIXELS];
    for (size_t row = 0; row < IMAGE_SIDE; ++row) {
        double y = 1.25 - 2.5 * (double)row / (double)(IMAGE_SIDE - 1U);
        for (size_t column = 0; column < IMAGE_SIDE; ++column) {
            double x = -1.25 +
                2.5 * (double)column / (double)(IMAGE_SIDE - 1U);
            size_t index = row * IMAGE_SIDE + column;
            values[index] = fourier_polynomial_value(
                coefficients, SAMPLE_COUNT, 8,
                (struct fourier_complex){x, y});
        }
    }

    struct wegert_rgb pixels[IMAGE_PIXELS];
    assert(wegert_color_values(values, IMAGE_PIXELS,
                               pixels, IMAGE_PIXELS));

    size_t different = 0;
    for (size_t index = 1; index < IMAGE_PIXELS; ++index)
        if (pixels[index].red != pixels[0].red ||
            pixels[index].green != pixels[0].green ||
            pixels[index].blue != pixels[0].blue)
            ++different;
    assert(different > IMAGE_PIXELS / 2U);

    size_t center = (IMAGE_SIDE / 2U) * IMAGE_SIDE + IMAGE_SIDE / 2U;
    assert(byte_near(pixels[center].red, 212));
    assert(byte_near(pixels[center].green, 141));
    assert(byte_near(pixels[center].blue, 155));

    const char *path = argc > 1 ? argv[1] : "fourier-render.ppm";
    assert(rgb24_write_ppm(path, pixels, IMAGE_SIDE, IMAGE_SIDE,
                           IMAGE_PIXELS));
    verify_ppm(path, pixels);

    printf("PASS PCM -> complex DFT -> sampled field -> canonical Wegert color -> PPM %s\n",
           path);
    return 0;
}
