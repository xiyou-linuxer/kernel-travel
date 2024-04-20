#ifndef _LINUX_ALIGN_H
#define _LINUX_ALIGN_H

#include <linux/types.h>

#define ATA_SECTOR_SIZE 512//磁盘块的大小

#define HBA_PxIS_TFES   (1 << 30)       /* 若置位则代表文件有错误 */

/*磁盘状态*/
#define BIT_STAT_BSY 0x80	// 硬盘忙
#define BIT_STAT_DRQ 0x08	// 数据传输准备好了
#define BIT_STAT_DRDY 0x40	// 设备准备好 

/*磁盘控制命令*/
#define ATA_CMD_READ_DMA_EXT 0x25//读磁盘
#define ATA_CMD_WRITE_DMA_EXT 0x30//写磁盘
#define ATA_CMD_IDENTIFY 0xEC//读取磁盘信息

/*磁盘驱动程序的的返回结果*/
enum disk_result {
    AHCI_SUCCESS,       // 请求成功
    E_NOEMPTYSLOT,      // 没有空闲的命令槽位slot
    E_TASK_FILE_ERROR,  // 文件错误
	E_PORT_HUNG,
};

/*SATA控制器*/
#define HBA_CAP 0x000//HBA 特性寄存器
#define HBA_GHC 0x004//全局 HBA 控制寄存器
#define HBA_IS 0x008//中断状态寄存器
#define HBA_PI 0x00c//端口寄存器
#define HBA_VS 0x010// AHCI 版本寄存器
#define HBA_CCC_CTL 0x014//命令完成合并控制寄存器
#define HBA_CCC_PORTS 0x018// 命令完成合并端口寄存器
#define HBA_CAP2 0x024//HBA 特性扩展寄存器
#define HBA_BISTAFR 0x0A0// BIST 激活 FIS
#define HBA_BISTCR 0x0A4//BIST 控制寄存器
#define HBA_BISTCTR 0x0A8//BIST FIS 计数寄存器
#define HBA_BISTSR 0x0AC // BIST 状态寄存器
#define HBA_BISTDECR 0x0B0// BIST 双字错计数寄存器
#define HBA_OOBR 0x0BC//OOB 寄存器
#define HBA_TIMER1MS 0x0E0//1ms 计数寄存器
#define HBA_GPARAM1R 0x0E8//全局参数寄存器 1

/*GHC中填入的位*/
#define HBA_GHC_IE (1UL << 1)//HBA全局使能
#define HBA_GHC_AHCI_ENABLE (1UL << 31)//启用AHCI
#define HBA_GHC_RESET (1UL << 0)//复位标志

/*端口基地址*/
#define PORT_BASE 0x100 //端口基址
#define PORT_OFFEST 0X80 //端口偏移量
#define PORT_NR 0x20//一共有32端口

/*HBA 端口寄存器*/
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

/*向CMD寄存器中填入的位*/
#define HBA_PxCMD_ST    0x0001//表示端口的命令引擎的启动/停止位
#define HBA_PxCMD_FRE   0x0010//表示端口的 FIS 接收引擎
#define HBA_PxCMD_FR    0x4000//表示端口的 FIS 接收引擎状态位
#define HBA_PxCMD_CR    0x8000//表示端口的命令引擎状态位

/*向IE寄存器写入的位*/
#define HBA_PORT0_IE_DHRE (1UL << 0)//中断使能

#define HBA_PORT_IPM_ACTIVE 1//端口的 IPM状态为激活状态
#define HBA_PORT_DET_PRESENT 3//端口的 DET状态为已连接状态
 
/*设备签名*/
#define	SATA_SIG_ATA	0x00000101	// 普通的 SATA 硬盘驱动器
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI设备，识别支持 ATAPI 协议的 SATA 设备
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define SATA_SIG_PM 0x96690101          // Port multiplier

/*返回的设备类型*/
#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

/* 在SATA3.0规范中定义的Frame Information Structure类型*/
typedef enum {
    FIS_TYPE_REG_H2D = 0x27,    // Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,    // Register FIS - device to host
    FIS_TYPE_DMA_ACT = 0x39,    // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41,  // DMA setup FIS - bidirectional
    FIS_TYPE_DATA = 0x46,       // Data FIS - bidirectional
    FIS_TYPE_BIST = 0x58,       // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F,  // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS = 0xA1,   // Set device bits FIS - device to host
} FIS_TYPE;
/*主机到设备*/
struct fis_reg_host_to_device {
	uint8_t	fis_type;//FIS_TYPE_REG_H2D帧类型

	/*一个8位的位域如下指定出的整数*/
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

struct hba_received_fis {
	volatile struct fis_dma_setup fis_ds;
	volatile uint8_t pad0[4];
	
	volatile struct fis_pio_setup fis_ps;
	volatile uint8_t pad1[12];
	
	volatile struct fis_reg_device_to_host fis_r;
	volatile uint8_t pad2[4];
	
	volatile struct fis_dev_bits fis_sdb;
	volatile uint8_t ufis[64];
	volatile uint8_t reserved[0x100 - 0xA0];
}__attribute__ ((packed));

/*命令列表*/
struct hba_command_header {
	uint8_t fis_length:5;			//FIS 的长度
	uint8_t atapi:1;				//是否支持SATAPI
	uint8_t write:1;				//读写指令
	uint8_t prefetchable:1;
	
	uint8_t reset:1;
	uint8_t bist:1;					//指示是否执行内建自检
	uint8_t clear_busy_upon_r_ok:1;	//指示是否在读操作完成时清除繁忙位
	uint8_t reserved0:1;
	uint8_t pmport:4;				//指示端口多路复用的值
	
	uint16_t prdt_len;
	
	volatile uint32_t prdb_count;	//物理区域描述符字节长度
	
	uint64_t command_table_base;	//命令表的基址，即存储命令数据的地址
	
	uint32_t reserved1[4];
}__attribute__ ((packed));

struct hba_prdt_entry {
	uint64_t data_base;// 数据基址
	uint32_t reserved0;// 保留字段
	
	uint32_t byte_count:22;// 字节计数字段，占用22位
	uint32_t reserved1:9;// 保留字段，占用9位
	uint32_t interrupt_on_complete:1;// 完成时中断位，占用1位
}__attribute__ ((packed));

struct hba_command_table {
	uint8_t command_fis[64];// 64字节的命令FIS（帧信息结构）数组
	uint8_t acmd[16]; // 16字节的ATAPI命令数组
	uint8_t reserved[48];// 48字节的保留空间数组
	struct hba_prdt_entry prdt_entries[1];// 物理区域描述符表（PRDT）条目的数组
}__attribute__ ((packed));
void disk_init(void);

struct ata_identify {
	uint16_t ata_device;
	
	uint16_t dont_care[48];
	
	uint16_t cap0;
	uint16_t cap1;
	
	uint16_t obs[2];
	
	uint16_t free_fall;
	
	uint16_t dont_care_2[8];
	
	uint16_t dma_mode0;
	
	uint16_t pio_modes;
	
	uint16_t dont_care_3[4];
	
	uint16_t additional_supported;
	
	uint16_t rsv1[6];
	
	uint16_t serial_ata_cap0;
	
	uint16_t rsv2;
	
	uint16_t serial_ata_features;
	
	uint16_t serial_ata_features_enabled;
	
	uint16_t maj_ver;
	
	uint16_t min_ver;
	
	uint16_t features0;
	
	uint16_t features1;
	
	uint16_t features2;
	
	uint16_t features3;
	
	uint16_t features4;
	
	uint16_t features5;
	
	uint16_t udma_modes;
	
	uint16_t dont_care_4[11];
	
	uint64_t lba48_addressable_sectors;
	
	uint16_t wqewqe[2];
	
	uint16_t ss_1;
	
	uint16_t rrrrr[4];
	
	uint32_t ss_2;
	
	/* ...and more */
};

#endif