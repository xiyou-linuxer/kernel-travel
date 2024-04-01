#ifndef _ASM_PCI_H
#define _ASM_PCI_H
#include <./include/linux/types.h>
#include<asm-generic/io.h>

// pci设备结构的通用标题字段
struct pci_device_structure_header_t
{
    //struct List list;
    // ==== 以下三个变量表示该结构体所处的位置
    uint8_t bus;
    uint8_t device;
    uint8_t func;

    uint16_t Vendor_ID; // 供应商ID 0xffff是一个无效值，在读取访问不存在的设备的配置空间寄存器时返回
    uint16_t Device_ID; // 设备ID，标志特定设备

    uint16_t Command; // 提供对设备生成和响应pci周期的能力的控制 向该寄存器写入0时，设备与pci总线断开除配置空间访问以外的所有连接
    uint16_t Status;  // 用于记录pci总线相关时间的状态信息寄存器

    uint8_t RevisionID; // 修订ID，指定特定设备的修订标志符
    uint8_t ProgIF;     // 编程接口字节，一个只读寄存器，指定设备具有的寄存器级别的编程接口（如果有的话）
    uint8_t SubClass;   // 子类。指定设备执行的特定功能的只读寄存器
    uint8_t Class_code; // 类代码，一个只读寄存器，指定设备执行的功能类型

    uint8_t CacheLineSize; // 缓存线大小：以 32 位为单位指定系统缓存线大小。设备可以限制它可以支持的缓存线大小的数量，如果不支持的值写入该字段，设备将表现得好像写入了 0 值
    uint8_t LatencyTimer;  // 延迟计时器：以 PCI 总线时钟为单位指定延迟计时器。
    uint8_t HeaderType;    // 标头类型 a value of 0x0 specifies a general device, a value of 0x1 specifies a PCI-to-PCI bridge, and a value of 0x2 specifies a CardBus bridge. If bit 7 of this register is set, the device has multiple functions; otherwise, it is a single function device.
    uint8_t BIST;          // Represents that status and allows control of a devices BIST (built-in self test).
                           // Here is the layout of the BIST register:
                           // |     bit7     |    bit6    | Bits 5-4 |     Bits 3-0    |
                           // | BIST Capable | Start BIST | Reserved | Completion Code |
                           // for more details, please visit https://wiki.osdev.org/PCI
} __attribute__((packed));

/**
 * @brief 表头类型为0x0的pci设备结构,一般的pci设备
 *
 */

struct pci_device_structure_general_device_t
{
    struct pci_device_structure_header_t header;
    uint32_t BAR0;
    uint32_t BAR1;
    uint32_t BAR2;
    uint32_t BAR3;
    uint32_t BAR4;
    uint32_t BAR5;
    uint32_t Cardbus_CIS_Pointer; // 指向卡信息结构，供在 CardBus 和 PCI 之间共享芯片的设备使用。

    uint16_t Subsystem_Vendor_ID;
    uint16_t Subsystem_ID;

    uint32_t Expansion_ROM_base_address;

    uint8_t Capabilities_Pointer;
    uint8_t reserved0;
    uint16_t reserved1;

    uint32_t reserved2;

    uint8_t Interrupt_Line; // 指定设备的中断引脚连接到系统中断控制器的哪个输入，并由任何使用中断引脚的设备实现。对于 x86 架构，此寄存器对应于 PIC IRQ 编号 0-15（而不是 I/O APIC IRQ 编号），并且值0xFF定义为无连接。
    uint8_t Interrupt_PIN;  // 指定设备使用的中断引脚。其中值为0x1INTA#、0x2INTB#、0x3INTC#、0x4INTD#，0x0表示设备不使用中断引脚。
    uint8_t Min_Grant;      // 一个只读寄存器，用于指定设备所需的突发周期长度（以 1/4 微秒为单位）（假设时钟速率为 33 MHz）
    uint8_t Max_Latency;    // 一个只读寄存器，指定设备需要多长时间访问一次 PCI 总线（以 1/4 微秒为单位）。
} __attribute__((packed));


#endif