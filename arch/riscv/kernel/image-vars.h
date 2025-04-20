/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Western Digital Corporation or its affiliates.
 * Linker script variables to be set after section resolution, as
 * ld.lld does not like variables assigned before SECTIONS is processed.
 * Based on arch/arm64/kernel/image-vars.h
 */
#ifndef __RISCV_KERNEL_IMAGE_VARS_H
#define __RISCV_KERNEL_IMAGE_VARS_H

#ifndef LINKER_SCRIPT
#error This file should only be included in vmlinux.lds.S
#endif

#ifdef CONFIG_EFI_STUB

/* __efistub_strcmp = strcmp; */
__efistub_kernel_entry = kernel_entry;
/* __efistub_kernel_asize = kernel_asize; */
__efistub_kernel_fsize = kernel_fsize;
__efistub_kernel_offset = kernel_offset;
/* __efistub_screen_info = screen_info; */

#endif

#endif /* __RISCV_KERNEL_IMAGE_VARS_H */
