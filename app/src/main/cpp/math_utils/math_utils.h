#ifndef ANDROIDNDK_MATH_UTILS_H
#define ANDROIDNDK_MATH_UTILS_H

// 纯 C 接口的工具库：extern "C" 让 C++ 编译器不修改函数名（不做 name mangling），
// 这样无论 .a 被 C 还是 C++ 代码链接，符号名都一致
#ifdef __cplusplus
extern "C" {
#endif

// 求平方
int mu_square(int x);

// 求阶乘
long mu_factorial(int n);

#ifdef __cplusplus
}
#endif

#endif //ANDROIDNDK_MATH_UTILS_H
