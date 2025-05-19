
#include <xkernel/init.h>
#include <asm/soc.h>

struct of_device_id {
	char	name[32];
	char	type[32];
	char	compatible[128];
	const void *data;
};

extern void *_dtb_early_va;
#define dtb_early_va	_dtb_early_va

// void __init soc_early_init(void)
// {
// 	void (*early_fn)(const void *fdt);
// 	const struct of_device_id *s;
// 	const void *fdt = dtb_early_va;

// 	for (s = (void *)&__soc_early_init_table_start;
// 	     (void *)s < (void *)&__soc_early_init_table_end; s++) {
// 		if (!fdt_node_check_compatible(fdt, 0, s->compatible)) {
// 			early_fn = s->data;
// 			early_fn(fdt);
// 			return;
// 		}
// 	}
// }
