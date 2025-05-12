# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the RISC-V Linux kernel
#
CPPFLAGS_vmlinux.lds += -DVMLINUX_LOAD_ADDRESS=0xffffffff80000000
# Object file lists.
obj-y := setup.o
head-y			:= head.o
extra-y := $(head-y) vmlinux.lds
