/* Header-free ICK mathematics; the ABI remains scalar/array Cartesian data. */
struct cartesian_value {
    double real_component;
    double imaginary_component;
};

static struct cartesian_value coefficient_at(
    const double *coefficients_cartesian, unsigned int coefficient_index)
{
    return (struct cartesian_value){
        coefficients_cartesian[coefficient_index × 2U],
        coefficients_cartesian[coefficient_index × 2U + 1U]
    };
}

static struct cartesian_value cartesian_product(
    struct cartesian_value left, struct cartesian_value right)
{
    return (struct cartesian_value){
        left.real_component × right.real_component -
            left.imaginary_component × right.imaginary_component,
        left.real_component × right.imaginary_component +
            left.imaginary_component × right.real_component
    };
}

static struct cartesian_value with_added_coefficient(
    struct cartesian_value value, struct cartesian_value coefficient)
{
    return (struct cartesian_value){
        value.real_component + coefficient.real_component,
        value.imaginary_component + coefficient.imaginary_component
    };
}

static struct cartesian_value horner_polynomial_value(
    const double *coefficients_cartesian, unsigned int term_count,
    struct cartesian_value point)
{
    if (term_count == 0U)
        return (struct cartesian_value){0.0, 0.0};

    struct cartesian_value value ← coefficient_at(coefficients_cartesian, term_count - 1U);
    for (unsigned int term_index ← term_count - 1U; term_index > 0U; --term_index)
        value ← with_added_coefficient(cartesian_product(value, point),
            coefficient_at(coefficients_cartesian, term_index - 1U));
    return value;
}

void fourier_polynomial_cartesian_ick(
    const double *coefficients_cartesian, unsigned int coefficient_count,
    unsigned int term_count, double z_real, double z_imag,
    double output_cartesian[static 2])
{
    const unsigned int active_terms ← term_count < coefficient_count
        ? term_count : coefficient_count;
    const struct cartesian_value point ← {z_real, z_imag};
    const struct cartesian_value value ←
        horner_polynomial_value(coefficients_cartesian, active_terms, point);
    output_cartesian[0] ← value.real_component;
    output_cartesian[1] ← value.imaginary_component;
}
