#include <android/log.h>
#include <android/native_activity.h>
#include <stdbool.h>

/* Permission presentation must execute on the framework/UI thread. Acquisition
 * remains entirely AAudio/C. Only these existing framework methods use JNI. */
static bool exception(JNIEnv *env)
{
    if (!(*env)->ExceptionCheck(env)) return false;
    (*env)->ExceptionClear(env);
    __android_log_print(ANDROID_LOG_ERROR, "FourierMic", "microphone permission framework call failed");
    return true;
}
bool microphone_permission(ANativeActivity *activity)
{
    JNIEnv *env = NULL;
    bool attached = false;
    if ((*activity->vm)->GetEnv(activity->vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        if ((*activity->vm)->AttachCurrentThread(activity->vm, &env, NULL) != JNI_OK) return false;
        attached = true;
    }
    bool granted = false;
    if ((*env)->PushLocalFrame(env, 8) < 0) { (void)exception(env); goto done; }
    jclass type = (*env)->GetObjectClass(env, activity->clazz);
    if (exception(env) || !type) goto pop;
    jmethodID check = (*env)->GetMethodID(env, type, "checkSelfPermission", "(Ljava/lang/String;)I");
    if (exception(env) || !check) goto pop;
    jstring permission = (*env)->NewStringUTF(env, "android.permission.RECORD_AUDIO");
    if (exception(env) || !permission) goto pop;
    jint status = (*env)->CallIntMethod(env, activity->clazz, check, permission);
    granted = !exception(env) && status == 0;
pop:
    (*env)->PopLocalFrame(env, NULL);
done:
    if (attached) (*activity->vm)->DetachCurrentThread(activity->vm);
    return granted;
}
/* NDK glue's entry is renamed at compile time so this entry can request the
 * permission on the UI thread without an app Java/Kotlin class or DEX. */
void fourier_glue_on_create(ANativeActivity *, void *, size_t);
void ANativeActivity_onCreate(ANativeActivity *activity, void *saved, size_t length)
{
    fourier_glue_on_create(activity, saved, length);
    if (microphone_permission(activity)) return;
    JNIEnv *env = activity->env;
    if ((*env)->PushLocalFrame(env, 8) < 0) { (void)exception(env); return; }
    jclass type = (*env)->GetObjectClass(env, activity->clazz);
    if (exception(env) || !type) goto done;
    jmethodID request = (*env)->GetMethodID(env, type, "requestPermissions", "([Ljava/lang/String;I)V");
    if (exception(env) || !request) goto done;
    jclass string_type = (*env)->FindClass(env, "java/lang/String");
    if (exception(env) || !string_type) goto done;
    jstring permission = (*env)->NewStringUTF(env, "android.permission.RECORD_AUDIO");
    if (exception(env) || !permission) goto done;
    jobjectArray permissions = (*env)->NewObjectArray(env, 1, string_type, permission);
    if (exception(env) || !permissions) goto done;
    (*env)->CallVoidMethod(env, activity->clazz, request, permissions, (jint)1);
    (void)exception(env);
done:
    (*env)->PopLocalFrame(env, NULL);
}
