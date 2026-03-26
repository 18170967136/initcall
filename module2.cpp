// module2.cpp
// ==========================================================================
// 模块 B 和模块 C —— 一个文件中也可以注册多个模块
// 每个函数下方各自跟一行 AUTOREG_MODULE 即可。
// ==========================================================================

#include <iostream>
#include "auto_init.hpp"

// Module B 的初始化函数
void module_b_init() {
    std::cout << "  -> Module B initialized\n";
}
// ↓ 注册：优先级 10（最先执行）
AUTOREG_MODULE(module_b_init, "Module B", 10)

// Module C 的初始化函数
void module_c_init() {
    std::cout << "  -> Module C initialized\n";
}
// ↓ 注册：优先级 20（中间执行）
AUTOREG_MODULE(module_c_init, "Module C", 20)