#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include <xkernel/types.h>
#include <asm/pt_regs.h>

typedef void (*intr_handler)(struct pt_regs *regs);
void irq_init(void);

/* 定义中断的两种状态:
 * INTR_OFF值为0,表示关中断,
 * INTR_ON值为1,表示开中断 */
enum intr_status {		 // 中断状态
    INTR_OFF,			 // 中断关闭
    INTR_ON		         // 中断打开
};

#define INTENSET_0 0x1fe01428//低 32 位设置中断使能的寄存器
#define INTEN_0 0x1fe01424//低 32 位中断使能状态寄存器读取中断状态
#define INTENSET_1 0x1fe01468//高 32 位设置中断使能的寄存器
#define INTEN_1 0x1fe01464//高 32 位中断使能状态寄存器读取中断状态
#define IO_ENTRY_0 0x1fe01400//中断路由寄存器基址,低32
#define IO_ENTRY_1 0x1fe01440//中断路由寄存器基址,高32
bool in_interrupt(void);
enum intr_status intr_get_status(void);
enum intr_status intr_set_status (enum intr_status);
enum intr_status intr_enable (void);
enum intr_status intr_disable (void);
void register_handler(uint8_t vector_no, intr_handler function);
void irq_routing_set(uint8_t cpu, uint8_t IPx, uint8_t source_num);

#endif
