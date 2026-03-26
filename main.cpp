// main.cpp
// ==========================================================================
// 自动初始化 & CLI 命令行演示程序
//
// 核心思路（类似 RT-Thread 的 INIT_XXX_EXPORT + finsh/msh）：
//   1) 各模块在自己的 .cpp 中用 REGISTER_MODULE 完成自注册
//   2) 各模块在自己的 .cpp 中用 REGISTER_COMMAND 注册 CLI 命令
//   3) main() 中调用 do_auto_init() 执行所有模块初始化
//   4) main() 中调用 cli_loop() 进入交互式命令行
//
// 新增模块/命令时无需修改此文件。
// ==========================================================================

#include "auto_init.hpp"

int main() {
    std::cout << "=== Auto Initialization Demo ===\n";

    // 按优先级执行所有已注册的初始化函数
    do_auto_init();

    std::cout << "=== Initialization complete ===\n\n";

    // 进入交互式命令行（输入 help 查看命令列表，输入 exit 退出）
    cli_loop();

    return 0;
}