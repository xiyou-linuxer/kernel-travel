#ifndef _LINUX_ALIGN_H
#define _LINUX_ALIGN_H

#include <linux/types.h>

#define ATA_SECTOR_SIZE 512//磁盘块的大小

/*磁盘状态*/
#define BIT_STAT_BSY 0x80	// 硬盘忙
#define BIT_STAT_DRQ 0x08	// 数据传输准备好了
#define BIT_STAT_DRDY 0x40	// 设备准备好 

/*磁盘控制命令*/
#define ATA_CMD_READ_DMA_EXT 0x25//读磁盘
#define ATA_CMD_WRITE_DMA_EXT 0x30//写磁盘

/*端口基地址*/
#define PORT0_BASE 0x100 //端口0
#define PORT1_BASE 0X180 //端口1

/*磁盘驱动程序的的返回结果*/
enum disk_result{
	AHCI_SUCCESS ,  	// 请求成功
}

/*HBA 寄存器*/
#define PORT_CLB 0X00//命令列表基地址低 32 位
#define PORT_CLBU 0X04//命令列表基地址高 32 位
#define PORT_FB 0X08//FIS 基地址低 32 位
#define PORT_FBU 0X0C//FIS 基地址高 32 位
#define PORT_IS 0X10//中断状态寄存器
#define PORT_IE 0X14//中断使能寄存器
#define PORT_CMD 0X18//命令寄存器
#define PORT_TFD 0X20//任务文件数据寄存器
#define PORT_SIG 0X24//签名寄存器
#define PORT_SSTS 0X28//SATA 状态寄存器
#define PORT_SCTL 0X2C//SATA 控制寄存器
#define PORT_SERR 0X30//SATA 错误寄存器
#define PORT_SACT 0X34//SATA 激活寄存器
#define PORT_CI 0X38//命令发送寄存器
#define PORT_SNTF 0X03C//SATA 命令通知寄存器
#define PORT_DMACR 0X70//DMA 控制寄存器
#define PORT_PHYCR 0X78//PHY 控制寄存器
#define PORT_PHYSR 0X7C//PHY 状态寄存器

/*向CMD寄存器中填入*/
#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

/* 在SATA3.0规范中定义的Frame Information Structure类型*/
typedef enum
{
    FIS_TYPE_REG_H2D = 0x27,   // 寄存器FIS - 主机到设备
    FIS_TYPE_REG_D2H = 0x34,   // 寄存器FIS - 设备到主机
    FIS_TYPE_DMA_ACT = 0x39,   // DMA激活FIS - 设备到主机
    FIS_TYPE_DMA_SETUP = 0x41, // DMA设置FIS - 双向
    FIS_TYPE_DATA = 0x46,      // 数据FIS - 双向
    FIS_TYPE_BIST = 0x58,      // BIST激活FIS - 双向
    FIS_TYPE_PIO_SETUP = 0x5F, // PIO设置FIS - 设备到主机
    FIS_TYPE_DEV_BITS = 0xA1,  // 设置设备位FIS - 设备到主机
} FIS_TYPE;

/*主机到设备*/
struct fis_reg_host_to_device {
	uint8_t	fis_type;//FIS_TYPE_REG_H2D帧类型
	
	uint8_t pmport:4;//端口多路复用器编号，表示该 FIS 目标设备的端口号。在这个结构中，占用了 4 个位，表示端口号范围为 0 到 15
	uint8_t reserved0:3;
	uint8_t c:1;//用于指示该 FIS 是一个命令 FIS。
	
	uint8_t command;//命令寄存器，用于存储主机发送给设备的命令
	uint8_t feature_l;
	/*lba0~5存储磁盘逻辑块地址的各个字节*/
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;//设备寄存器
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t feature_h;
	
	uint8_t count_l;
	uint8_t count_h;
	uint8_t icc;
	uint8_t control;
	
	uint8_t reserved1[4];
}__attribute__ ((packed));

/*设备到主机*/
struct fis_reg_device_to_host {
	uint8_t fis_type;
	
	uint8_t pmport:4;
	uint8_t reserved0:2;
	uint8_t interrupt:1;
	uint8_t reserved1:1;
	
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;
	
	uint8_t count_l;
	uint8_t count_h;
	uint8_t reserved3[2];
	
	uint8_t reserved4[4];
}__attribute__ ((packed));

struct fis_data {
	uint8_t fis_type;
	uint8_t pmport:4;
	uint8_t reserved0:4;
	uint8_t reserved1[2];
	
	uint32_t data[1];
}__attribute__ ((packed));

/*由设备发送给主机，告诉主机相关PIO操作参数*/
struct fis_pio_setup {
	uint8_t fis_type;
	
	uint8_t pmport:4;
	uint8_t reserved0:1;
	uint8_t direction:1;
	uint8_t interrupt:1;
	uint8_t reserved1:1;
	
	uint8_t status;
	uint8_t error;
	
	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;
	
	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t reserved2;
	
	uint8_t count_l;
	uint8_t count_h;
	uint8_t reserved3;
	uint8_t e_status;
	
	uint16_t transfer_count;
	uint8_t reserved4[2];
}__attribute__ ((packed));

/*发送方通过发送这个FIS，要求对方配置好DMA控制器*/
struct fis_dma_setup {
	uint8_t fis_type;
	
	uint8_t pmport:4;
	uint8_t reserved0:1;
	uint8_t direction:1;
	uint8_t interrupt:1;
	uint8_t auto_activate:1;
	
	uint8_t reserved1[2];
	
	uint64_t dma_buffer_id;
	
	uint32_t reserved2;
	
	uint32_t dma_buffer_offset;
	
	uint32_t transfer_count;
	
	uint32_t reserved3;
}__attribute__ ((packed));

/*让对方进入测试模式，是一个双向可用的FIS，接收方以R_OK回应，完成测试工作之后就进入BIST交换状态*/
struct fis_dev_bits {
	volatile uint8_t fis_type;
	
	volatile uint8_t pmport:4;
	volatile uint8_t reserved0:2;
	volatile uint8_t interrupt:1;
	volatile uint8_t notification:1;
	
	volatile uint8_t status;
	volatile uint8_t error;
	
	volatile uint32_t protocol;
}__attribute__ ((packed));

#endif