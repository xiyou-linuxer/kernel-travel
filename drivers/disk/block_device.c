#include <block_device.h>
#include <ahci.h>
/*磁盘写入
*base_addr：磁盘起始扇区
*要写入的扇区数
*port_num：磁盘端口号
*/
block_write(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num) 
{
    struct block_device_request_packet * pack= block_make_request(ATA_CMD_WRITE_DMA_EXT,base_addr, count, buffer,port_num);
    ahci_submit(pack);
}
/*磁盘读取
*
*/
block_read(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num)
{
    struct block_device_request_packet * pack= block_make_request(ATA_CMD_READ_DMA_EXT,base_addr, count, buffer,port_num);
    ahci_submit(pack);
}
static struct block_device_request_packet *block_make_request(int cmd, uint64_t base_addr, uint64_t count, uint64_t buffer,uint8_t port_num)
{
    struct block_device_request_packet *pack = (struct block_device_request_packet *)kmalloc(sizeof(struct block_device_request_packet), 0);

    pack->pthread = running_thread();

    // 由于ahci不需要中断即可读取磁盘，因此end handler为空
    switch (cmd)
    {
    case ATA_CMD_READ_DMA_EXT:
        pack->end_handler = NULL;
        pack->cmd = ATA_CMD_READ_DMA_EXT;
        break;
    case ATA_CMD_WRITE_DMA_EXT:
        pack->end_handler = NULL;
        pack->cmd = ATA_CMD_WRITE_DMA_EXT;
        break;
    default:
        pack->end_handler = NULL;
        pack->cmd = cmd;
        break;
    }

    pack->LBA_start = base_addr;
    pack->count = count;
    pack->buffer_vaddr = buffer;
    pack->port_num = port_num*PORT_OFFEST+PORT_BASE;
    return pack;
}
