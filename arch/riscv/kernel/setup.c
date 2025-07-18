#include<asm/bootinfo.h>
#include <asm/asm.h>

#include <xkernel/init.h>
#include <xkernel/types.h>
#include <xkernel/fdt.h>
#include <xkernel/printk.h>

#define COMMAND_LINE_SIZE	512
#define FIX_BTMAPS_SLOTS	7
#define NR_FIX_BTMAPS 2  // 每个slot占用的fixmap条目数，具体值依据平台
#define FIXADDR_TOP      0xfffff000UL
#define PAGE_SHIFT       12
#define __fix_to_virt(x) (FIXADDR_TOP - ((x) << PAGE_SHIFT))
#define FIX_BTMAP_BEGIN  50  // 你可以根据需要定义这个索引值
static void *prev_map[FIX_BTMAPS_SLOTS];
static void *slot_virt[FIX_BTMAPS_SLOTS];

char  boot_command_line[COMMAND_LINE_SIZE];
void  early_ioremap_setup(void)
{
	int i;

	for (i = 0; i < FIX_BTMAPS_SLOTS; i++) {
		//WARN_ON_ONCE(prev_map[i]);   逻辑未完成
		slot_virt[i] = __fix_to_virt(FIX_BTMAP_BEGIN - NR_FIX_BTMAPS*i);
	}
}
unsigned long boot_cpu_hartid;

static void __init parse_dtb(void);

void  parse_early_param(void)
{
	static int done __initdata;
	static char tmp_cmdline[COMMAND_LINE_SIZE] __initdata;

	if (done)
		return;

	//strscpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
	//parse_early_options(tmp_cmdline);
	done = 1;
}

void setup_arch(void)
{
	// 解析设备树
	parse_dtb();
	early_ioremap_setup();//早期内存映射（early ioremap）机制的初始化函数

	parse_early_param();
	return;
}


static void __init parse_dtb(void){
	// 解析从启动参数中传入的 dtb_early_va
	if(early_init_dt_scan(dtb_early_va)){
		pr_info("1");
	}
	
}