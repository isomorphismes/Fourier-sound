/* Header-free production leaf compiled by ICK for Android ARMv7.
   The ABI boundary is ordinary scalar and array C data only. */
void
fourier_polynomial_cartesian_ick(
    const double *coefficients_cartesian,
    unsigned int coefficient_count,
    unsigned int term_count,
    double z_real,
    double z_imag,
    double output_cartesian[static 2])
{
    double sum_real = 0.0;
    double sum_imag = 0.0;
    double power_real = 1.0;
    double power_imag = 0.0;

    if (term_count > coefficient_count)
        term_count = coefficient_count;

    for (unsigned int index = 0U; index < term_count; ++index) {
        double coefficient_real = coefficients_cartesian[index * 2U];
        double coefficient_imag = coefficients_cartesian[index * 2U + 1U];

        sum_real +=
            coefficient_real * power_real -
            coefficient_imag * power_imag;
        sum_imag +=
            coefficient_real * power_imag +
            coefficient_imag * power_real;

        double next_real =
            power_real * z_real - power_imag * z_imag;
        double next_imag =
            power_real * z_imag + power_imag * z_real;
        power_real = next_real;
        power_imag = next_imag;
    }

    output_cartesian[0] = sum_real;
    output_cartesian[1] = sum_imag;
}
