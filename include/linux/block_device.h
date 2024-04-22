#ifndef _LINUX_BLOCK_DEVICE_H
#define _LINUX_BLOCK_DEVICE_H

#include <linux/thread.h>
#include <linux/list.h>

struct block_device_request_packet
{
    int cmd;//读写指令
    uint32_t LBA_startl;//起始磁盘逻辑扇区号，低32位
    uint32_t LBA_starth;//高32位
    uint32_t count;//要读取的磁盘扇区数
    uint64_t buffer_vaddr;//缓冲区
    uint8_t port_num;   // ahci的设备端口号
    void (*end_handler)(unsigned long num, unsigned long arg);//中断处理函数
    struct task_struct* pthread;//发出请求的任务控制块指针
    struct list_elem* node;//节点
};

struct block_device_request_queue
{
    struct list queue_list;
    struct block_device_request_packet * in_service;    // 正在请求的结点
    unsigned long request_count;
};

void block_read(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num);
void block_write(uint64_t base_addr, uint64_t count, uint64_t buffer, uint8_t port_num);
#endif