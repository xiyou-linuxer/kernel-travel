#include <linux/efi.h>

#include "efistub.h"

/* 具有4级分页64位内核的最大物理地址 */
#define MAXMEM_X86_64_4LEVEL (1ull << 46)

const efi_system_table_t *efi_system_table;
const efi_dxe_services_table_t *efi_dxe_table;
