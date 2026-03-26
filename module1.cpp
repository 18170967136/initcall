// module1.cpp
// ==========================================================================
// 模块 A —— 自注册示例（类似 RT-Thread 的 INIT_XXX_EXPORT 风格）
//
// 使用方式：
//   1) 定义初始化函数
//   2) 在函数下方紧跟 REGISTER_MODULE 宏完成注册
//   3) 无需修改 main.cpp，编译链接后自动生效
// ==========================================================================

#include <iostream>
#include "auto_init.h"   // 引入注册框架

// Module A 的初始化函数
// 在实际项目中，这里可以执行硬件初始化、配置加载、服务启动等操作
void module_a_init() {
    std::cout << "  -> Module A initialized\n";
}
// ↓ 注册：优先级 30（数值越小越先执行，所以 Module A 最后执行）
REGISTER_MODULE(module_a_init, "Module A", 30)