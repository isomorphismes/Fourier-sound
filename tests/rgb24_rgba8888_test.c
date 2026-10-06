#include "rgb24_rgba8888.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void stride_and_alpha(void)
{
    struct rgb24 pixels[] = {
        {1, 2, 3}, {4, 5, 6},
        {7, 8, 9}, {10, 11, 12}
    };
    unsigned char destination[2U * 3U * 4U];
    memset(destination, 0xcc, sizeof(destination));

    assert(rgb24_copy_rgba8888(
        pixels, 2U, 2U, 4U, destination, 3U, 2U));

    const unsigned char expected_first[] = {
        1, 2, 3, 255, 4, 5, 6, 255
    };
    const unsigned char expected_second[] = {
        7, 8, 9, 255, 10, 11, 12, 255
    };
    assert(memcmp(destination, expected_first, sizeof(expected_first)) == 0);
    assert(destination[8] == 0xcc && destination[9] == 0xcc &&
           destination[10] == 0xcc && destination[11] == 0xcc);
    assert(memcmp(destination + 12, expected_second,
                  sizeof(expected_second)) == 0);
    assert(destination[20] == 0xcc && destination[21] == 0xcc &&
           destination[22] == 0xcc && destination[23] == 0xcc);
}

static void rejected_inputs(void)
{
    struct rgb24 pixel = {1, 2, 3};
    unsigned char destination[8] = {0};

    assert(!rgb24_copy_rgba8888(
        NULL, 1U, 1U, 1U, destination, 1U, 1U));
    assert(!rgb24_copy_rgba8888(
        &pixel, 1U, 1U, 1U, NULL, 1U, 1U));
    assert(!rgb24_copy_rgba8888(
        &pixel, 0U, 1U, 1U, destination, 1U, 1U));
    assert(!rgb24_copy_rgba8888(
        &pixel, 2U, 1U, 1U, destination, 2U, 1U));
    assert(!rgb24_copy_rgba8888(
        &pixel, 2U, 1U, 2U, destination, 1U, 1U));
    assert(!rgb24_copy_rgba8888(
        &pixel, 1U, 2U, 2U, destination, 1U, 1U));
}

int main(void)
{
    stride_and_alpha();
    rejected_inputs();
    puts("PASS RGB24 to RGBA8888 channel order, alpha, stride padding and bounds");
    return 0;
}
