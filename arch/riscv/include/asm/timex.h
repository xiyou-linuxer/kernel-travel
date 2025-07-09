#ifndef __TIMERX_H
#define __TIMERX_H
#include "xkernel/types.h"
#define CLOCK_TICK_RATE	1193180 /* 时钟滴答*/


typedef unsigned long cycles_t;

/* 
* @brief 读取 time 寄存器，获取当前时间周期数。
* @return 当前的周期数 cyclest -> unsigned long
*/
static inline cycles_t get_cycles(void)
{
	// return readq_relaxed(clint_time_val);
     cycles_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n));
    return n;
}

extern void time_init(void);

#endif