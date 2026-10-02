#include "audio_input.h"
const char *audio_result_text(enum audio_result result)
{
    switch (result) {
    case AUDIO_OK: return "audio ready";
    case AUDIO_TIMEOUT: return "audio operation timed out";
    case AUDIO_INVALID: return "invalid audio argument or lifecycle state";
    case AUDIO_NO_MEMORY: return "audio buffer allocation failed";
    case AUDIO_PERMISSION: return "audio permission denied";
    case AUDIO_UNAVAILABLE: return "audio device unavailable";
    case AUDIO_UNSUPPORTED: return "audio format or required atomics unsupported";
    case AUDIO_DISCONNECTED: return "audio device disconnected; close and reopen";
    case AUDIO_SYSTEM_ERROR: return "audio platform operation failed";
    }
    return "unknown audio error";
}
