#include "ppm.h"

#include <stdint.h>
#include <stdio.h>

bool rgb24_write_ppm(const char *path, const struct rgb24 *pixels,
                     size_t width, size_t height, size_t capacity)
{
    if (!path || !pixels || !width || !height ||
        width > SIZE_MAX / height) return false;

    size_t count = width * height;
    if (capacity < count) return false;

    FILE *file = fopen(path, "wb");
    if (!file) return false;
    if (fprintf(file, "P6\n%zu %zu\n255\n", width, height) < 0) {
        fclose(file);
        return false;
    }

    bool ok = true;
    for (size_t index = 0; index < count; ++index) {
        unsigned char bytes[3] = {
            pixels[index].red,
            pixels[index].green,
            pixels[index].blue
        };
        if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
            ok = false;
            break;
        }
    }

    if (fclose(file) != 0) ok = false;
    return ok;
}
