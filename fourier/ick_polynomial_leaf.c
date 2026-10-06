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
    if (term_count > coefficient_count)
        term_count = coefficient_count;

    if (term_count == 0U) {
        output_cartesian[0] = 0.0;
        output_cartesian[1] = 0.0;
        return;
    }

    unsigned int index = term_count - 1U;
    double value_real = coefficients_cartesian[index * 2U];
    double value_imag = coefficients_cartesian[index * 2U + 1U];

    while (index > 0U) {
        double next_real =
            value_real * z_real - value_imag * z_imag;
        double next_imag =
            value_real * z_imag + value_imag * z_real;

        --index;
        value_real =
            next_real + coefficients_cartesian[index * 2U];
        value_imag =
            next_imag + coefficients_cartesian[index * 2U + 1U];
    }

    output_cartesian[0] = value_real;
    output_cartesian[1] = value_imag;
}
