#ifndef _BLOCK_DEVICE_H
#define _BLOCK_DEVICE_H

#include <thread.h>
#include <list.h>

struct block_device_request_packet
{
    int cmd;//读写指令
    uint64_t LBA_start;//起始磁盘逻辑扇区号
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
#endif