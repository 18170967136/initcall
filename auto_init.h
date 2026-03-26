// auto_init.h
// ==========================================================================
// 自动初始化框架 —— 公共头文件
// 灵感来源于 RT-Thread 的 INIT_XXX_EXPORT 机制。
//
// 使用方法（在各模块的 .cpp 文件中）：
//   #include "auto_init.h"
//
//   void my_module_init() { /* 初始化逻辑 */ }
//   REGISTER_MODULE(my_module_init, "My Module", 10)
//
// 这样每个模块在自己的源文件中完成注册，无需修改 main.cpp。
// ==========================================================================

#ifndef AUTO_INIT_H
#define AUTO_INIT_H

#include <vector>

// --------------------------------------------------------------------------
// 1. 初始化函数指针类型
// --------------------------------------------------------------------------
// 指向「无参数、无返回值」函数的指针类型
using init_func_t = void(*)();

// --------------------------------------------------------------------------
// 2. 模块描述符结构体
// --------------------------------------------------------------------------
struct init_entry {
    init_func_t func;      // 指向模块初始化函数的指针
    const char* name;      // 模块名称（用于日志输出）
    int priority;          // 优先级，数值越小越先执行
};

// --------------------------------------------------------------------------
// 3. 全局注册表的访问函数（「首次使用时构造」惯用法）
// --------------------------------------------------------------------------
// 不用 extern 变量，而是通过函数返回一个函数内部的 static 局部变量引用。
//
// 为什么这样做？
//   如果用全局变量 extern std::vector<init_entry> init_table，
//   当多个 .cpp 文件的静态初始化代码（REGISTER_MODULE 展开的 static bool）
//   比 init_table 的定义所在的 .cpp 更早被链接器初始化时，
//   会出现「向未构造的 vector push_back」的未定义行为（注册数量变成 0 或崩溃）。
//
// 函数内的 static 局部变量（C++11 保证）：
//   第一次调用 get_init_table() 时才构造 table，之后每次返回同一个实例。
//   无论哪个 .cpp 先被初始化，调用此函数时 table 必然已经构造好，
//   彻底避免了初始化顺序问题。
inline std::vector<init_entry>& get_init_table() {
    static std::vector<init_entry> table;
    return table;
}

// --------------------------------------------------------------------------
// 4. 注册宏 —— 类似 RT-Thread 的 INIT_XXX_EXPORT
// --------------------------------------------------------------------------
// 用法：在模块初始化函数定义的下方，直接写一行：
//   REGISTER_MODULE(函数名, "模块名", 优先级)
//
// 原理：
//   宏展开为一个 static bool 变量，其初始值由一个「立即执行的 lambda」提供。
//   程序启动时（main 之前），lambda 被执行，将模块信息 push 进全局 init_table。
//
// 例如：REGISTER_MODULE(module_a_init, "Module A", 30)
// 展开为：
//   static bool __reg_module_a_init = []() {
//       get_init_table().push_back({module_a_init, "Module A", 30});
//       return true;
//   }();
//
// ## 是预处理器「token 粘贴」操作符，将 __reg_ 与函数名拼接生成唯一变量名。
#define REGISTER_MODULE(func, name, prio) \
    static bool __reg_##func = []() { \
        get_init_table().push_back({func, name, prio}); \
        return true; \
    }();

#endif // AUTO_INIT_H
