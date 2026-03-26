# ==========================================================================
# Makefile —— autoreg 自动初始化 & CLI 框架演示程序的构建脚本
# ==========================================================================

# CXX:      C++ 编译器
# CXXFLAGS: C++ 编译选项
#   -std=c++11   C++11 标准（Magic Statics, lambda 等特性依赖）
#   -Wall        开启所有常见警告
#   -g           生成调试信息（可用 gdb 调试）
#   -DAUTOREG_USE_LINENOISE  启用 linenoise 高级行编辑（Tab 补全、历史、方向键）
# CC:       C 编译器（编译 linenoise.c）
# CFLAGS:   C 编译选项
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g
CC = gcc
CFLAGS = -Wall -g

# 编译输出目录：所有 .o 文件和可执行文件都放在这里
BUILD_DIR = build

# antirez/linenoise 下载地址（BSD 协议）
LINENOISE_BASE = https://raw.githubusercontent.com/antirez/linenoise/master

# 自动扫描根目录下所有 .cpp 源文件
# linenoise.c 通过 wget 下载，显式加入 OBJS（避免首次构建时 wildcard 找不到）
SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS)) $(BUILD_DIR)/linenoise.o
TARGET = $(BUILD_DIR)/auto_init_demo

# 默认目标：先确保依赖就绪，再构建可执行文件
all: linenoise.h linenoise.c $(BUILD_DIR) $(TARGET)

# 自动下载 antirez/linenoise（如果不存在）
linenoise.h:
	@echo "[download] Fetching linenoise.h ..."
	wget -q -O $@ $(LINENOISE_BASE)/linenoise.h

linenoise.c:
	@echo "[download] Fetching linenoise.c ..."
	wget -q -O $@ $(LINENOISE_BASE)/linenoise.c

# 创建 build 目录（如果不存在）
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接：将所有 .o 文件链接成最终可执行文件
# $@ = 目标名，$^ = 所有依赖的 .o 文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 编译 .cpp → build/*.o（依赖 auto_init.hpp 和 linenoise.h）
$(BUILD_DIR)/%.o: %.cpp auto_init.hpp linenoise.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译 linenoise.c → build/linenoise.o
$(BUILD_DIR)/linenoise.o: linenoise.c linenoise.h
	$(CC) $(CFLAGS) -c $< -o $@

# 清理：删除整个 build 目录
clean:
	rm -rf $(BUILD_DIR)

# 深度清理：同时删除下载的第三方文件
distclean: clean
	rm -f linenoise.h linenoise.c

# 声明伪目标（它们不是真实文件名，避免与同名文件冲突）
.PHONY: all clean distclean