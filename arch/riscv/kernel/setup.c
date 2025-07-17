#include<asm/bootinfo.h>
#include <asm/asm.h>

#include <xkernel/init.h>
#include <xkernel/types.h>
#include <xkernel/fdt.h>
#include <xkernel/printk.h>
unsigned long boot_cpu_hartid;

static void __init parse_dtb(void);

void setup_arch(void)
{
	// 解析设备树
	parse_dtb();




	return;
}


static void __init parse_dtb(void){
	// 解析从启动参数中传入的 dtb_early_va
	if(early_init_dt_scan(dtb_early_va)){
		pr_info("1");
	}

}