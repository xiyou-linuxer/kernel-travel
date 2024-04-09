#ifndef _ASM_PCI_H
#define _ASM_PCI_H

#include <linux/types.h>

#define PCI_CONFIG0_BASE 0xfe00000000	/*PCI配置空间数据端口,type0 一般设备*/
#define PCI_CONFIG1_BASE 0xfe10000000   /*PCI配置空间数据端口,type1 PCI Bridge*/

#define PCI_MAX_BAR 6		/*每个设备最多有6地址信息*/
#define PCI_MAX_BUS 256		/*PCI总共有256个总线*/
#define PCI_MAX_DEV 32		/*PCI每条总线上总共有32个设备*/
#define PCI_MAX_FUN 8		/*PCI设备总共有8个功能号*/
#define PCI_MAX_DEVICE_NR 256	/*系统最大支持检测多少个设备*/

/*PCI配置空间数据的偏移*/
#define PCI_DEVICE_VENDER								0x00	/* 设备厂商ID和设备ID的寄存器偏移量*/
#define PCI_STATUS_COMMAND								0x04	/*状态和命令寄存器偏移量*/
#define PCI_CLASS_CODE_REVISION_ID						0x08	/*类型、子类型、次类型和修订号寄存器偏移量*/
#define PCI_BIST_HEADER_TYPE_LATENCY_TIMER_CACHE_LINE	0x0C    /*BIST（Built-In Self-Test，内建自测）、头部类型、延迟计时器和缓存行寄存器偏移量。*/
#define PCI_BASS_ADDRESS0								0x10/*（BARs，基地址寄存器）0到5的偏移量。*/
#define PCI_BASS_ADDRESS1								0x14
#define PCI_BASS_ADDRESS2								0x18
#define PCI_BASS_ADDRESS3								0x1C
#define PCI_BASS_ADDRESS4								0x20
#define PCI_BASS_ADDRESS5								0x24
#define PCI_CARD_BUS_POINTER							0x28/*卡总线指针寄存器偏移量*/
#define PCI_SUBSYSTEM_ID							    0x2C/*子系统ID寄存器偏移量*/
#define PCI_EXPANSION_ROM_BASE_ADDR                     0x30/*扩展ROM基地址寄存器偏移量*/
#define PCI_CAPABILITY_LIST                             0x34/*能力列表寄存器偏移量*/
#define PCI_RESERVED							        0x38/*保留寄存器偏移量*/
#define PCI_MAX_LNT_MIN_GNT_IRQ_PIN_IRQ_LINE			0x3C/*最大的总线延迟时间、最小的总线延迟时间、最大的访问延迟时间、中断引脚、中断线寄存器偏移量*/

/*向command中写入的控制位*/
#define  PCI_COMMAND_IO		0x1	/* I/O 空间的响应使能 */
#define  PCI_COMMAND_MEMORY	0x2	/* 内存空间的响应使能 */
#define  PCI_COMMAND_MASTER	0x4	/* 总线主控制使能 */
#define  PCI_COMMAND_SPECIAL	0x8	/* 特殊事务响应使能 */
#define  PCI_COMMAND_INVALIDATE	0x10	/* 使用内存写入并且使其失效 */
#define  PCI_COMMAND_VGA_PALETTE 0x20	/* 启用VGA调色板的监听 */
#define  PCI_COMMAND_PARITY	0x40	/* 启用奇偶校验检查 */
#define  PCI_COMMAND_WAIT	0x80	/* 启用地址/数据步进 */
#define  PCI_COMMAND_SERR	0x100	/* 启用系统错误（SERR） */
#define  PCI_COMMAND_FAST_BACK	0x200	/* 启用快速回写 */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx模拟禁用 */

/*IO地址和MEM地址的地址mask*/
#define PCI_BASE_ADDR_MEM_MASK           (~0x0FUL)
#define PCI_BASE_ADDR_IO_MASK            (~0x03UL)

/* PCI设备ID */
struct pci_device_id
{
	uint32_t vendor, device;   //供应商和设备 ID 或 PCI_ANY_ID
	uint32_t subvendor, subdevice;   //子系统的 id 或 PCI_ANY_ID
	uint32_t device_class, class_mask;   //
};

/*PCI地址bar结构体，保存Bass Address （0~5）的信息*/
typedef struct pci_device_bar
{
	unsigned int type;		    /*地址bar的类型（IO地址/MEM地址）*/
    unsigned int base_addr;	    /*地址的值*/
    unsigned int length;	    /*地址的长度*/
} pci_device_bar_t;

#define PCI_DEVICE_INVALID 		0/*被定义为 0，表示无效的PCI设备*/
#define PCI_DEVICE_USING		1/*被定义为 1，表示正在使用的PCI设备*/

/*PCI BAR的类型*/
#define PCI_BAR_TYPE_INVALID 	0
#define PCI_BAR_TYPE_MEM 		1
#define PCI_BAR_TYPE_IO 		2

/*
PCI设备结构体，用于保存我们所需要的pci信息，并不是和硬件的一样
*/
typedef struct pci_device
{
	int flags; 		/*device flags*/
	
	unsigned char bus;				/*bus总线号*/
	unsigned char dev;				/*device号*/
	unsigned char function;		/*功能号*/
	
	unsigned short vendor_id;		/*配置空间:Vendor ID*/
    unsigned short device_id;		/*配置空间:Device ID*/
    unsigned short command;		    /*配置空间:Command*/
    unsigned short status;		    /*配置空间:Status*/
    
    unsigned int class_code;		/*配置空间:Class Code*/
	unsigned char revision_id;		/*配置空间:Revision ID*/
    unsigned char multi_function;	/*多功能标志*/
    unsigned int card_bus_pointer;
    unsigned short subsystem_vendor_id;
    unsigned short subsystem_device_id;
    unsigned int expansion_rom_base_addr;
    unsigned int capability_list;
    
    unsigned char irq_line;			/*配置空间:IRQ line*/
    unsigned char irq_pin;			/*配置空间:IRQ pin*/
    unsigned char min_gnt;
    unsigned char max_lat;
    pci_device_bar_t bar[PCI_MAX_BAR];	/*有6个地址信息*/
} pci_device_t;
#endif


void init_pci();//初始化pci