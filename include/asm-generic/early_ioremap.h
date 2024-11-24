#ifndef _ASM_EARLY_IOREMAP_H
#define _ASM_EARLY_IOREMAP_H

#include <xkernel/types.h>
#include <xkernel/compiler.h>

extern void __iomem *early_ioremap(resource_size_t phys_addr,
				   unsigned long size);
extern void *early_memremap(resource_size_t phys_addr,
			    unsigned long size);
extern void *early_memremap_ro(resource_size_t phys_addr,
			       unsigned long size);
extern void *early_memremap_prot(resource_size_t phys_addr,
				 unsigned long size, unsigned long prot_val);

#endif /* _ASM_EARLY_IOREMAP_H */
