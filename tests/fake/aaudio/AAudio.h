/* Host test double: never Android build or hardware evidence. Values need only
 * be distinct here. Android compilation uses the real pinned NDK header. */
#ifndef TEST_AAUDIO_H
#define TEST_AAUDIO_H
#include <stdint.h>
typedef int32_t aaudio_result_t;
typedef int32_t aaudio_format_t;
typedef int32_t aaudio_stream_state_t;
typedef int32_t aaudio_data_callback_result_t;
typedef struct AAudioStream AAudioStream;
typedef struct AAudioStreamBuilder AAudioStreamBuilder;
enum { AAUDIO_OK = 0, AAUDIO_UNSPECIFIED = 0, AAUDIO_DIRECTION_INPUT = 1,
       AAUDIO_SHARING_MODE_SHARED = 1, AAUDIO_FORMAT_PCM_FLOAT = 2,
       AAUDIO_FORMAT_PCM_I16 = 1, AAUDIO_CALLBACK_RESULT_CONTINUE = 0,
       AAUDIO_STREAM_STATE_STARTED = 4, AAUDIO_STREAM_STATE_STOPPING = 9, AAUDIO_STREAM_STATE_STOPPED = 10,
       AAUDIO_STREAM_STATE_DISCONNECTED = 13 };
enum { AAUDIO_ERROR_DISCONNECTED = -1, AAUDIO_ERROR_NO_MEMORY = -2,
       AAUDIO_ERROR_UNAVAILABLE = -3, AAUDIO_ERROR_INVALID_FORMAT = -4,
       AAUDIO_ERROR_INVALID_RATE = -5, AAUDIO_ERROR_INVALID_STATE = -6,
       AAUDIO_ERROR_ILLEGAL_ARGUMENT = -7, AAUDIO_ERROR_TIMEOUT = -8 };
typedef aaudio_data_callback_result_t (*data_callback)(AAudioStream *, void *, void *, int32_t);
typedef void (*error_callback)(AAudioStream *, void *, aaudio_result_t);
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *, int32_t);
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder *, int32_t);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *, int32_t);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *, int32_t);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *, aaudio_format_t);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *, data_callback, void *);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *, error_callback, void *);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *, AAudioStream **);
int32_t AAudioStream_getSampleRate(AAudioStream *);
int32_t AAudioStream_getChannelCount(AAudioStream *);
aaudio_format_t AAudioStream_getFormat(AAudioStream *);
aaudio_result_t AAudioStream_requestStart(AAudioStream *);
aaudio_result_t AAudioStream_requestStop(AAudioStream *);
aaudio_stream_state_t AAudioStream_getState(AAudioStream *);
aaudio_result_t AAudioStream_waitForStateChange(AAudioStream *, aaudio_stream_state_t, aaudio_stream_state_t *, int64_t);
aaudio_result_t AAudioStream_close(AAudioStream *);
#endif
