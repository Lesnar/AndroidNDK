#include <jni.h>
#include <string>
#include <iostream>
#include <android/log.h>

// ======== 日志宏 ========
// LOG_TAG 是固定的标签，Logcat 里按它过滤；四个宏分别对应 Log.e/w/i/d，
// __VA_ARGS__ 把 "fmt", args... 原样转发给 __android_log_print
#define LOG_TAG "NDK_DEMO"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// 引入静态库的头文件。链接关系在 CMakeLists.txt 里声明（target_link_libraries），
// 这里只需要能找到声明；真正 mu_factorial 的实现来自 libmath_utils.a
#include "math_utils/math_utils.h"

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
extern "C"
JNIEXPORT jint JNICALL
Java_com_series_ndk_MainActivity_sumArray(JNIEnv *env, jobject thiz, jintArray values) {
    jsize length = env->GetArrayLength(values);
    jint *data =
            env->GetIntArrayElements(values, nullptr);
    jint sum = 0;
    for (int i = 0; i < length; i++) {
        sum += data[i];
    }
    env->ReleaseIntArrayElements(
            values,
            data,
            JNI_ABORT
    );
    return sum;
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_series_ndk_MainActivity_createArray(JNIEnv *env, jobject thiz, jint size) {
    // 1. 在 JVM 中创建一个长度为 size 的 int 数组（元素初始为 0）
    jintArray result = env->NewIntArray(size);
    if (result == nullptr) {
        return nullptr; // 内存不足时创建失败，异常已由 JVM 抛出
    }

    // 2. 准备 C 侧数据
    jint buf[size];
    for (int i = 0; i < size; i++) {
        buf[i] = i * i; // 示例数据：平方数 0, 1, 4, 9, ...
    }

    // 3. 把 C 数组内容复制进 Java 数组
    env->SetIntArrayRegion(result, 0, size, buf);

    return result;
}
extern "C"
JNIEXPORT jintArray JNICALL
Java_com_series_ndk_MainActivity_multiplyArray(JNIEnv *env, jobject thiz, jintArray values) {
    jsize length = env->GetArrayLength(values);

    // 1. 把 Java 数组内容拷贝到 C 侧缓冲区
    jint buf[length];
    env->GetIntArrayRegion(values, 0, length, buf);

    // 2. 每个元素乘 2
    for (int i = 0; i < length; i++) {
        buf[i] *= 2;
    }

    // 3. 创建新的 Java 数组并填入结果（不修改原数组）
    jintArray result = env->NewIntArray(length);
    if (result == nullptr) {
        return nullptr;
    }
    env->SetIntArrayRegion(result, 0, length, buf);

    return result;
}
extern "C"
JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_getUserInfo(JNIEnv *env, jobject thiz, jobject user) {
    // 1. 拿到 User 类和它的字段 ID
    jclass userClass = env->GetObjectClass(user);
    jfieldID nameField = env->GetFieldID(userClass, "name", "Ljava/lang/String;");
    jfieldID ageField = env->GetFieldID(userClass, "age", "I");

    // 2. 读取字段值
    jstring name = static_cast<jstring>(env->GetObjectField(user, nameField));
    jint age = env->GetIntField(user, ageField);

    // 3. 拼接 "名字 - 年龄"
    const char *nameChars = env->GetStringUTFChars(name, nullptr);
    std::string result = nameChars;
    result += " - ";
    result += std::to_string(age);
    env->ReleaseStringUTFChars(name, nameChars);

//    调用 MainActivity 中的showMessage
    jstring message = env->NewStringUTF(result.c_str());
    jclass mainClass = env->GetObjectClass(thiz);
    jmethodID showMethod = env->GetMethodID(mainClass, "showMessage", "(Ljava/lang/String;)V");
    env->CallVoidMethod(thiz, showMethod, message);

    return env->NewStringUTF(result.c_str());
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_series_ndk_MainActivity_createUser(JNIEnv *env, jobject thiz) {
    // 1. 按全限定名找到 User 类（包名用 / 分隔）
    jclass userClass = env->FindClass("com/series/ndk/data/User");
    if (userClass == nullptr) {
        return nullptr; // 找不到类，异常已由 JVM 抛出
    }

    // 2. 拿到构造方法的 jmethodID，方法名固定为 <init>
    //    签名 (Ljava/lang/String;I)V 对应 User(name: String, age: Int)
    jmethodID constructor = env->GetMethodID(userClass, "<init>",
            "(Ljava/lang/String;I)V");
    if (constructor == nullptr) {
        return nullptr;
    }

    // 3. 调用构造方法创建对象：NewObject(类, 构造方法, 参数...)
    jstring name = env->NewStringUTF("Martin");
    jint age = 28;
    jobject user = env->NewObject(userClass, constructor, name, age);

    return user;
}

// ======== 静态库（.a）示例 ========
// 这两个函数的实现来自 libmath_utils.a：链接器在编译期把符号拷进 libndk.so，
// 运行时 app 只加载 libndk.so 一个库，感知不到 .a 的存在
extern "C"
JNIEXPORT jint JNICALL
Java_com_series_ndk_MainActivity_squareFromStaticLib(JNIEnv *env, jobject thiz, jint x) {
    jint result = mu_square(x);
    LOGI("squareFromStaticLib: mu_square(%d) = %d", x, result);
    return result;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_series_ndk_MainActivity_factorialFromStaticLib(JNIEnv *env, jobject thiz, jint n) {
    std::cout << "factorial input n = " << n << std::endl;
    return mu_factorial(n);
}

class User {
public:
    User(std::string name) : name_(std::move(name)) {
    }

    std::string getName() const {
        return name_;
    }

    std::string name_;
};


extern "C"
JNIEXPORT jstring JNICALL
Java_com_series_ndk_MainActivity_getUserFromCPP(JNIEnv *env, jobject thiz) {
    auto user = std::make_unique<User>("Martin from c++");
    std::string name = user->getName();
    return env->NewStringUTF(name.c_str());
}