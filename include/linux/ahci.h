#ifndef _LINUX_ALIGN_H
#define _LINUX_ALIGN_H
//磁盘状态
#define BIT_STAT_BSY	 0x80	      // 硬盘忙
#define BIT_STAT_DRDY	 0x40	      // 设备准备好	 
#define BIT_STAT_DRQ	 0x8	      // 数据传输准备好了

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY	   0xec	    // identify指令
#define CMD_READ_SECTOR	   0x20     // 读扇区指令
#define CMD_WRITE_SECTOR   0x30	    // 写扇区指令


/**
 * @brief 在SATA3.0规范中定义的Frame Information Structure类型
 *
 */
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


#endif