# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

KBUILD_DEFCONFIG := virt_defconfig	

LDFLAGS_vmlinux := -z norelro

export BITS
ifeq ($(CONFIG_ARCH_RV64I),y)
	BITS := 64
	UTS_MACHINE := riscv64

	KBUILD_CFLAGS += -mabi=lp64
	KBUILD_AFLAGS += -mabi=lp64

	KBUILD_LDFLAGS += -melf64lriscv
else
	BITS := 32
	UTS_MACHINE := riscv32

	KBUILD_CFLAGS += -mabi=ilp32
	KBUILD_AFLAGS += -mabi=ilp32
	KBUILD_LDFLAGS += -melf32lriscv
endif

ifndef CONFIG_RISCV_USE_LINKER_RELAXATION
	KBUILD_CFLAGS += -mno-relax
	KBUILD_AFLAGS += -mno-relax
ifndef CONFIG_AS_IS_LLVM
	KBUILD_CFLAGS += -Wa,-mno-relax
	KBUILD_AFLAGS += -Wa,-mno-relax
endif
# LLVM has an issue with target-features and LTO: https://github.com/llvm/llvm-project/issues/59350
# Ensure it is aware of linker relaxation with LTO, otherwise relocations may
# be incorrect: https://github.com/llvm/llvm-project/issues/65090
else ifeq ($(CONFIG_LTO_CLANG),y)
	KBUILD_LDFLAGS += -mllvm -mattr=+c -mllvm -mattr=+relax
endif

ifeq ($(CONFIG_SHADOW_CALL_STACK),y)
	KBUILD_LDFLAGS += --no-relax-gp
endif

# ISA string setting
riscv-march-$(CONFIG_ARCH_RV32I)	:= rv32ima
riscv-march-$(CONFIG_ARCH_RV64I)	:= rv64ima
riscv-march-$(CONFIG_FPU)		:= $(riscv-march-y)fd
riscv-march-$(CONFIG_RISCV_ISA_C)	:= $(riscv-march-y)c
riscv-march-$(CONFIG_RISCV_ISA_V)	:= $(riscv-march-y)v

ifdef CONFIG_TOOLCHAIN_NEEDS_OLD_ISA_SPEC
KBUILD_CFLAGS += -Wa,-misa-spec=2.2
KBUILD_AFLAGS += -Wa,-misa-spec=2.2
else
riscv-march-$(CONFIG_TOOLCHAIN_NEEDS_EXPLICIT_ZICSR_ZIFENCEI) := $(riscv-march-y)_zicsr_zifencei
endif

# Check if the toolchain supports Zihintpause extension
riscv-march-$(CONFIG_TOOLCHAIN_HAS_ZIHINTPAUSE) := $(riscv-march-y)_zihintpause

# Remove F,D,V from isa string for all. Keep extensions between "fd" and "v" by
# matching non-v and non-multi-letter extensions out with the filter ([^v_]*)
KBUILD_CFLAGS += -march=$(shell echo $(riscv-march-y) | sed -E 's/(rv32ima|rv64ima)fd([^v_]*)v?/\1\2/')

KBUILD_AFLAGS += -march=$(riscv-march-y)

# For C code built with floating-point support, exclude V but keep F and D.
CC_FLAGS_FPU  := -march=$(shell echo $(riscv-march-y) | sed -E 's/(rv32ima|rv64ima)([^v_]*)v?/\1\2/')

KBUILD_CFLAGS += -mno-save-restore
KBUILD_CFLAGS += -DCONFIG_PAGE_OFFSET=$(CONFIG_PAGE_OFFSET)

ifeq ($(CONFIG_CMODEL_MEDLOW),y)
	KBUILD_CFLAGS += -mcmodel=medlow
endif
ifeq ($(CONFIG_CMODEL_MEDANY),y)
	KBUILD_CFLAGS += -mcmodel=medany
endif

# Avoid generating .eh_frame sections.
KBUILD_CFLAGS += -fno-asynchronous-unwind-tables -fno-unwind-tables

# The RISC-V attributes frequently cause compatibility issues and provide no
# information, so just turn them off.
KBUILD_CFLAGS += $(call cc-option,-mno-riscv-attribute)
KBUILD_AFLAGS += $(call cc-option,-mno-riscv-attribute)
KBUILD_CFLAGS += $(call as-option,-Wa$(comma)-mno-arch-attr)
KBUILD_AFLAGS += $(call as-option,-Wa$(comma)-mno-arch-attr)

KBUILD_CFLAGS_MODULE += $(call cc-option,-mno-relax)
KBUILD_AFLAGS_MODULE += $(call as-option,-Wa$(comma)-mno-relax)

# GCC versions that support the "-mstrict-align" option default to allowing
# unaligned accesses.  While unaligned accesses are explicitly allowed in the
# RISC-V ISA, they're emulated by machine mode traps on all extant
# architectures.  It's faster to have GCC emit only aligned accesses.
ifneq ($(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS),y)
KBUILD_CFLAGS += $(call cc-option,-mstrict-align)
endif

ifeq ($(CONFIG_STACKPROTECTOR_PER_TASK),y)
prepare: stack_protector_prepare
stack_protector_prepare: prepare0
	$(eval KBUILD_CFLAGS += -mstack-protector-guard=tls		  \
				-mstack-protector-guard-reg=tp		  \
				-mstack-protector-guard-offset=$(shell	  \
			awk '{if ($$2 == "TSK_STACK_CANARY") print $$3;}' \
					include/generated/asm-offsets.h))
endif

# arch specific predefines for sparse
CHECKFLAGS += -D__riscv -D__riscv_xlen=$(BITS)

# Default value
head-y		:= arch/riscv/kernel/head.o

export	TEXT_OFFSET GZFLAGS

core-y		+= arch/riscv/kernel/
core-y		+= arch/riscv/mm/

# Default target when executing plain make
ifeq ($(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE),y)
KBUILD_IMAGE	:= Image.gz-dtb
else
KBUILD_IMAGE	:= Image.gz
endif

KBUILD_DTBS	:= dtbs

all:	$(KBUILD_DTBS) Image

boot	:= arch/riscv/boot

uImage: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

Image Image.gz: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

zinstall install: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $@

%.dtb: scripts
	$(Q)$(MAKE) $(build)=$(boot)/dts $(boot)/dts/$@

PHONY += dtbs dtbs_install

dtbs: prepare scripts
	$(Q)$(MAKE) $(build)=$(boot)/dts

dtbs_install:
	$(Q)$(MAKE) $(dtbinst)=$(boot)/dts

Image.gz-dtb: vmlinux scripts dtbs
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

PHONY += vdso_install
vdso_install:
	$(Q)$(MAKE) $(build)=arch/loongarch/kernel/vdso $@

# We use MRPROPER_FILES and CLEAN_FILES now
archclean:
	$(Q)$(MAKE) $(clean)=$(boot)
	$(Q)$(MAKE) $(clean)=$(boot)/dts

define archhelp
  echo  '* Image.gz      - Compressed kernel image (arch/$(ARCH)/boot/Image.gz)'
  echo  '  Image         - Uncompressed kernel image (arch/$(ARCH)/boot/Image)'
  echo  '* dtbs          - Build device tree blobs for enabled boards'
  echo  '  dtbs_install  - Install dtbs to $(INSTALL_DTBS_PATH)'
  echo  '  install       - Install uncompressed kernel'
  echo  '  zinstall      - Install compressed kernel'
  echo  '                  Install using (your) ~/bin/installkernel or'
  echo  '                  (distribution) /sbin/installkernel or'
  echo  '                  install to $$(INSTALL_PATH) and run lilo'
endef
