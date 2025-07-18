#include<asm/bootinfo.h>
#include <asm/asm.h>
#include <asm/csr.h>

#include <xkernel/init.h>
#include <xkernel/types.h>
#include <xkernel/fdt.h>
#include <xkernel/printk.h>

void trap_init(void){
    csr_write(CSR_SSCRATCH, 0);                     // 清除 scratch 区
}