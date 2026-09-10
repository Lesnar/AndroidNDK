//
// Created by mwang on 9/9/26.
//
// ======== Native 线程附加 JVM 演示 ========
// pthread 创建的子线程默认不在 JVM 里注册，直接用 JNIEnv* 会崩溃：
// JNIEnv 是线程局部的，只有通过 AttachCurrentThread 拿到的 env 才能在该线程使用。
// 流程：AttachCurrentThread -> 用 env 调 Java 方法 -> DetachCurrentThread。
//
#include <jni.h>
#include <pthread.h>
#include <thread>
#include <android/log.h>

#define LOG_TAG "NDK_DEMO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// 在 native-lib.cpp 的 JNI_OnLoad 里已经保存过，这里直接复用
extern JavaVM *gJavaVM;

// 子线程只能带走 JVM 层对象的“全局引用”，局部引用出了创建它的那次 JNI 调用就失效了
struct ThreadArgs {
    jobject activityGlobalRef;
};

static void *threadEntry(void *rawArgs) {
    auto *args = static_cast<ThreadArgs *>(rawArgs);

    JNIEnv *env = nullptr;
    // 子线程首次接触 JVM，必须先 attach，才能拿到可用的 JNIEnv*
    int attachResult = gJavaVM->AttachCurrentThread(&env, nullptr);
    if (attachResult != JNI_OK) {
        LOGE("AttachCurrentThread failed: %d", attachResult);
        delete args;
        return nullptr;
    }

    LOGI("native thread attached, calling back into Java");

    jclass activityClass = env->GetObjectClass(args->activityGlobalRef);
    jmethodID showMessage = env->GetMethodID(
            activityClass, "showMessage", "(Ljava/lang/String;)V");
    if (showMessage != nullptr) {
        jstring message = env->NewStringUTF("hello from native thread");
        env->CallVoidMethod(args->activityGlobalRef, showMessage, message);
        env->DeleteLocalRef(message);
    }
    env->DeleteLocalRef(activityClass);

    // 用完即释放全局引用，避免内存泄漏
    env->DeleteGlobalRef(args->activityGlobalRef);
    delete args;

    // 线程退出前必须 detach，否则这个线程会一直占用 JVM 分配的线程结构
    gJavaVM->DetachCurrentThread();
    LOGI("native thread detached");
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_series_ndk_MainActivity_nativeThreadDemo(JNIEnv *env, jobject thiz) {
    // thiz 是局部引用，只在当前这次 JNI 调用里有效；
    // 要带到子线程里用，必须先升级成全局引用
    auto *args = new ThreadArgs{env->NewGlobalRef(thiz)};

    pthread_t thread;
    pthread_create(&thread, nullptr, threadEntry, args);
    // 演示用途，不 join，主线程直接返回，子线程后台异步跑完自己 detach
    pthread_detach(thread);
}

// ======== std::thread 版本：逻辑和上面 pthread 版完全一样，写法更 C++ ========
// std::thread 底层在 Android/NDK 上也是包了一层 pthread，AttachCurrentThread /
// DetachCurrentThread 该做的事一样都不能少，区别只是线程创建、传参、生命周期管理的写法。
extern "C"
JNIEXPORT void JNICALL
Java_com_series_ndk_MainActivity_nativeStdThreadDemo(JNIEnv *env, jobject thiz) {
    // lambda 直接按值捕获全局引用，不用像 pthread 那样手动打包成 ThreadArgs* 再 new/delete
    jobject activityGlobalRef = env->NewGlobalRef(thiz);

    std::thread worker([activityGlobalRef]() {
        JNIEnv *threadEnv = nullptr;
        int attachResult = gJavaVM->AttachCurrentThread(&threadEnv, nullptr);
        if (attachResult != JNI_OK) {
            LOGE("std::thread AttachCurrentThread failed: %d", attachResult);
            return;
        }

        LOGI("std::thread attached, calling back into Java");

        jclass activityClass = threadEnv->GetObjectClass(activityGlobalRef);
        jmethodID showMessage = threadEnv->GetMethodID(
                activityClass, "showMessage", "(Ljava/lang/String;)V");
        if (showMessage != nullptr) {
            jstring message = threadEnv->NewStringUTF("hello from std::thread");
            threadEnv->CallVoidMethod(activityGlobalRef, showMessage, message);
            threadEnv->DeleteLocalRef(message);
        }
        threadEnv->DeleteLocalRef(activityClass);
        threadEnv->DeleteGlobalRef(activityGlobalRef);

        gJavaVM->DetachCurrentThread();
        LOGI("std::thread detached");
    });

    // std::thread 析构前必须 join() 或 detach()，否则程序直接 std::terminate。
    // 这里不需要等结果，所以 detach，让它后台自己跑完。
    worker.detach();
}
