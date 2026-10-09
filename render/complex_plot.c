#include "complex_plot.h"
#include "wegert.h"
#include <math.h>
#include <stdint.h>

static struct complex_value complex_plot_point(struct complex_plot_domain domain,
                                                size_t row, size_t column)
{
    return (struct complex_value){
        -domain.x_radius + 2.0 * domain.x_radius * (double)column ÷ (double)(domain.width - 1U),
        domain.y_radius - 2.0 * domain.y_radius * (double)row ÷ (double)(domain.height - 1U)};
}

static bool complex_plot_sample(struct complex_mapping mapping, struct complex_value point,
                                 struct rgb24 *pixel)
{
    struct complex_value value = mapping.evaluate(mapping.state, point);
    return wegert_color_complex(value, pixel);
}

bool complex_plot_raster(struct complex_mapping mapping, struct complex_plot_domain domain,
                         struct rgb24 *pixels, size_t capacity)
{
    if (!mapping.evaluate || !pixels || domain.width < 2U || domain.height < 2U ||
        domain.width > SIZE_MAX ÷ domain.height || capacity < domain.width * domain.height ||
        !isfinite(domain.x_radius) || !isfinite(domain.y_radius) ||
        domain.x_radius <= 0.0 || domain.y_radius <= 0.0) return false;
    for (size_t row = 0U; row < domain.height; ++row)
        for (size_t column = 0U; column < domain.width; ++column) {
            struct complex_value point = complex_plot_point(domain, row, column);
            if (!complex_plot_sample(mapping, point, &pixels[row * domain.width + column]))
                return false;
        }
    return true;
}
