#
# arch/loongarch/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2001 by Russell King

LDFLAGS_vmlinux	:=-m elf64loongarch -G0 -n -nostdlib
# CPPFLAGS_vmlinux.lds = -DVMLINUX_LOAD_ADDRESS=0x9000000000200000 -fmacro-prefix-map=./= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -funsigned-char -std=gnu11 -mabi=lp64s -G0 -pipe -msoft-float -mexplicit-relocs -ffreestanding -mno-check-zero-division -fno-asynchronous-unwind-tables -isystem /opt/cross-tools/lib/gcc/loongarch64-unknown-linux-gnu/13.0.0/include -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member -O2 -fno-allow-store-data-races -Wframe-larger-than=2048 -fstack-protector-strong -Wno-main -Wno-unused-but-set-variable -Wno-unused-const-variable -Wno-dangling-pointer -fomit-frame-pointer -ftrivial-auto-var-init=zero -fno-stack-clash-protection -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wcast-function-type -Wno-stringop-truncation -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -fno-strict-overflow -fno-stack-check -fconserve-stack -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wno-packed-not-aligned   -mdirect-extern-access  -DKBUILD_MODFILE='"arch/loongarch/mm/init"' -DKBUILD_BASENAME='"init"' -DKBUILD_MODNAME='"init"' -D__KBUILD_MODNAME=kmod_init -c
# OBJCOPYFLAGS	:=-O binary -R .note -R .note.gnu.build-id -R .comment -S

ifdef CONFIG_2K1000LA
OBJCOPYFLAGS	:=-O binary
else
OBJCOPYFLAGS	:=-O binary --remove-section=.comment --remove-section=.note --remove-section=.options --remove-section=.note.gnu.build-id -S
endif

GZFLAGS		:=-9

# LDFLAGS		:= -m elf64loongarch -z noexecstack --no-warn-rwx-segments -r --build-id=sha1
# LDFLAGS		+= -m elf64loongarch --no-warn-rwx-segments -T ./arch/loongarch/kernel/vmlinux.lds -G0 -n -nostdlib
LDFLAGS		+= -m elf64loongarch -G0 -n -nostdlib
KBUILD_AFLAGS	+= -nostdinc -D__ASSEMBLY__ -fno-PIE -mabi=lp64s -G0 -pipe -msoft-float -ffreestanding -mno-check-zero-division

KBUILD_DEFCONFIG := loongson3_defconfig

KBUILD_CFLAGS	+= -fno-pic
KBUILD_CFLAGS	+= -nostdinc -I./arch/loongarch/include -I./include -I./arch/loongarch/include/uapi -I./include/uapi\
	-Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing \
	-fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type \
	-Wno-format-security -funsigned-char -std=gnu11 -mabi=lp64s -G0 -pipe -msoft-float -ffreestanding \
	-mno-check-zero-division -fno-asynchronous-unwind-tables \
	-fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member \
	-O2 -Wframe-larger-than=2048 -fstack-protector-strong -Wno-main -Wno-unused-but-set-variable \
	-Wno-unused-const-variable -Wno-dangling-pointer -fomit-frame-pointer -fno-stack-clash-protection \
	-Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wcast-function-type -Wno-stringop-truncation -Wno-stringop-overflow -Wno-restrict \
	-Wno-maybe-uninitialized -Wno-alloc-size-larger-than -Wimplicit-fallthrough=5 -fno-strict-overflow -fno-stack-check -fconserve-stack \
	-Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wno-packed-not-aligned -fpie -Os -DDISABLE_BRANCH_PROFILING \
	-ffreestanding -fno-stack-protector

ifeq ($(CONFIG_CPU_BIG_ENDIAN), y)
AS		+= -EB
LD		+= -EB
else
AS		+= -EL
LD		+= -EL
endif

# CHECKFLAGS	+= $(shell $(CC) $(KBUILD_CFLAGS) -dM -E -x c /dev/null | \
# 	grep -E -vw '__GNUC_(MINOR_|PATCHLEVEL_)?_' | \
# 	sed -e "s/^\#define /-D'/" -e "s/ /'='/" -e "s/$$/'/" -e 's/\$$/&&/g')

ifeq ($(CONFIG_ARM64_ERRATUM_843419), y)
KBUILD_CFLAGS_MODULE	+= -mcmodel=large
endif

# Default value
head-y		:= arch/loongarch/kernel/head.o

export	TEXT_OFFSET GZFLAGS

# core-y		+= arch/loongarch/kernel/ arch/loongarch/mm/
core-y		+= arch/loongarch/kernel/
core-y		+= arch/loongarch/lib/
#core-$(CONFIG_NET) += arch/loongarch/net/
#core-$(CONFIG_KVM) += arch/loongarch/kvm/
#core-$(CONFIG_XEN) += arch/loongarch/xen/
#core-$(CONFIG_CRYPTO) += arch/loongarch/crypto/
# libs-y		:= arch/loongarch/lib/ $(libs-y)
#core-$(CONFIG_EFI_STUB) += $(objtree)/drivers/firmware/efi/libstub/lib.a

# Default target when executing plain make
ifeq ($(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE),y)
KBUILD_IMAGE	:= Image.gz-dtb
else
KBUILD_IMAGE	:= Image.gz
endif

KBUILD_DTBS	:= dtbs

all:	$(KBUILD_IMAGE) $(KBUILD_DTBS)

boot := arch/loongarch/boot

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
