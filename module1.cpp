// module1.cpp
// ==========================================================================
// 模块 A —— 自注册示例（类似 RT-Thread 的 INIT_XXX_EXPORT 风格）
//
// 使用方式：
//   1) 定义初始化函数
//   2) 在函数下方紧跟 INIT_EXPORT 宏完成注册
//   3) 无需修改 main.cpp，编译链接后自动生效
// ==========================================================================

#include <iostream>
#include "initcall.hpp"

// Module A 的初始化函数
// 在实际项目中，这里可以执行硬件初始化、配置加载、服务启动等操作
void module_a_init() {
    std::cout << "  -> Module A initialized\n";
}
// ↓ 注册：组件级初始化
INIT_COMPONENT_EXPORT(module_a_init)