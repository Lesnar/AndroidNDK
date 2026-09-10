//
// Created by mwang on 9/9/26.
//

// ======== std::vector 用法演示（Logcat 版）========
// vector 是 C++ 的动态数组：长度可变、内存连续、自动管理生命周期，
// 相当于一个会自己 new[] / delete[] 的智能容器（RAII）。

#include <jni.h>
#include <vector>
#include <string>
#include <android/log.h>

// 和 native-lib.cpp 顶部的日志宏一致，Logcat 里按 NDK_DEMO 过滤
#define LOG_TAG "NDK_DEMO"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 演示用的简单类：构造/拷贝时会打日志，方便观察 push_back 和
// emplace_back 的区别（后者少一次拷贝）
class Item {
public:
    std::string name;
    explicit Item(std::string n) : name(std::move(n)) {
        LOGI("  构造 Item(%s)", name.c_str());
    }
    Item(const Item &other) : name(other.name) {
        LOGI("  拷贝 Item(%s)", name.c_str());
    }
};

void vectorDemo() {
    // ======== 创建与 size() ========
    std::vector<int> v;             // 空向量，长度 0
    LOGI("初始 size = %zu", v.size());   // size()：当前元素个数

    // ======== push_back()：在尾部追加一个"已存在/临时"的元素 ========
    v.push_back(10);                // 尾部追加 10
    v.push_back(20);
    v.push_back(30);
    LOGI("push_back 3 次后 size = %zu", v.size());  // 3

    // 遍历：可以用下标（内存连续，和 C 数组一样快）
    for (size_t i = 0; i < v.size(); i++) {
        LOGI("v[%zu] = %d", i, v[i]);
    }

    // ======== emplace_back()：在尾部"就地构造"元素，少一次拷贝/移动 ========
    std::vector<Item> items;

    LOGI("--- push_back ---");
    items.push_back(Item("A"));     // 先构造临时 Item，再拷贝进容器（可能被优化掉）

    LOGI("--- emplace_back ---");
    items.emplace_back("B");        // 直接在容器内部用 "B" 构造 Item，不产生临时对象

    // ======== clear()：清空所有元素，size 变 0 ========
    // 注意：clear 只删元素，通常不归还内存（capacity 保持），
    // 想真正释放内存可用 shrink_to_fit() 或 swap 惯用法
    v.clear();
    LOGI("clear 后 size = %zu", v.size());   // 0

    // ======== 其他常用操作（顺带一提） ========
    v.push_back(100);
    LOGI("front = %d, back = %d, empty = %d", // 第一个/最后一个元素、是否为空
         v.front(), v.back(), v.empty());
    v.pop_back();                                 // 删除尾部元素（无返回值）

    // 范围 for 遍历（最常用的读法）
    for (const auto &item : items) {
        LOGI("item: %s", item.name.c_str());
    }
}

// ======== JNI 入口：Java 层调用后，Logcat 过滤 NDK_DEMO 查看输出 ========
extern "C"
JNIEXPORT void JNICALL
Java_com_series_ndk_MainActivity_vectorDemo(JNIEnv *env, jobject thiz) {
    vectorDemo();
}
