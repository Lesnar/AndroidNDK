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
#include <mutex>
#include <vector>
#include <string>
#include <queue>
#include <condition_variable>
#include <chrono>
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

// ======== std::mutex 演示：多线程抢同一个计数器 ========
// 这几个线程不碰 JNI，纯 C++ 层面的数据竞争，用来对比“加锁 vs 不加锁”的结果差异。
static constexpr int kThreadCount = 4;
static constexpr int kIncrementsPerThread = 100000;

static void incrementWithoutLock(int *counter) {
    for (int i = 0; i < kIncrementsPerThread; i++) {
        // 危险写法：多线程同时读-加-写同一个 int，没有任何同步手段
        (*counter)++;
    }
}

static void incrementWithLock(int *counter, std::mutex *mutex) {
    for (int i = 0; i < kIncrementsPerThread; i++) {
        // std::lock_guard 在构造时 lock()，析构（离开作用域）时自动 unlock()，
        // 即使中间抛异常也不会漏解锁，比手动 mutex->lock()/unlock() 更安全
        std::lock_guard<std::mutex> guard(*mutex);
        (*counter)++;
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_nativeMutexDemo(JNIEnv *env, jobject thiz) {
    const int expected = kThreadCount * kIncrementsPerThread;

    // 1. 不加锁：多个线程同时对同一个 int 做 ++，结果几乎必然小于期望值（丢计数）
    int unsafeCounter = 0;
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < kThreadCount; i++) {
            threads.emplace_back(incrementWithoutLock, &unsafeCounter);
        }
        for (auto &t: threads) {
            t.join(); // 这里要等所有线程跑完再读结果，所以用 join 而不是 detach
        }
    }

    // 2. 加锁：std::mutex 保证同一时刻只有一个线程能执行临界区（++counter），结果总是精确的
    int safeCounter = 0;
    std::mutex mutex;
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < kThreadCount; i++) {
            threads.emplace_back(incrementWithLock, &safeCounter, &mutex);
        }
        for (auto &t: threads) {
            t.join();
        }
    }

    LOGI("mutex demo: expected=%d, unsafeCounter=%d, safeCounter=%d",
         expected, unsafeCounter, safeCounter);

    // 这里没有另起线程调回 Java，仍在原来的 JNI 调用线程上，env 直接能用，无需 attach/detach
    std::string result = "expected=" + std::to_string(expected) +
                          ", 不加锁结果=" + std::to_string(unsafeCounter) +
                          ", 加锁结果=" + std::to_string(safeCounter);
    return env->NewStringUTF(result.c_str());
}

// ======== std::condition_variable 演示：生产者-消费者 ========
// mutex 只能保证“同一时刻只有一个线程进临界区”，但没法解决“消费者怎么知道队列里有新数据、
// 不用傻等（忙轮询）”的问题——这正是 condition_variable 要解决的：
// 线程在没有数据时挂起（不占 CPU），有数据时被 notify 唤醒。
static constexpr int kItemCount = 5;

struct ProducerConsumerState {
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<int> queue;
    bool finished = false; // 生产者已经生产完，消费者不用再等新数据了
};

static void producer(ProducerConsumerState *state) {
    for (int i = 1; i <= kItemCount; i++) {
        {
            // unique_lock 和 lock_guard 类似会自动解锁，但支持配合 cv 临时释放锁，
            // 所以 condition_variable 必须搭配 unique_lock 而不是 lock_guard
            std::unique_lock<std::mutex> lock(state->mutex);
            state->queue.push(i);
            LOGI("producer push %d", i);
        }
        // notify 必须在锁外调用也可以（这里在花括号外），避免被唤醒的线程立刻又阻塞在锁上
        state->cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 模拟生产耗时
    }

    {
        std::unique_lock<std::mutex> lock(state->mutex);
        state->finished = true;
    }
    state->cv.notify_one(); // 最后再唤醒一次，让消费者能退出等待、发现 finished=true
}

static void consumer(ProducerConsumerState *state, std::vector<int> *consumed) {
    while (true) {
        std::unique_lock<std::mutex> lock(state->mutex);
        // wait 第二个参数是谓词：条件不满足就释放锁并挂起，被 notify 唤醒后重新加锁并再判一次谓词
        // （防止“虚假唤醒”——操作系统偶尔会无缘无故唤醒等待线程）
        state->cv.wait(lock, [state] {
            return !state->queue.empty() || state->finished;
        });

        if (state->queue.empty()) {
            // 队列空了并且生产者已完成，没有更多数据可消费，退出
            break;
        }

        int value = state->queue.front();
        state->queue.pop();
        lock.unlock(); // 尽快解锁，处理数据不必占着锁
        LOGI("consumer pop %d", value);
        consumed->push_back(value);
    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_nativeConditionVariableDemo(JNIEnv *env, jobject thiz) {
    ProducerConsumerState state;
    std::vector<int> consumed;

    std::thread producerThread(producer, &state);
    std::thread consumerThread(consumer, &state, &consumed);

    producerThread.join();
    consumerThread.join();

    std::string result = "consumed order = ";
    for (int v: consumed) {
        result += std::to_string(v) + " ";
    }
    LOGI("condition_variable demo: %s", result.c_str());

    return env->NewStringUTF(result.c_str());
}
