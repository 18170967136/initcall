// initcall.hpp
// ==========================================================================
// initcall —— C++ 自动初始化 & CLI 命令注册框架（Header-Only）
//
// 灵感来源于 RT-Thread 的 INIT_XXX_EXPORT 和 finsh/msh 机制。
// 只需 #include 此文件即可使用全部功能，无需额外链接任何 .cpp。
//
// 所有类型和函数位于 namespace initcall 中，避免命名冲突。
// 宏命名沿用嵌入式惯例：INIT_EXPORT / MSH_CMD_EXPORT / INIT_MAIN。
//
// ======================== 模块自动初始化 ========================
//
//   #include "initcall.hpp"
//
//   void my_module_init() { /* 初始化逻辑 */ }
//   INIT_EXPORT(my_module_init, "My Module", 10)
//
// ======================== CLI 命令注册 ==========================
//
//   void cmd_hello(int argc, const char* argv[]) {
//       std::cout << "Hello!\n";
//   }
//   MSH_CMD_EXPORT(cmd_hello, "hello", "打印问候语")
//
// ======================== 自动入口 ==============================
//
//   // main.cpp 中只需写：
//   #include "initcall.hpp"
//   INIT_MAIN()
//
//   // 效果：
//   //   1) 所有 INIT_EXPORT 注册的模块在 main() 之前自动初始化
//   //   2) main() 自动进入交互式 CLI 命令行（支持 Tab 补全和历史记录）
//
// ==========================================================================
//
// 多线程安全性说明：
//   所有 INIT_EXPORT / MSH_CMD_EXPORT 在 main() 之前的静态初始化阶段
//   执行，此时只有一个线程（C++ 标准保证）。get_init_table() 和 get_cmd_table()
//   使用函数局部 static 变量，C++11 保证其构造是线程安全的（Magic Statics）。
//   因此，只要不在 main() 之后动态注册新模块/命令，无需加锁。
//
// ==========================================================================

#ifndef INITCALL_HPP
#define INITCALL_HPP

#define INITCALL_VERSION  "1.0.0"
#define INITCALL_AUTHOR   "zeal"
#define INITCALL_DATE     "2026-03-26"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cstdio>

// antirez/linenoise（行编辑库：Tab 补全 / 历史 / 方向键）
extern "C" {
#include "linenoise.h"
}

namespace initcall {

// **************************************************************************
//  Part 1: 模块自动初始化
// **************************************************************************

// 初始化函数指针类型
using init_func_t = void(*)();

// 模块描述符
struct init_entry {
    init_func_t func;      // 指向模块初始化函数的指针
    const char* name;      // 模块名称（用于日志输出）
    int priority;          // 优先级，数值越小越先执行
};

// 全局注册表（首次使用时构造，避免静态初始化顺序问题）
inline std::vector<init_entry>& get_init_table() {
    static std::vector<init_entry> table;
    return table;
}

// 执行所有已注册模块的初始化（按优先级升序）
// 内置一次性守卫，防止被多次调用
inline void do_auto_init() {
    static bool done = false;
    if (done) return;
    done = true;

    auto& table = get_init_table();

    std::sort(table.begin(), table.end(),
              [](const init_entry& a, const init_entry& b) {
                  return a.priority < b.priority;
              });

    std::cout << "[initcall] Found " << table.size() << " registered modules\n";

    for (const auto& entry : table) {
        std::cout << "[initcall] Executing: " << entry.name
                  << " (priority=" << entry.priority << ")\n";
        entry.func();
    }
}

// **************************************************************************
//  Part 2: CLI 命令注册（类似 RT-Thread finsh/msh）
// **************************************************************************

// 命令处理函数签名（类似标准 main：argc 为参数个数，argv[0] 为命令名本身）
using cmd_handler_t = void(*)(int argc, const char* argv[]);

// 命令描述符
struct cmd_entry {
    cmd_handler_t handler; // 命令处理函数
    const char* name;      // 命令名（用户在终端输入的字符串）
    const char* help;      // 帮助信息
};

// 命令注册表（首次使用时构造）
inline std::vector<cmd_entry>& get_cmd_table() {
    static std::vector<cmd_entry> table;
    return table;
}

// 将输入字符串按空格拆分为 argc / argv
inline std::vector<const char*> cli_split(char* line) {
    std::vector<const char*> tokens;
    char* tok = std::strtok(line, " \t\r\n");
    while (tok) {
        tokens.push_back(tok);
        tok = std::strtok(nullptr, " \t\r\n");
    }
    return tokens;
}

// ======================== 内置命令 ========================

// help —— 显示所有可用命令
inline void cmd_builtin_help(int, const char*[]) {
    // 前置声明：get_cmd_table_with_builtins 在下方定义
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

// clear —— 清屏
inline void cmd_builtin_clear(int, const char*[]) {
    linenoiseClearScreen();
}

// list —— 列出所有已注册的初始化模块
inline void cmd_builtin_list(int, const char*[]) {
    auto& table = get_init_table();
    std::cout << "Registered modules (" << table.size() << "):\n";
    for (const auto& entry : table) {
        std::cout << "  [" << entry.priority << "] " << entry.name << "\n";
    }
}

// version —— 显示框架版本信息
inline void cmd_builtin_version(int, const char*[]) {
    std::cout << "initcall framework v" INITCALL_VERSION
                 "  by " INITCALL_AUTHOR
                 "  (" INITCALL_DATE ")\n";
}

// 命令历史记录（linenoise C API 不直接暴露历史，内部维护）
inline std::vector<std::string>& get_cli_history() {
    static std::vector<std::string> hist;
    return hist;
}

// history —— 显示历史命令
inline void cmd_builtin_history(int, const char*[]) {
    auto& hist = get_cli_history();
    if (hist.empty()) {
        std::cout << "(no history)\n";
        return;
    }
    for (size_t i = 0; i < hist.size(); ++i) {
        std::cout << "  " << i + 1 << "  " << hist[i] << "\n";
    }
}

// 确保内置命令只注册一次
inline std::vector<cmd_entry>& get_cmd_table_with_builtins() {
    auto& table = get_cmd_table();
    static bool builtins_registered = []() {
        auto& t = get_cmd_table();
        t.insert(t.begin(), {
            {cmd_builtin_help,    "help",    "显示所有可用命令"},
            {cmd_builtin_clear,   "clear",   "清屏"},
            {cmd_builtin_list,    "list",    "列出所有已注册模块"},
            {cmd_builtin_version, "version", "显示框架版本信息"},
            {cmd_builtin_history, "history", "显示历史命令"},
        });
        return true;
    }();
    (void)builtins_registered;
    return table;
}

// 在命令表中查找并执行一条命令
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

// **************************************************************************
//  Part 3: 交互式命令行循环
// **************************************************************************

// Tab 补全回调：遍历命令表，对前缀匹配的命令名加入补全列表
inline void cli_completion_callback(const char* input,
                                    linenoiseCompletions* lc) {
    auto& table = get_cmd_table_with_builtins();
    size_t len = std::strlen(input);
    for (const auto& cmd : table) {
        if (std::strncmp(cmd.name, input, len) == 0) {
            linenoiseAddCompletion(lc, cmd.name);
        }
    }
}

// 使用 antirez/linenoise 的交互式命令行循环
// prompt: 提示符字符串，默认为 "msh> "（致敬 RT-Thread msh）
inline void cli_loop(const char* prompt = "msh> ") {
    // 注册 Tab 补全回调
    linenoiseSetCompletionCallback(cli_completion_callback);
    // 设置历史最大行数
    linenoiseHistorySetMaxLen(100);

    while (true) {
        char* raw = linenoise(prompt);

        if (!raw) {
            // EOF (Ctrl+D) 或错误
            std::cout << "\n";
            break;
        }

        std::string line(raw);
        linenoiseFree(raw);

        // 跳过空行
        if (line.empty()) continue;

        // exit / quit 退出
        if (line == "exit" || line == "quit") break;

        // 添加到历史记录
        linenoiseHistoryAdd(line.c_str());
        get_cli_history().push_back(line);

        cli_execute(line);
    }
}

} // namespace initcall

// **************************************************************************
//  宏定义（沿用嵌入式惯例命名：INIT_EXPORT / MSH_CMD_EXPORT / INIT_MAIN）
// **************************************************************************

// --------------------------------------------------------------------------
// 模块注册宏（类似 RT-Thread INIT_XXX_EXPORT）
// --------------------------------------------------------------------------
// 用法：在初始化函数定义的下方，写一行：
//   INIT_EXPORT(函数名, "模块名", 优先级)
//
// 展开后为一个 static bool 变量，通过立即执行的 lambda 在程序启动时
// 将模块信息 push 进注册表。
#define INIT_EXPORT(func, name, prio) \
    static bool __initcall_##func = []() { \
        initcall::get_init_table().push_back({func, name, prio}); \
        return true; \
    }();

// --------------------------------------------------------------------------
// 命令注册宏（类似 RT-Thread MSH_CMD_EXPORT）
// --------------------------------------------------------------------------
// 用法：在命令处理函数定义的下方，写一行：
//   MSH_CMD_EXPORT(函数名, "命令名", "帮助信息")
#define MSH_CMD_EXPORT(func, name, help) \
    static bool __msh_cmd_##func = []() { \
        initcall::get_cmd_table().push_back({func, name, help}); \
        return true; \
    }();

// --------------------------------------------------------------------------
// 自动入口宏 —— 在一个 .cpp 文件中使用（通常是 main.cpp）
// --------------------------------------------------------------------------
// 展开为 int main()，自动在第一行调用 do_auto_init()，然后进入 cli_loop()。
// 由于 C++ 标准保证所有全局/静态对象的动态初始化在 main() 之前完成，
// 因此进入 main() 时所有 INIT_EXPORT / MSH_CMD_EXPORT 已注册完毕。
// 用户无需手动调用 do_auto_init() 或 cli_loop()。
#define INIT_MAIN() \
    int main() { \
        initcall::do_auto_init(); \
        std::cout << "=== Initialization complete ===\n\n"; \
        initcall::cli_loop(); \
        return 0; \
    }

#endif // INITCALL_HPP
