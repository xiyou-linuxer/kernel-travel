#include <linux/block_device.h>
#include <linux/ahci.h>
#include <linux/stdio.h>

/*打包请求*/
 /*struct block_device_request_packet **/void block_make_request(int cmd, uint64_t base_addr, uint64_t count, uint64_t buffer,uint8_t port_num)
{
    /*struct block_device_request_packet *pack = (struct block_device_request_packet *)kmalloc(sizeof(struct block_device_request_packet), 0);
    //pack->pthread = running_thread();
    pack->LBA_startl = base_addr&0xffff;
    pack->LBA_starth = (base_addr >> 32);
    pack->count = count;
    pack->buffer_vaddr = buffer;
    pack->port_num = port_num*PORT_OFFEST+PORT_BASE;
    // 由于ahci不需要中断即可读取磁盘，因此end handler为空
    switch (cmd)
    {
    case ATA_CMD_READ_DMA_EXT:
        pack->end_handler = NULL;
        pack->cmd = ATA_CMD_READ_DMA_EXT;
        ahci_read(pack->port_num,pack->LBA_startl,pack->LBA_starth,pack->count,pack->buffer_vaddr);
        break;
    case ATA_CMD_WRITE_DMA_EXT:
        pack->end_handler = NULL;
        pack->cmd = ATA_CMD_WRITE_DMA_EXT;
        break;
    default:
        pack->end_handler = NULL;
        pack->cmd = cmd;
        ahci_write(pack->port_num,pack->LBA_startl,pack->LBA_starth,pack->count,pack->buffer_vaddr);
        break;
    }
    //return pack;*/
	int prot_base = port_num*PORT_OFFEST+PORT_BASE;
	int LBA_startl = base_addr&0xffff;
	int LBA_starth = (base_addr >> 32);
	switch (cmd)
	{
	case ATA_CMD_READ_DMA_EXT:
		printk("block_make_request\n");
		ahci_read(prot_base, LBA_startl, LBA_starth, count, buffer);
		break;
	case ATA_CMD_WRITE_DMA_EXT:
		ahci_write(prot_base,LBA_startl,LBA_starth,count,buffer);
		break;
	default:
		break;
	}
}
/*磁盘读取
*base_addr：磁盘起始扇区
*count：要读入的扇区数
*buffer：缓冲区
*port_num：磁盘端口号
*/
void block_read(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num)
{
    printk("block_read\n");

    /*struct block_device_request_packet * pack=*/ block_make_request(ATA_CMD_READ_DMA_EXT,base_addr, count, buffer,port_num);

    //ahci_submit(pack);
}

/*磁盘写入
*base_addr：磁盘起始扇区
*count：要写入的扇区数
*buffer：缓冲区
*port_num：磁盘端口号
*/
void block_write(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num) 
{
    /*struct block_device_request_packet * pack= */block_make_request(ATA_CMD_WRITE_DMA_EXT,base_addr, count, buffer,port_num);
    //ahci_submit(pack);
}

