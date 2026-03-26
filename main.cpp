// main.cpp
// ==========================================================================
// 自动初始化 & CLI 命令行演示程序
//
// 各模块在自己的 .cpp 中通过 INIT_EXPORT / MSH_CMD_EXPORT 完成自注册，
// 由 C++ 静态初始化保证在 main() 之前注册完毕。
// 新增模块/命令时无需修改此文件。
// ==========================================================================

#include "initcall.hpp"

int main() {
    // 按优先级执行所有已注册模块的初始化
    initcall::do_auto_init();
    std::cout << "=== Initialization complete ===\n\n";

    // 进入交互式 CLI（支持 Tab 补全和历史记录）
    initcall::cli_loop();

    return 0;
}