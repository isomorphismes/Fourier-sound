#include "dft.h"

#include <math.h>

bool fourier_dft_real(const float *samples, size_t sample_count,
                      struct complex_value *coefficients, size_t capacity)
{
    if (!samples || !coefficients || !sample_count || capacity < sample_count)
        return false;

    const double tau = 6.283185307179586476925286766559;
    const double scale = 1.0 / (double)sample_count;

    for (size_t n = 0; n < sample_count; ++n)
        if (!isfinite(samples[n])) return false;

    for (size_t k = 0; k < sample_count; ++k) {
        double real = 0.0;
        double imaginary = 0.0;
        for (size_t n = 0; n < sample_count; ++n) {
            double angle = tau * (double)k * (double)n / (double)sample_count;
            double sample = samples[n];
            real += sample * cos(angle);
            imaginary -= sample * sin(angle);
        }
        coefficients[k] = (struct complex_value){real * scale,
                                                  imaginary * scale};
    }
    return true;
}
