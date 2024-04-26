#include <asm/pci.h>
#include <linux/printk.h>
#include <asm-generic/io.h>
#include <linux/stdio.h>
#include <asm/pt_regs.h>
#include <asm/addrspace.h>
#include <linux/ahci.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/block_device.h>
#include <debug.h>
#include <trap/irq.h>
#define IO_BASE 0x8000000000000000
#define SOLT_NR 32
struct HbaMemReg *_hba_mem_reg = NULL;
struct HbaPortReg *_hba_port_reg[32];
struct HbaCmdList *_port_cmd_lst_base[ 32 ];
struct HbaRevFis* _port_rec_fis_base[32];
uint32_t hba_cmd_lst_len = 0x400U;
uint32_t hba_rec_fis_len = 0x100U;
char page[4096*32];
struct block_device_request_queue ahci_req_queue;

struct HbaCmdHeader* get_cmd_header( uint32_t port, uint32_t head_index )
{
	
	struct HbaCmdHeader *head = ( struct HbaCmdHeader*) (( uint64_t )_port_cmd_lst_base[port]|IO_BASE);
	head += head_index;
	return head;
}

struct HbaCmdTbl* get_cmd_table( uint32_t port, uint32_t slot_index )
{
	struct HbaCmdHeader *head = get_cmd_header( port, slot_index );
	uint64_t addr = ( uint64_t ) ( head->ctba );
	addr |= ( uint64_t ) ( head->ctbau ) << 32;
	addr |= IO_BASE;
	struct HbaCmdTbl *tbl = ( struct HbaCmdTbl * ) addr;
	return tbl;
}

void fill_fis_h2d_lba( struct FisRegH2D *fis, uint64_t lba )
{
	fis->lba_low = ( uint8_t ) ( ( lba >> 0 ) & 0xFF );
	fis->lba_mid = ( uint8_t ) ( ( lba >> 8 ) & 0xFF );
	fis->lba_high = ( uint8_t ) ( ( lba >> 16 ) & 0xFF );
	fis->lba_low_exp = ( uint8_t ) ( ( lba >> 24 ) & 0xFF );
	fis->lba_mid_exp = ( uint8_t ) ( ( lba >> 32 ) & 0xFF );
	fis->lba_high_exp = ( uint8_t ) ( ( lba >> 40 ) & 0xFF );
}

void send_cmd(uint32_t port, uint32_t cmd_slot )
{
	_hba_port_reg[ port ]->ci = 0x1U << cmd_slot;
}

int ahci_read(unsigned int port,unsigned int lba, unsigned int count, unsigned long buf)
{
	struct HbaCmdTbl *cmd_tbl = get_cmd_table( port, 0 );
	//assert( ( uint64_t ) cmd_tbl != 0x0UL );
	// 命令表使用的 FIS 为 H2D
	struct FisRegH2D *fis_h2d = ( struct FisRegH2D * ) cmd_tbl->cmd_fis;
	fis_h2d->fis_type = FIS_TYPE_REG_H2D;
	fis_h2d->pm_port = 0;		// 端口复用使用的值，这里写0就可以了，不涉及端口复用
	fis_h2d->c = 1;				// 表示这是一个主机发给设备的uint64 lba 命令帧
	fis_h2d->command = ATA_CMD_READ_DMA_EXT;
	fis_h2d->features = fis_h2d->features_exp = 0;		// refer to ATA8-ACS, this field should be N/A ( or 0 ) when the command is 'indentify' 
	fis_h2d->device = 1 << 6;							// similar to above 
	fill_fis_h2d_lba( fis_h2d, lba );
	fis_h2d->sector_cnt = fis_h2d->sector_cnt_exp = count;
	fis_h2d->control = 0;

	// 暂时直接引用 0 号命令槽
	uint32_t cmd_slot_num = 0;
	struct HbaCmdHeader* head = get_cmd_header( port, cmd_slot_num );
	// log_trace( "head address: %p", head );

	// 设置命令头 
	head->prdtl = 1;
	head->pmp = 0;
	head->c = 1;		// 传输结束清除忙状态
	head->b = 0;
	head->r = 0;
	head->p = 0;
	head->w = 0;
	head->a = 0;
	head->cfl = 5;
	head->prdbc = 0;	// should be set 0 before issue command 

	// head->ctba = ( uint32 ) loongarch::qemuls2k::virt_to_phy_address( ( uint64 ) cmd_tbl );
	// head->ctbau = ( uint32 ) ( loongarch::qemuls2k::virt_to_phy_address( ( uint64 ) cmd_tbl ) >> 32 );

	// 设置数据区 
	struct HbaPrd *prd0 = &cmd_tbl->prdt[ 0 ];
	prd0->dba = buf;
	prd0->interrupt = 1;
	prd0->dbc = (count << 9);
	send_cmd( port, cmd_slot_num );
	while (1)
    {
        if (!_hba_port_reg[port]->ci & 1) {
           
            break;
        }
        if (_hba_port_reg[port]->is & HBA_PxIS_TFES) // 如果HBA_PxIS_TFES被置位则说明访问异常
        {
            printk("Read disk error");
            return E_TASK_FILE_ERROR;
        } 
    }
	return AHCI_SUCCESS;	
}

int ahci_write(unsigned int port,unsigned int lba, unsigned int count, unsigned long buf)
{
	struct HbaCmdTbl *cmd_tbl = get_cmd_table( port, 0 );
	//assert( ( uint64_t ) cmd_tbl != 0x0UL );
	// 命令表使用的 FIS 为 H2D
	struct FisRegH2D *fis_h2d = ( struct FisRegH2D * ) cmd_tbl->cmd_fis;
	fis_h2d->fis_type = FIS_TYPE_REG_H2D;// 表示这是一个主机发给设备的uint64 lba 命令帧
	fis_h2d->pm_port = 0;		// 端口复用使用的值，这里写0就可以了，不涉及端口复用
	fis_h2d->command = ATA_CMD_WRITE_DMA_EXT;
	fis_h2d->features = fis_h2d->features_exp = 0;		// refer to ATA8-ACS, this field should be N/A ( or 0 ) when the command is 'indentify' 
	fill_fis_h2d_lba( fis_h2d, lba );
	fis_h2d->sector_cnt = 1;
	fis_h2d->sector_cnt_exp = 0;
	fis_h2d->c = 1;	
	fis_h2d->device = 1 << 6;		

	// 暂时直接引用 0 号命令槽
	uint32_t cmd_slot_num = 0;
	struct HbaCmdHeader* head = get_cmd_header( port, cmd_slot_num );
	// log_trace( "head address: %p", head );

	// 设置命令头 
	head->prdtl = 1;
	head->c = 1;		// 传输结束清除忙状态
	head->b = 0;
	head->r = 0;
	head->p = 0;
	head->w = 0;
	head->a = 0;
	head->cfl = 5;
	head->prdbc = 0;	// should be set 0 before issue command 

	// head->ctba = ( uint32 ) loongarch::qemuls2k::virt_to_phy_address( ( uint64 ) cmd_tbl );
	// head->ctbau = ( uint32 ) ( loongarch::qemuls2k::virt_to_phy_address( ( uint64 ) cmd_tbl ) >> 32 );

	// 设置数据区 
	struct HbaPrd *prd0 = &cmd_tbl->prdt[ 0 ];
	prd0->dba = ( uint64_t ) buf;
	prd0->interrupt = 1;
	prd0->dbc = 512 - 1;
	// 发布命令
	send_cmd( port, cmd_slot_num );
	while (1)
    {
        if (!_hba_port_reg[port]->ci & 1) {
           
            break;
        }
        if (_hba_port_reg[port]->is & HBA_PxIS_TFES) // 如果HBA_PxIS_TFES被置位则说明访问异常
        {
            printk("Read disk error");
            return E_TASK_FILE_ERROR;
        } 
    }
	return AHCI_SUCCESS;
}

static void port_rebase(int port_num)
{
	printk("port_rebase\n");
	// 配置 command list 地址
	_hba_port_reg[ port_num ]->clb = ( ( uint32_t ) ( uint64_t ) _port_cmd_lst_base[ port_num ] );
	_hba_port_reg[ port_num ]->clbu = ( uint32_t ) ( ( uint64_t ) _port_cmd_lst_base[ port_num ] >> 32 );
	// 分配 command table 
	struct HbaCmdHeader *head;
	uint64_t page;
	for (int j = 0; j < SOLT_NR; j++ )
	{
		head = get_cmd_header(port_num, j);
		/*page = ( uint64_t ) mm::k_pmm.alloc_page();
		page = loongarch::qemuls2k::virt_to_phy_address( page );*/
		head->ctba = (uint32_t)page;
		head->ctbau = ( uint32_t )(( uint32_t )page >> 32 );
		page+=4096;
	}

	// 配置 receive FIS 地址 
	_hba_port_reg[ port_num ]->fb = ( ( uint32_t ) ( uint64_t ) _port_rec_fis_base[ port_num ] );
	_hba_port_reg[ port_num ]->fbu = ( uint32_t ) ( ( uint64_t ) _port_rec_fis_base[ port_num ] >> 32 );

	// 使能中断
	_hba_mem_reg->ghc |= HBA_GHC_IE;
	_hba_port_reg[ port_num ]->ie |= HBA_PORT_IE_DHRE;

	// 启动设备 
	_hba_port_reg[ port_num ]->cmd |= HBA_PxCMD_FRE;
	_hba_port_reg[ port_num ]->cmd |= HBA_PxCMD_ST;
			
}

static int check_type(unsigned int port_num)
{
    uint32_t ssts = _hba_port_reg[port_num]->ssts;
    uint32_t sig = _hba_port_reg[port_num]->sig;
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
    
    uint32_t pi = _hba_mem_reg->pi;
    for (int i = 0; i < PORT_NR; ++i, (pi >>= 1))
    {
        if (pi & 1)
        {
            unsigned int dt = check_type(i);
            printk("ahci_probe_port dt:%d\n",dt);
            if (dt == AHCI_DEV_SATA)
            {
                port_rebase(i);
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
    printk("ahci_probe_port down\n");
}

void disk_init(void)
{
	
	pci_device_t *pci_dev=pci_get_device_by_bus(0, 8, 0);
	if (pci_dev==NULL)
    {
        printk(KERN_ERR "[ahci]: no AHCI controllers present!\n");
    }
	_hba_mem_reg =(struct HbaMemReg *)(0x8000000000000000|pci_dev->bar[0].base_addr);
	printk("%x",_hba_mem_reg);
	    /*注册中断处理程序*/
	irq_routing_set(0, 3, 19);

	for ( int i = 0; i < 32; i++ ){
		_hba_port_reg[ i ] = (struct HbaPortReg * )&_hba_mem_reg->ports[ i ];
	}
	printk("disk_init start\n");
	ahci_probe_port();
}
