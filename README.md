# Auto Init Demo —— C++ 自动初始化框架

> 类似 RT-Thread `INIT_XXX_EXPORT` 的模块自注册机制演示

## 一、这个项目是做什么的？

在嵌入式系统（如 RT-Thread）中，各模块可以通过一行宏在自己的源文件中完成注册，
程序启动时自动按优先级调用所有已注册模块的初始化函数——**无需手动维护一个集中的调用列表**。

本项目用纯 C++ 实现了同样的机制。

## 二、项目结构

```
├── auto_init.h       # 公共头文件：类型定义 + 注册宏 + get_init_table()
├── main.cpp          # 主程序：do_auto_init() 和 main()
├── module1.cpp       # 模块 A：函数定义 + REGISTER_MODULE（自注册）
├── module2.cpp       # 模块 B/C：函数定义 + REGISTER_MODULE（自注册）
├── user1.cpp         # 用户模块：函数定义 + REGISTER_MODULE（自注册）
├── build/            # 编译输出目录（.o 文件 + 可执行文件，由 make 自动创建）
└── makefile          # 构建脚本（自动扫描所有 .cpp/.c 文件）
```

## 三、核心原理详解

### 3.1 整体流程

```
程序启动
  │
  ▼
┌─────────────────────────────────────────────┐
│ 静态初始化阶段（main 之前自动执行）            │
│                                             │
│  各模块 .cpp 中的 REGISTER_MODULE 被执行        │
│    → get_init_table().push_back(Module A, 30) │
│    → get_init_table().push_back(Module B, 10) │
│    → get_init_table().push_back(Module C, 20) │
│    → get_init_table().push_back(user_init, 5) │
└─────────────────────────────────────────────┘
  │
  ▼
main() 开始执行
  │
  ▼
do_auto_init()
  │  1) 按 priority 排序 get_init_table()
  │  2) 依次调用：user(5) → B(10) → C(20) → A(30)
  ▼
主程序继续运行...
```

### 3.2 涉及的 C++ 关键特性

#### ① `using` 类型别名（C++11）

```cpp
using init_func_t = void(*)();
```

等价于传统写法 `typedef void(*init_func_t)();`，定义一个「函数指针类型」。
`init_func_t` 可以存储任何 `void xxx()` 签名的函数地址。

#### ② 结构体聚合初始化

```cpp
get_init_table().push_back({module_a_init, "Module A", 30});
```

花括号 `{}` 按字段顺序初始化 `init_entry` 结构体——依次是 `func`、`name`、`priority`。

#### ③ `static` 全局变量的初始化时机

```cpp
static bool __reg_xxx = /* 表达式 */;
```

文件作用域的 `static` 变量会在 **`main()` 执行之前**的「静态初始化阶段」被初始化。
利用这个特性，可以在程序启动时自动执行注册代码。

#### ④ Lambda 表达式 + 立即调用（IIFE）

```cpp
static bool __reg_xxx = []() {
    // 注册代码
    return true;
}();          // ← 注意这个 ()，表示立即调用
```

- `[]` — 捕获列表（空 = 不捕获外部变量）
- `()` — 参数列表（空 = 无参数）
- `{}` — 函数体
- 末尾 `()` — **立即调用**这个 lambda

返回 `true` 只是为了给 `static bool` 赋一个值，真正目的是执行 `push_back`。

#### ⑤ `##` 预处理器粘贴操作符

```cpp
#define REGISTER_MODULE(func, name, prio) \
    static bool __reg_##func = ...
```

`##` 将 `__reg_` 和 `func` 参数值拼接成一个标识符，如：
- `func = module_a_init` → 变量名 `__reg_module_a_init`

保证每次注册生成不同的变量名，避免重名。

#### ⑥ 函数局部 `static` + `inline`（首次使用时构造）

```cpp
// auto_init.h 中
inline std::vector<init_entry>& get_init_table() {
    static std::vector<init_entry> table;  // 第一次调用时才构造
    return table;                          // 之后每次返回同一实例
}
```

**为什么不用 `extern` 全局变量？**

C++ 不保证不同 `.cpp` 文件中全局变量的初始化顺序（称为 **SIOF**，Static Initialization Order Fiasco）。
如果模块的 `REGISTER_MODULE` 在 `init_table` 构造之前执行，会向未构造的 `vector` 写入数据——导致注册数量为 0 或崩溃。

函数内的 `static` 局部变量由 C++11 保证在**第一次调用时构造**，彻底避免了顺序问题。
`inline` 关键字允许该函数定义在头文件中被多个 `.cpp` 包含而不产生重复定义错误。

#### ⑦ `std::sort` + Lambda 比较函数

```cpp
auto& table = get_init_table();
std::sort(table.begin(), table.end(),
          [](const init_entry& a, const init_entry& b) {
              return a.priority < b.priority;
          });
```

按 `priority` 升序排列。`const auto& entry` 中的 `&` 是引用，避免拷贝。

### 3.3 REGISTER_MODULE 宏展开示例

源码：
```cpp
void module_a_init() { /* ... */ }
REGISTER_MODULE(module_a_init, "Module A", 30)
```

预处理器展开后：
```cpp
void module_a_init() { /* ... */ }
static bool __reg_module_a_init = []() {
    get_init_table().push_back({module_a_init, "Module A", 30});
    return true;
}();
```

## 四、如何新增一个模块

只需 **一步**，完全不用修改 `main.cpp`，也不用改 `makefile`：

在项目根目录下创建一个新的 `.cpp` 文件：

```cpp
// module3.cpp
#include <iostream>
#include "auto_init.h"

void module_d_init() {
    std::cout << "  -> Module D initialized\n";
}
REGISTER_MODULE(module_d_init, "Module D", 15)
```

然后 `make clean && make && ./build/auto_init_demo`，Module D 会自动出现在输出中。

> makefile 使用 `wildcard *.cpp *.c` 自动扫描根目录下所有源文件，无需手动维护文件列表。

## 五、运行示例

```bash
$ make && ./build/auto_init_demo
=== Auto Initialization Demo ===
Found 4 registered modules
Executing: user_init (priority=5)
  -> user_init initialized
Executing: Module B (priority=10)
  -> Module B initialized
Executing: Module C (priority=20)
  -> Module C initialized
Executing: Module A (priority=30)
  -> Module A initialized
=== Main continues... ===
```

## 六、与 RT-Thread 的对比

| 特性 | RT-Thread | 本项目 |
|------|-----------|--------|
| 注册位置 | 函数下方一行宏 | 函数下方一行宏（相同） |
| 宏名称 | `INIT_BOARD_EXPORT(fn)` 等 | `REGISTER_MODULE(fn, name, prio)` |
| 优先级 | 由宏类型决定（board/device/...） | 由数字参数指定 |
| 底层机制 | 链接器 section + 函数指针数组 | `static` 变量 + lambda + `std::vector` |
| 存储方式 | ELF section 中的裸指针数组 | 堆上的 `std::vector` |
| 适用场景 | 裸机/RTOS（无 C++ 运行时） | 有 C++ 标准库的环境 |