#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());

//    const char *cstr = "Hello from C";
//    return env->NewStringUTF(cstr);

//    return env->NewStringUTF("Hell from!");
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_series_ndk_MainActivity_add(JNIEnv *env, jobject thiz, jint a, jint b) {
    return a + b;
}
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_series_ndk_MainActivity_multiply(JNIEnv *env, jobject thiz, jdouble a, jdouble b) {
    return a * b;
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_sayHello(JNIEnv *env, jobject thiz, jstring name) {
    const char *text = env->GetStringUTFChars(name, nullptr);
    std::string result = "Hello ";
    result += text;
    env->ReleaseStringUTFChars(name, text);
    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_series_ndk_MainActivity_isPositive(JNIEnv *env, jobject thiz, jint value) {
    return value > 10;
}