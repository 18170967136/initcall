# ==========================================================================
# Makefile —— 自动初始化演示程序的构建脚本
# ==========================================================================

# CXX:      C++ 编译器，这里用 g++
# CXXFLAGS: 编译选项
#   -Wall   开启所有常见警告
#   -g      生成调试信息（可用 gdb 调试）
CXX = g++
CXXFLAGS = -Wall -g

# 编译输出目录：所有 .o 文件和可执行文件都放在这里
BUILD_DIR = build

# 自动扫描根目录下所有 .cpp 和 .c 源文件
# wildcard 函数展开通配符，patsubst 将 xxx.cpp/xxx.c 替换为 build/xxx.o
SRCS = $(wildcard *.cpp *.c)
OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SRCS))) \
       $(patsubst %.c,  $(BUILD_DIR)/%.o,$(filter %.c,  $(SRCS)))
TARGET = $(BUILD_DIR)/auto_init_demo

# 默认目标：先确保 build/ 目录存在，再构建可执行文件
all: $(BUILD_DIR) $(TARGET)

# 创建 build 目录（如果不存在）
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接：将所有 .o 文件链接成最终可执行文件
# $@ = 目标名，$^ = 所有依赖的 .o 文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译 .cpp → build/*.o
$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译 .c → build/*.o
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理：删除整个 build 目录
clean:
	rm -rf $(BUILD_DIR)

# 声明伪目标（它们不是真实文件名，避免与同名文件冲突）
.PHONY: all clean