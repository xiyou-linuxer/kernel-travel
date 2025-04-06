# Makefile for building init_code

# 工具链和编译选项
ifndef CROSS_COMPILE
CROSS_COMPILE ?= /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-
$(warning CROSS_COMPILE is not set, using default CROSS_COMPILE /opt/gcc-13.2.0-loongarch64-linux-gnu/bin/loongarch64-linux-gnu-)
endif

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

CFLAGS = -c -g -mabi=lp64s -D__KERNEL__
INCLUDE = -I ../arch/loongarch/include/ -I ../include

# 输出目标和源文件
BIN = init_code
OBJS = ../arch/loongarch/kernel/syscall.o printf.o $(BIN).o

# 检查 xxd 是否存在
XXD := $(shell command -v xxd 2>/dev/null)

# 默认目标
all: check-xxd $(BIN)

# 检查 xxd 并在缺失时尝试安装
check-xxd:
ifndef XXD
	@echo "xxd command not found, attempting to install..."
	@which apt-get >/dev/null 2>&1 && sudo apt-get update && sudo apt-get install -y xxd || \
	which dnf >/dev/null 2>&1 && sudo dnf install -y xxd || \
	(echo "Error: Could not install xxd automatically. Please install it manually." && exit 1)
endif

# 链接目标文件生成可执行文件
$(BIN): $(OBJS)
	$(LD) -e main -T program.ld $(OBJS) -o $(BIN)
	xxd -i $(BIN) ../include/xkernel/initcode.h

# 编译规则：通用的 .c 到 .o
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<

# 编译规则：特定路径下的 syscall.c
../arch/loongarch/kernel/syscall.o: ../arch/loongarch/kernel/syscall.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $<

# 清理生成的文件
clean:
	rm -f *.o $(BIN)

# 声明伪目标
.PHONY: all clean
