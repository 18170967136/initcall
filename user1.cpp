#include <iostream>
#include "initcall.hpp"

void user_init() {
    std::cout << "  -> user_init initialized\n";
}
INIT_BOARD_EXPORT(user_init)

// ======================== CLI 命令注册示例 ==========================

// "echo" 命令：打印所有参数（类似 shell 的 echo）
void cmd_echo(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (i > 1) std::cout << ' ';
        std::cout << argv[i];
    }
    std::cout << '\n';
}
MSH_CMD_EXPORT(cmd_echo, "打印参数内容")