#include <asm/pci.h>
#include <linux/printk.h>
#include <asm-generic/io.h>
#include <linux/stdio.h>
#include <asm/addrspace.h>
#include <linux/ahci.h>
#include <linux/list.h>
#include <linux/string.h>
unsigned long SATA_ABAR_BASE;//sata控制器的bar地址，0x80000000400e0000
struct hba_command_header ahci_port_base_vaddr[32];
/*启动命令引擎*/
static void start_cmd(unsigned long prot_base)
{

    // Wait until CR (bit15) is cleared
    while (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) & HBA_PxCMD_CR);

    // Set FRE (bit4) and ST (bit0)
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) |= HBA_PxCMD_FRE;// 开启端口的接收
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CMD)) |= HBA_PxCMD_ST;//开启向端口输入命令
}

/*停止命令引擎*/ 
static void stop_cmd(unsigned int prot_base)
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

/*寻找可用的命令槽位*/
static int ahci_find_cmdslot(unsigned long prot_base)
{
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_SACT)) | *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CI)));
    int num_of_cmd_clots =((*(unsigned int*)(SATA_ABAR_BASE | HBA_CAP)) & 0x0f00) >> 8;  // bit 12-8
    for (int i = 0; i < num_of_cmd_clots; i++)
    {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }
    printk("Cannot find free command list entry\n");
    return -1;
}

static int ahci_read(unsigned long prot_base, unsigned int startl, unsigned int starth, unsigned int count, unsigned int buf)
{
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_IS)) = (uint32_t)-1; // Clear pending interrupt bits
    //int spin = 0;            // Spin lock timeout counter
    int slot = ahci_find_cmdslot(prot_base);
 
    if (slot == -1)
        return E_NOEMPTYSLOT;

    struct hba_command_header *cmdheader = (struct hba_command_header *)((*(unsigned long*)(SATA_ABAR_BASE|(prot_base+PORT_CLBU)) << 32)|*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CLB)));
    cmdheader += slot;
    cmdheader->fis_length = sizeof(struct fis_reg_host_to_device) / sizeof(uint32_t); // 帧结构大小
    cmdheader->write = 0;                                        // Read from device
    cmdheader->prdt_len = (uint16_t)((count - 1) >> 4) + 1;     // PRDT entries count

    struct hba_command_table *cmdtbl = (struct hba_command_table *)(cmdheader->command_table_base);
    memset(cmdtbl, 0, sizeof(struct hba_command_table) + (cmdheader->prdt_len - 1) * sizeof(struct hba_prdt_entry));

    // 8K bytes (16 sectors) per PRDT
    int i;
    for (i = 0; i < cmdheader->prdt_len - 1; ++i)
    {
        cmdtbl->prdt_entries[i].data_base = buf;
        cmdtbl->prdt_entries[i].byte_count = 8 * 1024 - 1; // 8K bytes (this value should always be set to 1 less than the actual value)
        cmdtbl->prdt_entries[i].interrupt_on_complete = 1;
        buf += 4 * 1024; // 4K uint16_ts
        count -= 16;     // 16 sectors
    }

    // Last entry
    cmdtbl->prdt_entries[i].data_base = buf;
    cmdtbl->prdt_entries[i].byte_count = (count << 9) - 1; // 512 bytes per sector
    cmdtbl->prdt_entries[i].interrupt_on_complete = 1;

    // Setup command
    struct fis_reg_host_to_device *cmdfis = (struct fis_reg_host_to_device *)(&cmdtbl->command_fis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command
    cmdfis->command = ATA_CMD_READ_DMA_EXT;

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl >> 8);
    cmdfis->lba2 = (uint8_t)(startl >> 16);
    cmdfis->device = 1 << 6; // LBA mode

    cmdfis->lba3 = (uint8_t)(startl >> 24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth >> 8);

    cmdfis->count_l = count & 0xFF;
    cmdfis->count_h = (count >> 8) & 0xFF;

    // The below loop waits until the port is no longer busy before issuing a new command
    /*while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
    {
        spin++;
    }
    if (spin == 1000000)
    {
        kerror("Port is hung");
        return E_PORT_HUNG;
    }*/

    printk("slot=%d", slot);
    *(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CI)) = 1 << slot; // Issue command

    // Wait for completion
    while (1)
    {
        // In some longer duration reads, it may be helpful to spin on the DPS bit
        // in the PxIS port field as well (1 << 5)
        if ((*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_CI)) & (1 << slot)) == 0)
            break;
        if (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_IS)) & HBA_PxIS_TFES) // Task file error
        {
            printk("Read disk error");
            return E_TASK_FILE_ERROR;
        }
    }

    // Check again
    if (*(unsigned int *)(SATA_ABAR_BASE|(prot_base+PORT_IS))  & HBA_PxIS_TFES)
    {
        printk("Read disk error");
        return E_TASK_FILE_ERROR;
    }

    return AHCI_SUCCESS;
}

/*io调度函数
static long ahci_query_disk()
{
    
}*/

/*将请求提交到io调度队列*/
/*static void ahci_submit(struct block_device_request_packet *pack)
{
    
}*/

static int check_type(unsigned int port)
{
    printk("check_type\n");
    uint32_t ssts = *(unsigned int*)(SATA_ABAR_BASE | (port + PORT_SSTS));
    uint32_t sig = *(unsigned int*)(SATA_ABAR_BASE | (port + PORT_SIG));
    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT) // Check drive status
        return AHCI_DEV_NULL;
    if (ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;

    switch (sig)
    {
    case SATA_SIG_ATAPI:
        return AHCI_DEV_SATAPI;
    case SATA_SIG_SEMB:
        return AHCI_DEV_SEMB;
    case SATA_SIG_PM:
        return AHCI_DEV_PM;
    default:
        return AHCI_DEV_SATA;
    }
}

static void ahci_probe_port(void)
{
    
    uint32_t pi = *(unsigned int*)(SATA_ABAR_BASE | HBA_PI);
    printk("ahci_probe_port pi:%d\n",pi);
    for (int i = 0; i < PORT_NR; ++i, (pi >>= 1))
    {
        if (pi & 1)
        {
            unsigned int dt = check_type(PORT_BASE+PORT_OFFEST*i);
            printk("ahci_probe_port dt:%d\n",dt);
            if (dt == AHCI_DEV_SATA)
            {
                printk("SATA drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SATAPI)
            {
                printk("SATAPI drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_SEMB)
            {
                printk("SEMB drive found at port %d\n", i);
            }
            else if (dt == AHCI_DEV_PM)
            {
                printk("PM drive found at port %d\n", i);
            }
            else
            {
                // kdebug("No drive found at port %d", i);
            }
        }
    }
    printk("ahci_probe_port down");
}

static void port_rebase(int portno)
{
    unsigned int port = PORT_BASE + portno * PORT_OFFEST;//计算端口的偏移地址
    
    stop_cmd(port); // 停止命令引擎

    // Command list offset: 1K*portno
    // Command list entry size = 32
    // Command list entry maxim count = 32
    // Command list maxim size = 32*32 = 1K per port

    *(unsigned long *)(SATA_ABAR_BASE|(port+PORT_CLB)) = ahci_port_base_vaddr + (portno << 10);

    memset((void *)(ahci_port_base_vaddr + (portno << 10)), 0, 1024);

    // FIS offset: 32K+256*portno
    // FIS entry size = 256 bytes per port
    *(unsigned long *)(SATA_ABAR_BASE|(port+PORT_FB)) = ahci_port_base_vaddr + (32 << 10) + (portno << 8);

    memset((void *)(ahci_port_base_vaddr + (32 << 10) + (portno << 8)), 0, 256);

    // Command table offset: 40K + 8K*portno
    // Command table size = 256*32 = 8K per port
    struct hba_command_header *cmdheader = (struct hba_command_header *)((SATA_ABAR_BASE|(port+PORT_CLB)));
    for (int i = 0; i < 32; ++i)
    {
        cmdheader[i].prdt_len = 8; // 8 prdt entries per command table
                                // 256 bytes per command table, 64+16+48+16*8
        // Command table offset: 40K + 8K*portno + cmdheader_index*256
        cmdheader[i].command_table_base = ahci_port_base_vaddr + (40 << 10) + (portno << 13) + (i << 8);

        memset((void *)cmdheader[i].command_table_base, 0, 256);
    }

    start_cmd(port); // Start command engine
}

/*磁盘驱动初始化*/
void disk_init(void) {
    printk("disk_init start\n");
    char* block_data = 0;
    /*获取磁盘控制器的bar地址*/
    pci_device_t *pci_dev=pci_get_device_by_bus(0, 8, 0);
    /*sata控制器不存在则报错*/
    if (pci_dev==NULL)
    {
        printk(KERN_ERR "[ahci]: no AHCI controllers present!\n");
    }
    SATA_ABAR_BASE = 0x8000000000000000|pci_dev->bar[0].base_addr;
    printk("SATA_ABAR_BASE: %16x\n", SATA_ABAR_BASE);
    /*注册中断处理程序*/

    *(unsigned int *)(SATA_ABAR_BASE|HBA_GHC) |= HBA_GHC_IE;//全局中断使能
    *(unsigned int *)(SATA_ABAR_BASE|HBA_GHC) |= HBA_GHC_AHCI_ENABLE;//启用ahci

    // kalloc();//分配
    ahci_probe_port();  // 扫描ahci的所有端口
    port_rebase(1);//开启0号端口

    /*io调度初始化*/
    /*ahci_req_queue.in_service = NULL;
    list_init(&(ahci_req_queue.queue_list));
    ahci_req_queue.request_count = 0;*/
    printk("disk_init down\n");
}