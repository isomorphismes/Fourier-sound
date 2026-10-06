#include "complex_plot.h"
#include "complex_field.h"
#include "wegert.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static struct complex_value identity(const void *state, struct complex_value point)
{
    assert(state == NULL);
    return point;
}
int main(void)
{
    struct complex_plot_domain domain = {2.0, 1.0, 3U, 3U};
    struct rgb24 pixels[9], expected;
    assert(complex_plot_raster((struct complex_mapping){NULL, identity}, domain, pixels, 9U));
    for (size_t row=0; row<3U; ++row) for (size_t col=0; col<3U; ++col) {
        assert(wegert_color_complex((struct complex_value){-2.0+2.0*(double)col,1.0-(double)row}, &expected));
        assert(memcmp(&expected,&pixels[row*3U+col],sizeof(expected))==0);
    }
    const struct complex_value coefficients[] = {{-1,0},{0,0},{0,0},{1,0}};
    struct complex_value before[4]; memcpy(before,coefficients,sizeof(before));
    struct fourier_polynomial polynomial = {coefficients,4,4};
    assert(complex_plot_raster((struct complex_mapping){&polynomial,fourier_polynomial_evaluate},domain,pixels,9));
    assert(memcmp(before,coefficients,sizeof(before))==0);
    for (size_t row=0; row<3U; ++row) for (size_t col=0; col<3U; ++col) {
        double x=-2+2*(double)col,y=1-(double)row;
        /* Independently expand z^3-1. */
        assert(wegert_color_complex((struct complex_value){x*x*x-3*x*y*y-1,3*x*x*y-y*y*y},&expected));
        assert(memcmp(&expected,&pixels[row*3U+col],sizeof(expected))==0);
    }
    memset(pixels,0x55,sizeof(pixels));
    struct rgb24 unchanged[9]; memcpy(unchanged,pixels,sizeof(pixels));
    assert(!complex_plot_raster((struct complex_mapping){NULL,NULL},domain,pixels,9));
    assert(!complex_plot_raster((struct complex_mapping){NULL,identity},domain,pixels,8));
    domain.x_radius=NAN;
    assert(!complex_plot_raster((struct complex_mapping){NULL,identity},domain,pixels,9));
    assert(memcmp(unchanged,pixels,sizeof(pixels))==0);
    puts("PASS complex mapping, projection, independent polynomial colour, readonly model and invalid domain");
}
