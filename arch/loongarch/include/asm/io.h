#ifndef _ASM_IO_H
#define _ASM_IO_H

#include <xkernel/init.h>
#include <xkernel/types.h>
#include <xkernel/compiler.h>

extern void __init __iomem *early_ioremap(u64 phys_addr, unsigned long size);
extern void __init early_iounmap(void __iomem *addr, unsigned long size);

#define early_memremap early_ioremap
#define early_memunmap early_iounmap

#endif /* _ASM_IO_H */
