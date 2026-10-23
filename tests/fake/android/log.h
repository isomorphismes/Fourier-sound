#ifndef FOURIER_TEST_ANDROID_LOG_H
#define FOURIER_TEST_ANDROID_LOG_H
#define ANDROID_LOG_INFO 4
int __android_log_print(int priority, const char *tag, const char *format, ...);
#endif
