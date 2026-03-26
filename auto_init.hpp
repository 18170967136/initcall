// auto_init.hpp
// ==========================================================================
// C++ 自动初始化 & CLI 命令注册框架（Header-Only）
//
// 灵感来源于 RT-Thread 的 INIT_XXX_EXPORT 和 finsh/msh 机制。
// 只需 #include 此文件即可使用全部功能，无需额外链接任何 .cpp。
//
// ======================== 模块自动初始化 ========================
//
//   #include "auto_init.hpp"
//
//   void my_module_init() { /* 初始化逻辑 */ }
//   REGISTER_MODULE(my_module_init, "My Module", 10)
//
//   // main.cpp 中调用：
//   do_auto_init();
//
// ======================== CLI 命令注册 ==========================
//
//   void cmd_hello(int argc, const char* argv[]) {
//       std::cout << "Hello!\n";
//   }
//   REGISTER_COMMAND(cmd_hello, "hello", "打印问候语")
//
//   // main.cpp 中调用：
//   cli_loop();          // 进入交互式命令行
//   // 或
//   cli_execute("hello");  // 直接执行一条命令
//
// ==========================================================================
//
// 多线程安全性说明：
//   所有 REGISTER_MODULE / REGISTER_COMMAND 在 main() 之前的静态初始化阶段
//   执行，此时只有一个线程（C++ 标准保证）。get_init_table() 和 get_cmd_table()
//   使用函数局部 static 变量，C++11 保证其构造是线程安全的（Magic Statics）。
//   因此，只要不在 main() 之后动态注册新模块/命令，无需加锁。
//
// ==========================================================================

#ifndef AUTO_INIT_HPP
#define AUTO_INIT_HPP

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>

// **************************************************************************
//  Part 1: 模块自动初始化
// **************************************************************************

// --------------------------------------------------------------------------
// 初始化函数指针类型
// --------------------------------------------------------------------------
using init_func_t = void(*)();

// --------------------------------------------------------------------------
// 模块描述符
// --------------------------------------------------------------------------
struct init_entry {
    init_func_t func;      // 指向模块初始化函数的指针
    const char* name;      // 模块名称（用于日志输出）
    int priority;          // 优先级，数值越小越先执行
};

// --------------------------------------------------------------------------
// 全局注册表（首次使用时构造，避免静态初始化顺序问题）
// --------------------------------------------------------------------------
inline std::vector<init_entry>& get_init_table() {
    static std::vector<init_entry> table;
    return table;
}

// --------------------------------------------------------------------------
// 模块注册宏
// --------------------------------------------------------------------------
// 用法：在初始化函数定义的下方，写一行：
//   REGISTER_MODULE(函数名, "模块名", 优先级)
//
// 展开后为一个 static bool 变量，通过立即执行的 lambda 在程序启动时
// 将模块信息 push 进 get_init_table()。
#define REGISTER_MODULE(func, name, prio) \
    static bool __reg_mod_##func = []() { \
        get_init_table().push_back({func, name, prio}); \
        return true; \
    }();

// --------------------------------------------------------------------------
// 执行所有已注册模块的初始化（按优先级升序）
// --------------------------------------------------------------------------
inline void do_auto_init() {
    auto& table = get_init_table();

    std::sort(table.begin(), table.end(),
              [](const init_entry& a, const init_entry& b) {
                  return a.priority < b.priority;
              });

    std::cout << "[auto_init] Found " << table.size() << " registered modules\n";

    for (const auto& entry : table) {
        std::cout << "[auto_init] Executing: " << entry.name
                  << " (priority=" << entry.priority << ")\n";
        entry.func();
    }
}

// **************************************************************************
//  Part 2: CLI 命令注册（类似 RT-Thread finsh/msh）
// **************************************************************************

// --------------------------------------------------------------------------
// 命令处理函数签名
// --------------------------------------------------------------------------
// 类似标准 main 的签名：argc 为参数个数，argv[0] 为命令名本身
using cmd_handler_t = void(*)(int argc, const char* argv[]);

// --------------------------------------------------------------------------
// 命令描述符
// --------------------------------------------------------------------------
struct cmd_entry {
    cmd_handler_t handler; // 命令处理函数
    const char* name;      // 命令名（用户在终端输入的字符串）
    const char* help;      // 帮助信息
};

// --------------------------------------------------------------------------
// 命令注册表（首次使用时构造）
// --------------------------------------------------------------------------
inline std::vector<cmd_entry>& get_cmd_table() {
    static std::vector<cmd_entry> table;
    return table;
}

// --------------------------------------------------------------------------
// 命令注册宏
// --------------------------------------------------------------------------
// 用法：在命令处理函数定义的下方，写一行：
//   REGISTER_COMMAND(函数名, "命令名", "帮助信息")
//
// 示例：
//   void cmd_reboot(int argc, const char* argv[]) { /* ... */ }
//   REGISTER_COMMAND(cmd_reboot, "reboot", "重启系统")
#define REGISTER_COMMAND(func, name, help) \
    static bool __reg_cmd_##func = []() { \
        get_cmd_table().push_back({func, name, help}); \
        return true; \
    }();

// --------------------------------------------------------------------------
// 将输入字符串按空格拆分为 argc / argv
// --------------------------------------------------------------------------
inline std::vector<const char*> cli_split(char* line) {
    std::vector<const char*> tokens;
    char* tok = std::strtok(line, " \t\r\n");
    while (tok) {
        tokens.push_back(tok);
        tok = std::strtok(nullptr, " \t\r\n");
    }
    return tokens;
}

// --------------------------------------------------------------------------
// 内置 help 命令
// --------------------------------------------------------------------------
inline void cmd_builtin_help(int, const char*[]) {
    auto& table = get_cmd_table();
    std::cout << "Available commands:\n";
    for (const auto& cmd : table) {
        std::cout << "  ";
        std::string name_str(cmd.name);
        std::cout << name_str;
        if (name_str.size() < 16) {
            std::cout << std::string(16 - name_str.size(), ' ');
        }
        std::cout << cmd.help << "\n";
    }
}

// --------------------------------------------------------------------------
// 确保内置命令只注册一次
// --------------------------------------------------------------------------
inline std::vector<cmd_entry>& get_cmd_table_with_builtins() {
    auto& table = get_cmd_table();
    static bool builtins_registered = []() {
        get_cmd_table().insert(get_cmd_table().begin(),
            {cmd_builtin_help, "help", "\u663e\u793a\u6240\u6709\u53ef\u7528\u547d\u4ee4"});
        return true;
    }();
    (void)builtins_registered;
    return table;
}

// --------------------------------------------------------------------------
// 在命令表中查找并执行一条命令
// --------------------------------------------------------------------------
inline bool cli_execute(const std::string& line) {
    if (line.empty()) return false;

    std::vector<char> buf(line.begin(), line.end());
    buf.push_back('\0');

    auto tokens = cli_split(buf.data());
    if (tokens.empty()) return false;

    const char* cmd_name = tokens[0];
    auto& table = get_cmd_table_with_builtins();

    for (const auto& cmd : table) {
        if (std::strcmp(cmd.name, cmd_name) == 0) {
            cmd.handler(static_cast<int>(tokens.size()), tokens.data());
            return true;
        }
    }

    std::cout << "Unknown command: " << cmd_name
              << "  (type 'help' for available commands)\n";
    return false;
}

// --------------------------------------------------------------------------
// 交互式命令行循环
// --------------------------------------------------------------------------
// 持续从 stdin 读取用户输入，解析并执行命令。
// 输入 "exit" 或 "quit" 退出循环，或按 Ctrl+D (EOF) 退出。
//
// prompt: 提示符字符串，默认为 "msh> "（致敬 RT-Thread msh）
inline void cli_loop(const char* prompt = "msh> ") {
    std::string line;
    while (true) {
        std::cout << prompt << std::flush;

        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl+D)
            std::cout << "\n";
            break;
        }

        // 跳过空行
        if (line.empty()) continue;

        // exit / quit 退出
        if (line == "exit" || line == "quit") break;

        cli_execute(line);
    }
}

#endif // AUTO_INIT_HPP
