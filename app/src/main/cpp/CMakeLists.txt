# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("ndk")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        vector.cpp
        native_thread.cpp)

# ======== 静态库示例 ========
# STATIC 表示把 math_utils.c 编成 libmath_utils.a（SHARED 则是编成 .so）。
# .a 不会打进 APK、也不会被 System.loadLibrary 加载——
# 它在"链接阶段"被拷进 libndk.so，成为后者的一部分。
add_library(math_utils STATIC
        math_utils/math_utils.c)

# 如果拿到的是别人编译好的现成 .a 文件（第三方 SDK 常见），不用上面的方式，
# 而是用 IMPORTED 声明一个"导入型"库，直接指向 .a 的路径：
#
#   add_library(foo STATIC IMPORTED)
#   set_target_properties(foo PROPERTIES IMPORTED_LOCATION
#           ${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}/libfoo.a)
#
# 注意：每个 ABI（arm64-v8a / armeabi-v7a / ...）都要有对应的一份 .a，
# 因为它们互不兼容。

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        math_utils
        android
        log)