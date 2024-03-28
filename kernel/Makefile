CFLAGS_app.o := -Iinclude/kapi/phoenix
CFLAGS_syscall.o += -Iinclude/kapi/ -Iadapter/klibc/include/arch/arm/ -w
ifeq ($(CONFIG_64BIT),y)
CFLAGS_syscall.o += -I$(srctree)/adapter/klibc/include/bits64 -D_BITSIZE=64
else
CFLAGS_syscall.o += -I$(srctree)/adapter/klibc/include//bits32 -D_BITSIZE=32
endif

# obj-y	= cpu.o smp.o printk.o panic.o \
# 	workqueue.o signal.o syscall.o

obj-$(CONFIG_KALLSYMS)	+= kallsyms.o

# obj-y	+= sched/ sh_kapi/ irq/ time/ locking/ count/
