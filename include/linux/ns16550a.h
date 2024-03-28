#ifndef _LINUX_NS16550A_H
#define _LINUX_NS16550A_H

#include <linux/types.h>

#ifndef CONFIG_2K1000LA
#define UART_BASE_ADDR	0x1fe001e0
#else
#define UART_BASE_ADDR	0x800000001fe20000ULL
#endif

/* 串口寄存器偏移地址 */
#define UART_RX      0       // 接收数据寄存器
#define UART_TX      0       // 发送数据寄存器
#define UART_IER     1       // 中断使能寄存器
#define UART_IIR     2       // 中断状态寄存器
#define UART_LCR     3       // 线路控制寄存器
#define UART_MCR     4       // 调制解调器控制寄存器
#define UART_LSR     5       // 线路状态寄存器
#define UART_MSR     6       // 调制解调器状态寄存器
#define UART_SR      7       // Scratch Register

#define UART_DIV_LATCH_LOW  0   // 分频锁存器1
#define UART_DIV_LATCH_HIGH 1   // 分频锁存器2

/* 线路控制寄存器 (LCR) */
#define UART_LCR_DLAB  0x80  // Divisor Latch Bit

/* 线路状态寄存器 (LSR) */
#define UART_LSR_DR   0x01   // Data Ready
#define UART_LSR_THRE 0x20   // Transmit Holding Register Empty

/* 初始化串口设备 */
void serial_ns16550a_init(uint32_t baud_rate);

/* 发送一个字符 */
void serial_ns16550a_putc(char c);

/* 接收一个字符 */
char serial_ns16550a_getc(void);

/* 发送一个字符串 */
void serial_ns16550a_puts(char *str);

#endif /* _LINUX_NS16550A_H */
