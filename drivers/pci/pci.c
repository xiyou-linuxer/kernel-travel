#include <asm/pci.h>
#include <linux/printk.h>
#include <asm-generic/io.h>
#include <linux/stdio.h>

pci_device_t pci_device_table[PCI_MAX_DEVICE_NR];/*存储设备信息的结构体数组*/

static void pci_device_bar_init(pci_device_bar_t *bar, unsigned int addr_reg_val, unsigned int len_reg_val)
{
	/*if addr is 0xffffffff, we set it to 0*/
    if (addr_reg_val == 0xffffffff) {
        addr_reg_val = 0;
    }
	/*we judge type by addr register bit 0, if 1, type is io, if 0, type is memory*/
    if (addr_reg_val & 1) {
        bar->type = PCI_BAR_TYPE_IO;
		bar->base_addr = addr_reg_val  & PCI_BASE_ADDR_IO_MASK;
        bar->length    = ~(len_reg_val & PCI_BASE_ADDR_IO_MASK) + 1;
    } else {
        bar->type = PCI_BAR_TYPE_MEM;
        bar->base_addr = addr_reg_val  & PCI_BASE_ADDR_MEM_MASK;
        bar->length    = ~(len_reg_val & PCI_BASE_ADDR_MEM_MASK) + 1;
    }
}

void pci_device_bar_dump(pci_device_bar_t *bar)
{
    printk(KERN_DEBUG "pci_device_bar_dump: type: %s\n", bar->type == PCI_BAR_TYPE_IO ? "io base address" : "mem base address");
    printk(KERN_DEBUG "pci_device_bar_dump: base address: %x\n", bar->base_addr);
    printk(KERN_DEBUG "pci_device_bar_dump: len: %x\n", bar->length);
}

static pci_device_t *pci_alloc_device()
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (pci_device_table[i].flags == PCI_DEVICE_INVALID) {
			pci_device_table[i].flags = PCI_DEVICE_USING;
			return &pci_device_table[i];
		}
	}
	return NULL;
}

static int pci_free_device(pci_device_t *device)
{
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		if (&pci_device_table[i] == device) {
			device->flags = PCI_DEVICE_INVALID;
			return 0;
		}
	}
	return -1;
}

/*
*base_cfg_addr:设备类型对应的基地址
*bus：总线号
*device：设备号
*function：功能号
*reg_id：命令的偏移
*read_data：存放读入内容的内存地址
*/
static unsigned int pci_read_config(unsigned int base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * read_data)
{
	unsigned long pcie_header_base = base_cfg_addr| (bus << 16) | (device << 11)| (function<<8);
    *(read_data) = *(volatile unsigned int *)( pcie_header_base + (reg_id<<2));
    
}

static unsigned int pci_read_config(unsigned int base_cfg_addr, unsigned int bus, unsigned int device, unsigned int function, unsigned int reg_id, unsigned int * write_data)
{
	unsigned long pcie_header_base = base_cfg_addr| (bus << 16) | (device << 11)| (function<<8);
    *(volatile unsigned int *)( pcie_header_base + (reg_id<<2)) = write_data;
    
}

static void pci_device_init(
    pci_device_t *device,
    unsigned char bus,
    unsigned char dev,
    unsigned char function,
    unsigned short vendor_id,
    unsigned short device_id,
    unsigned int class_code,
    unsigned char revision_id,
    unsigned char multi_function
) {
	/*set value to device*/
    device->bus = bus;
    device->dev = dev;
    device->function = function;

    device->vendor_id = vendor_id;
    device->device_id = device_id;
    device->multi_function = multi_function;
    device->class_code = class_code;
    device->revision_id = revision_id;
	int i;
    for (i = 0; i < PCI_MAX_BAR; i++) {
         device->bar[i].type = PCI_BAR_TYPE_INVALID;
    }
    device->irq_line = -1;
}

static void pci_scan_device(unsigned char bus, unsigned char device, unsigned char function)
{
	/*读取总线设备的设备id*/
    unsigned int val;
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    unsigned int vendor_id = val & 0xffff;
    unsigned int device_id = val >> 16;
	/*总线设备不存在，直接返回*/
    if (vendor_id == 0xffff) {
        return;
    }
    
	/*分配一个空闲的pci设备信息结构体*/
	pci_device_t *pci_dev = pci_alloc_device();
	if(pci_dev == NULL){
		return;
	}

	/*读取设备类型*/
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    unsigned char header_type = ((val >> 16));
	/*读取 command 寄存器*/
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    /*将寄存器中的内容存入结构体 */
    pci_dev->command = val & 0xffff;
    pci_dev->status = (val >> 16) & 0xffff;
    
   // unsigned int command = val & 0xffff;
	/*pci_read_config class code and revision id*/
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    unsigned int classcode = val >> 8;
    unsigned char revision_id = val & 0xff;
	
	/*init pci device*/
    pci_device_init(pci_dev, bus, device, function, vendor_id, device_id, classcode, revision_id, (header_type & 0x80));
	
	/*init pci device bar*/
	int bar, reg;
    for (bar = 0; bar < PCI_MAX_BAR; bar++) {
        reg = PCI_BASS_ADDRESS0 + (bar*4);
		/*pci_read_config bass address[0~5] to get address value*/
        pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
		/*set 0xffffffff to bass address[0~5], so that if we pci_read_config again, it's addr len*/
        pci_write_config(PCI_CONFIG0_BASE,bus, device, function, reg, 0xffffffff);
       
	   /*pci_read_config bass address[0~5] to get addr len*/
        unsigned int len;
        pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&len);
        /*pci_write_config the io/mem address back to confige space*/
		pci_write_config(bus, device, function, reg, val);
		/*init pci device bar*/
        if (len != 0 && len != 0xffffffff) {
            pci_device_bar_init(&pci_dev->bar[bar], val, len);
        }
    }

    /* get card bus CIS pointer */
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    pci_dev->card_bus_pointer = val;

    /* get subsystem device id and vendor id */
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    pci_dev->subsystem_vendor_id = val & 0xffff;
    pci_dev->subsystem_device_id = (val >> 16) & 0xffff;
    
    /* get expansion ROM base address */
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    pci_dev->expansion_rom_base_addr = val;
    
    /* get capability list */
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    pci_dev->capability_list = val;
    
	/*get irq line and pin*/
    pci_read_config(PCI_CONFIG0_BASE,bus, device, function, PCI_DEVICE_VENDER,&val);
    if ((val & 0xff) > 0 && (val & 0xff) < 32) {
        unsigned int irq = val & 0xff;
        pci_dev->irq_line = irq;
        pci_dev->irq_pin = (val >> 8)& 0xff;
    }
    pci_dev->min_gnt = (val >> 16) & 0xff;
    pci_dev->max_lat = (val >> 24) & 0xff;
    
    printk(KERN_DEBUG "pci_scan_device: pci device at bus: %d, device: %d function: %d\n", 
        bus, device, function);
    pci_device_dump(pci_dev);

}


static void pci_scan_buses()
{
	unsigned int bus;
	unsigned char device, function;
	/*扫描每一条总线上的设备*/
    for (bus = 0; bus < PCI_MAX_BUS; bus++) {
        for (device = 0; device < PCI_MAX_DEV; device++) {
           for (function = 0; function < PCI_MAX_FUN; function++) {
				pci_scan_device(bus, device, function);
			}
        }
    }
}

void init_pci()
{
    /*init pci device table*/
	int i;
	for (i = 0; i < PCI_MAX_DEVICE_NR; i++) {
		pci_device_table[i].flags = PCI_DEVICE_INVALID;
	}

	/*scan all pci buses*/
	pci_scan_buses();

    printk(KERN_INFO "init_pci: pci type device found %d.\n", pic_get_device_connected());
}