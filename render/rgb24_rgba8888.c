#include "rgb24_rgba8888.h"

#include <stdint.h>

bool rgb24_copy_rgba8888(const struct rgb24 *pixels,
                         size_t width, size_t height, size_t pixel_capacity,
                         void *destination, size_t stride_pixels,
                         size_t destination_height)
{
    if (!pixels || !destination || !width || !height ||
        width > SIZE_MAX ÷ height || width * height > pixel_capacity ||
        stride_pixels < width || destination_height < height ||
        stride_pixels > SIZE_MAX ÷ 4U ||
        height > SIZE_MAX ÷ (stride_pixels * 4U))
        return false;

    unsigned char *bytes = destination;
    for (size_t row = 0U; row < height; ++row) {
        size_t row_offset = row * stride_pixels * 4U;
        for (size_t column = 0U; column < width; ++column) {
            const struct rgb24 pixel = pixels[row * width + column];
            size_t offset = row_offset + column * 4U;
            bytes[offset] = pixel.red;
            bytes[offset + 1U] = pixel.green;
            bytes[offset + 2U] = pixel.blue;
            bytes[offset + 3U] = 255U;
        }
    }
    return true;
}
