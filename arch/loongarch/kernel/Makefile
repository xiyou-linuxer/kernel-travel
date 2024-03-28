#
# Makefile for the linux kernel.
#

CPPFLAGS_vmlinux.lds := -D__KERNEL__  -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -funsigned-char -std=gnu11 -mabi=lp64s -G0 -pipe -msoft-float -ffreestanding  -mno-check-zero-division -fno-asynchronous-unwind-tables -isystem /opt/cross-tools/lib/gcc/loongarch64-unknown-linux-gnu/13.0.0/include -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member -O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-main -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-dangling-pointer -fomit-frame-pointer -fno-stack-clash-protection -Wdeclaration-after-statement -Wvla -Wno-pointer-sign  -Wcast-function-type -Wno-stringop-truncation -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -fno-strict-overflow -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wno-packed-not-aligned
ifdef CONFIG_2K1000LA
CPPFLAGS_vmlinux.lds += -DVMLINUX_LOAD_ADDRESS=0x9000000008000000
else
 CPPFLAGS_vmlinux.lds += -DVMLINUX_LOAD_ADDRESS=0x9000000000200000
endif
# AFLAGS_head.o        := -DTEXT_OFFSET=$(TEXT_OFFSET)

# Object file lists.

obj-y := setup.o

head-y			:= head.o
extra-y := $(head-y) vmlinux.lds
