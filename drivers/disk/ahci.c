#include <asm/pci.h>
#include <linux/printk.h>
#include <asm-generic/io.h>
#include <linux/stdio.h>
#include <asm/addrspace.h>
#include <linux/ahci.h>
void disk_init(void)
{

 char *block_data = 0;

 int i;

 for (i = 0; i < NR_BUFFER; i++, block_data += BLOCK_SIZE)

 {

 buffer_table[i].blocknr = -1;

 if (i % 8 == 0)

 block_data = (char *)get_page();

 buffer_table[i].data = block_data;

 }

 *(unsigned int *)(HBA_PORT0_CMD) |= HBA_PORT0_CMD_FRE;

 *(unsigned int *)(HBA_PORT0_CMD) |= HBA_PORT0_CMD_ST;

 *(unsigned int *)(HBA_GHC) |= HBA_GHC_IE;

 *(unsigned int *)(HBA_PORT0_IE) |= HBA_PORT0_IE_DHRE;
}