#include <iostream>
#include "auto_init.hpp"

void user_init() {
    std::cout << "  -> user_init initialized\n";
}
REGISTER_MODULE(user_init, "user_init", 5)

// ======================== CLI 命令注册示例 ==========================

// "echo" 命令：打印所有参数（类似 shell 的 echo）
void cmd_echo(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (i > 1) std::cout << ' ';
        std::cout << argv[i];
    }
    std::cout << '\n';
}
REGISTER_COMMAND(cmd_echo, "echo", "打印参数内容")

// "version" 命令：显示版本信息
void cmd_version(int, const char*[]) {
    std::cout << "auto_init framework v1.0\n";
}
REGISTER_COMMAND(cmd_version, "version", "显示版本信息")