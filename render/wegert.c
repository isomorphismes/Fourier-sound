#include "wegert.h"

#include <math.h>

static uint8_t byte(double value)
{
    if (value <= 0.0) return 0;
    if (value >= 1.0) return 255;
    return (uint8_t)lrint(value * 255.0);
}

bool wegert_phase_rgb(struct fourier_complex value, struct wegert_rgb *rgb)
{
    if (!rgb) return false;
    if (!isfinite(value.real) || !isfinite(value.imaginary)) {
        *rgb = (struct wegert_rgb){0, 0, 0};
        return false;
    }

    double magnitude = hypot(value.real, value.imaginary);
    if (magnitude <= 1e-15) {
        *rgb = (struct wegert_rgb){0, 0, 0};
        return true;
    }

    const double tau = 6.283185307179586476925286766559;
    double phase = atan2(value.imaginary, value.real);
    if (phase < 0.0) phase += tau;
    double hue = 6.0 * phase / tau;
    int sector = (int)floor(hue);
    if (sector >= 6) sector = 0;
    double fraction = hue - floor(hue);

    double red = 0.0, green = 0.0, blue = 0.0;
    switch (sector) {
    case 0: red = 1.0; green = fraction; break;
    case 1: red = 1.0 - fraction; green = 1.0; break;
    case 2: green = 1.0; blue = fraction; break;
    case 3: green = 1.0 - fraction; blue = 1.0; break;
    case 4: red = fraction; blue = 1.0; break;
    case 5: red = 1.0; blue = 1.0 - fraction; break;
    default: return false;
    }

    *rgb = (struct wegert_rgb){byte(red), byte(green), byte(blue)};
    return true;
}
