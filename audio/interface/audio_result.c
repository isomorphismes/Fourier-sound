#include "audio_input.h"
const char *audio_result_text(enum audio_result result)
{
    switch (result) {
    case AUDIO_OK: return "audio input ready";
    case AUDIO_TIMEOUT: return "no audio arrived before the deadline";
    case AUDIO_INVALID: return "invalid audio argument or lifecycle state";
    case AUDIO_NO_MEMORY: return "audio buffer allocation failed";
    case AUDIO_PERMISSION: return "microphone permission denied";
    case AUDIO_UNAVAILABLE: return "microphone input unavailable";
    case AUDIO_UNSUPPORTED: return "audio format or lock-free atomics unsupported";
    case AUDIO_DISCONNECTED: return "audio input disconnected; close and reopen";
    case AUDIO_SYSTEM_ERROR: return "audio platform operation failed";
    }
    return "unknown audio error";
}
