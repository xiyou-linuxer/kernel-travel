# pci 总线驱动

pci 总线上连接了磁盘，网卡等众多设备，这些设备信息都需要通过pci总线驱动程序获取

## 读写配置信息

在用户手册中，将pci设备定义为普通pci设备（type0）与桥接设备（type1）。
在64位地址模式下，type0型设备配置头的基地址为0xfe00000000。type1则为0xfe10000000。

```c
#define PCI_CONFIG0_BASE 0xfe00000000
#define PCI_CONFIG1_BASE 0xfe10000000
```

在loongarch架构中采取统一编址的方式，因此在访问pci配置头空间时只需要计算出他们在内存中的地址即可。

访问外设时虚拟地址要使用0x8000000000000000作为基地址，加上设备头的基地址，以及总线号偏移，设备号偏移，功能号偏移，寄存器偏移，各自的偏移量最终形成存储该设备的配置信息的地址。

在《龙芯 2K1000LA 处理器用户手册》71页，图 6-432 中给出了具体的地址访问方式

```c
static void pci_read_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * read_data)
{
    unsigned long pcie_header_base = CSR_DMW0_BASE|base_cfg_addr| (bus << 16) | (device << 11)| (function<<8);
    *read_data = *(volatile unsigned int *)(pcie_header_base + reg_id) ; 
}

static void pci_write_config(unsigned long base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * write_data)
{
    unsigned long pcie_header_base = CSR_DMW0_BASE|base_cfg_addr| (bus << 16) | (device << 11)| (function<<8);
    *(volatile unsigned int *)( pcie_header_base + reg_id) = write_data;
    
}
```

## 解析bar地址

bar地址分为I/O空间与MEM两种类型。

该地址被存储在寄存器 Base Address Register 中，该寄存器中的位信息如下：

基地址寄存器

* Bit0:标志位，若为1则为io空间，若为0则为mem空间

I/O基地址寄存器:

* Bit1:保留
* Bit31-2:基地址单元
* Bit63-32:保留

MEM 基地址存储器:

* Bit2-1:MEM 基地址寄存器-译码器宽度单元,00-32 位,10-64 位
* Bit3:预提取属性
* Bit64-4:基地址单元

将无关位使用掩码屏蔽后就能得到pci设备的bar地址。

## 扫描pci设备

系统中定义了 pci_device_t 类型的结构体数组 pci_device_table 存储扫描到得 pci 设备信息。

```c
typedef struct pci_device
{
    int flags;     /*device flags标记该结构体是否被使用*/

    unsigned char bus;              /*bus总线号*/
    unsigned char dev;              /*device号*/
    unsigned char function;         /*功能号*/

    unsigned short vendor_id;       /*配置空间:Vendor ID*/
    unsigned short device_id;       /*配置空间:Device ID*/
    unsigned short command;         /*配置空间:Command*/
    unsigned short status;          /*配置空间:Status*/
    
    unsigned int class_code;        /*配置空间:Class Code*/
    unsigned char revision_id;      /*配置空间:Revision ID*/
    unsigned char multi_function;   /*多功能标志*/
    unsigned int card_bus_pointer;
    unsigned short subsystem_vendor_id;
    unsigned short subsystem_device_id;
    unsigned int expansion_rom_base_addr;
    unsigned int capability_list;
    
    unsigned char irq_line;     /*配置空间:IRQ line*/
    unsigned char irq_pin;      /*配置空间:IRQ pin*/
    unsigned char min_gnt;
    unsigned char max_lat;
    pci_device_bar_t bar[PCI_MAX_BAR];  /*有6个地址信息*/
} pci_device_t;
```

在系统初始化时调用pci_init()函数，将扫描总线上连接的所有的设备，读取它们的设备信息，并将其存储在结构体数组pci_device_table中。

## 获取设备信息

pci驱动程序向外提供多种接口，可以通过已知的设备信息获取存储设备信息的结构体。

```c
pci_device_t* pci_get_device(unsigned int vendor_id, unsigned int device_id);//通过设备id与厂商id获取设备信息
pci_device_t* pci_get_device_by_bus(unsigned int bus, unsigned int dev,unsigned int function);//通过总线号，设备号，功能号获取设备信息
```
