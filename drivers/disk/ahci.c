#include <asm/pci.h>
#include <linux/printk.h>
#include <asm-generic/io.h>
#include <linux/stdio.h>
#include <asm/addrspace.h>
#include <linux/ahci.h>
unsigned long SATA_ABAR_BASE;
static void start_cmd(unsigned long prot_base)
{

    // Wait until CR (bit15) is cleared
    while (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) |= HBA_PxCMD_FRE;
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) |= HBA_PxCMD_ST;
}

// Stop command engine
static void stop_cmd(unsigned long prot_base)
{
    // Clear ST (bit0)
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) &= ~HBA_PxCMD_ST;

    // Clear FRE (bit4)
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) &= ~HBA_PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    while (1)
    {
        if (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) & HBA_PxCMD_FR)
            continue;
        if (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) & HBA_PxCMD_CR)
            continue;
        break;
    }
}

void disk_init(void) {
    printk("disk_init start\n");
    char* block_data = 0;
    /*获取磁盘控制器的bar地址*/
    pci_device_t *pci_dev=pci_get_device_by_bus(0, 8, 0);
    /*设备不存在则报错*/
    if (pci_dev==NULL)
    {
        printk(KERN_ERR "[ahci]: no AHCI controllers present!\n");
    }
    SATA_ABAR_BASE = 0x8000000000000000 | pci_dev->bar[0].base_addr;
    /*注册中断处理程序*/
    
    /*初始化块缓冲区*/
    /*for (int i = 0; i < NR_BUFFER; i++, block_data += BLOCK_SIZE)
    {
        buffer_table[i].blocknr = -1;
        if (i % 8 == 0)
            block_data = (char*)get_page();
        buffer_table[i].data = block_data;
    }*/
    *(unsigned int *)(SATA_ABAR_BASE|(PORT0_BASE+PORT_CMD)) |= HBA_PxCMD_FRE;//开启端口的接收
    *(unsigned int *)(SATA_ABAR_BASE|(PORT0_BASE+PORT_CMD)) |= HBA_PxCMD_ST;//开启向端口输入命令
    *(unsigned int *)(SATA_ABAR_BASE|HBA_GHC) |= HBA_GHC_IE;//全局中断使能
    *(unsigned int *)(SATA_ABAR_BASE|HBA_GHC) |= HBA_GHC_AHCI_ENABLE;//启用ahci
    *(unsigned int *)(SATA_ABAR_BASE|(PORT0_BASE+PORT_IE)) |= HBA_PORT0_IE_DHRE;//port1中断使能
    printk("disk_init down\n");
    /*io调度初始化*/
}