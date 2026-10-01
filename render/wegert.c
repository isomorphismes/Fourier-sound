#include "wegert.h"

#include <math.h>

static double positive_fract(double value)
{
    return value - floor(value);
}

static double clamp01(double value)
{
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

static double srgb_component(double linear_value)
{
    double value = linear_value > 0.0 ? linear_value : 0.0;
    if (value <= 0.0031308) return 12.92 * value;
    return 1.055 * pow(value, 1.0 / 2.4) - 0.055;
}

static uint8_t byte(double value)
{
    return (uint8_t)lrint(clamp01(value) * 255.0);
}

static struct wegert_rgb hcl_to_srgb(double hue_degrees, double chroma,
                                     double lightness)
{
    const double white_u_prime = 0.19783982482140777;
    const double white_v_prime = 0.46833630293240974;
    const double degrees_to_radians =
        0.017453292519943295769236907684886;

    double hue = hue_degrees * degrees_to_radians;
    double u_star = chroma * cos(hue);
    double v_star = chroma * sin(hue);

    double y = lightness > 8.0
        ? pow((lightness + 16.0) / 116.0, 3.0)
        : lightness / 903.2962962962963;

    double u_prime =
        u_star / (13.0 * lightness) + white_u_prime;
    double v_prime =
        v_star / (13.0 * lightness) + white_v_prime;

    double x = (9.0 * y * u_prime) / (4.0 * v_prime);
    double z = y * (12.0 - 3.0 * u_prime - 20.0 * v_prime) /
               (4.0 * v_prime);

    double linear_red =
         3.2404542 * x - 1.5371385 * y - 0.4985314 * z;
    double linear_green =
        -0.9692660 * x + 1.8760108 * y + 0.0415560 * z;
    double linear_blue =
         0.0556434 * x - 0.2040259 * y + 1.0572252 * z;

    return (struct wegert_rgb){
        byte(srgb_component(linear_red)),
        byte(srgb_component(linear_green)),
        byte(srgb_component(linear_blue))
    };
}

bool wegert_color_from_phase_log_modulus(double phase, double log_modulus,
                                         struct wegert_rgb *rgb)
{
    if (!rgb || !isfinite(phase) || !isfinite(log_modulus)) return false;

    const double tau = 6.283185307179586476925286766559;
    const double log_10 = 2.3025850929940456840179914546844;

    double hue_degrees = 360.0 * positive_fract(phase / tau);
    double modulus_band = positive_fract(log_modulus / log_10);
    double lightness = 66.0
        + 4.0 * modulus_band
        + 3.0 * positive_fract(hue_degrees / 100.0);

    *rgb = hcl_to_srgb(hue_degrees, 45.0, lightness);
    return true;
}

bool wegert_color_complex(struct fourier_complex value,
                          struct wegert_rgb *rgb)
{
    if (!rgb || !isfinite(value.real) || !isfinite(value.imaginary))
        return false;

    double magnitude = hypot(value.real, value.imaginary);
    double phase = magnitude == 0.0
        ? 0.0
        : atan2(value.imaginary, value.real);
    if (magnitude < 1.0e-12) magnitude = 1.0e-12;

    return wegert_color_from_phase_log_modulus(phase, log(magnitude), rgb);
}

bool wegert_color_values(const struct fourier_complex *values, size_t count,
                         struct wegert_rgb *pixels, size_t capacity)
{
    if (!values || !pixels || capacity < count) return false;
    for (size_t index = 0; index < count; ++index)
        if (!wegert_color_complex(values[index], &pixels[index])) return false;
    return true;
}
