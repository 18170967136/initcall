// module2.cpp
// ==========================================================================
// 模块 B 和模块 C —— 一个文件中也可以注册多个模块
// 每个函数下方各自跟一行 REGISTER_MODULE 即可。
// ==========================================================================

#include <iostream>
#include "auto_init.hpp"

// Module B 的初始化函数
void module_b_init() {
    std::cout << "  -> Module B initialized\n";
}
// ↓ 注册：优先级 10（最先执行）
REGISTER_MODULE(module_b_init, "Module B", 10)

// Module C 的初始化函数
void module_c_init() {
    std::cout << "  -> Module C initialized\n";
}
// ↓ 注册：优先级 20（中间执行）
REGISTER_MODULE(module_c_init, "Module C", 20)

// ======================== CLI 命令注册示例 ==========================

// "list" 命令：列出所有已注册的初始化模块
void cmd_list(int, const char*[]) {
    auto& table = get_init_table();
    std::cout << "Registered modules (" << table.size() << "):\n";
    for (const auto& entry : table) {
        std::cout << "  [" << entry.priority << "] " << entry.name << "\n";
    }
}
REGISTER_COMMAND(cmd_list, "list", "列出所有已注册的初始化模块")