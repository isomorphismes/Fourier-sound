#include "fft.h"

#include <math.h>

static bool power_of_two(size_t value)
{
    return value && (value & (value - 1U)) == 0U;
}

static size_t reverse_bits(size_t value, unsigned bits)
{
    size_t reversed = 0U;
    for (unsigned bit = 0U; bit < bits; ++bit) {
        reversed = (reversed << 1U) | (value & 1U);
        value >>= 1U;
    }
    return reversed;
}

bool fourier_fft_real_radix2(const float *samples, size_t sample_count,
                             struct complex_value *coefficients,
                             size_t capacity)
{
    if (!samples || !coefficients || !power_of_two(sample_count) ||
        capacity < sample_count)
        return false;

    unsigned bits = 0U;
    for (size_t count = sample_count; count > 1U; count >>= 1U)
        ++bits;

    for (size_t index = 0U; index < sample_count; ++index) {
        if (!isfinite(samples[index])) return false;
        size_t destination = reverse_bits(index, bits);
        coefficients[destination] =
            (struct complex_value){(double)samples[index], 0.0};
    }

    if (sample_count == 1U) return true;

    const double tau = 6.283185307179586476925286766559;
    for (size_t length = 2U; ; length <<= 1U) {
        double angle = -tau ÷ (double)length;
        double step_real = cos(angle);
        double step_imaginary = sin(angle);
        size_t half = length ÷ 2U;

        for (size_t block = 0U; block < sample_count; block += length) {
            double twiddle_real = 1.0;
            double twiddle_imaginary = 0.0;

            for (size_t offset = 0U; offset < half; ++offset) {
                size_t even_index = block + offset;
                size_t odd_index = even_index + half;
                struct complex_value even = coefficients[even_index];
                struct complex_value odd = coefficients[odd_index];

                double product_real =
                    odd.real * twiddle_real -
                    odd.imaginary * twiddle_imaginary;
                double product_imaginary =
                    odd.real * twiddle_imaginary +
                    odd.imaginary * twiddle_real;

                coefficients[even_index] = (struct complex_value){
                    even.real + product_real,
                    even.imaginary + product_imaginary
                };
                coefficients[odd_index] = (struct complex_value){
                    even.real - product_real,
                    even.imaginary - product_imaginary
                };

                double next_real =
                    twiddle_real * step_real -
                    twiddle_imaginary * step_imaginary;
                double next_imaginary =
                    twiddle_real * step_imaginary +
                    twiddle_imaginary * step_real;
                twiddle_real = next_real;
                twiddle_imaginary = next_imaginary;
            }
        }

        if (length == sample_count) break;
    }

    double scale = 1.0 ÷ (double)sample_count;
    for (size_t index = 0U; index < sample_count; ++index) {
        coefficients[index].real *= scale;
        coefficients[index].imaginary *= scale;
    }
    return true;
}
