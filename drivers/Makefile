ccflags-y := -std=gnu99 -Wno-declaration-after-statement
ifdef CONFIG_LOONGARCH
obj-y	+= firmware/
endif

obj-y	+= serial/
obj-y   += pci/
obj-y   += disk/
obj-y   += ../thread/switch.o
obj-y   += timer.o
obj-y	+= console.o
obj-y   += ../thread/thread.o
obj-y   += ../lib/kernel/list.o
obj-y   += ../lib/kernel/bitmap.o
obj-y	+= ioqueue.o

