#include "math_utils.h"

// 这个文件会被编译成静态库 libmath_utils.a（见 CMakeLists.txt 中的 STATIC），
// 然后在链接阶段被"打进" libndk.so，APK 里不会单独出现这个 .a

int mu_square(int x) {
    return x * x;
}

long mu_factorial(int n) {
    long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}
