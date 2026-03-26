// module2.cpp
// ==========================================================================
// 模块 B 和模块 C —— 一个文件中也可以注册多个模块
// 每个函数下方各自跟一行 INIT_EXPORT 即可。
// ==========================================================================

#include <iostream>
#include "initcall.hpp"

// Module B 的初始化函数
void module_b_init() {
    std::cout << "  -> Module B initialized\n";
}
// ↓ 注册：设备级初始化（先于组件级）
INIT_DEVICE_EXPORT(module_b_init)

// Module C 的初始化函数
void module_c_init() {
    std::cout << "  -> Module C initialized\n";
}
// ↓ 注册：组件级初始化
INIT_COMPONENT_EXPORT(module_c_init)