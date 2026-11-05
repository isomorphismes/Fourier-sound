#ifndef FOURIER_CHARCOAL_WINDOW_H
#define FOURIER_CHARCOAL_WINDOW_H
#include "charcoal.h"
#include <android/native_window.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Blank microphone/speaker diagnostics have no scientific frame producer.
   Paint once when the native surface arrives; never add a recurring draw loop. */
static bool fourier_charcoal_window(ANativeWindow *window)
{
    if (!window || ANativeWindow_setBuffersGeometry(window,0,0,WINDOW_FORMAT_RGBA_8888))
        return false;
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window,&buffer,NULL)) return false;
    for (int32_t y←0;y<buffer.height;++y) {
        uint8_t *row←(uint8_t *)buffer.bits+(size_t)y*(size_t)buffer.stride*4;
        for (int32_t x←0;x<buffer.width;++x) {
            row[x*4]←(uint8_t)(CHARCOAL_BACKGROUND>>16);
            row[x*4+1]←(uint8_t)(CHARCOAL_BACKGROUND>>8);
            row[x*4+2]←(uint8_t)CHARCOAL_BACKGROUND;
            row[x*4+3]←255;
        }
    }
    return ANativeWindow_unlockAndPost(window)==0;
}
#endif
