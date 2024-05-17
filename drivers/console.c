#include <xkernel/console.h>
#include <xkernel/ns16550a.h>
#include <sync.h>
#include <xkernel/thread.h>
static struct lock console_lock;    // 控制台锁

/* 初始化终端 */
void console_init() {
	lock_init(&console_lock); 
}

/* 获取终端 */
void console_acquire() {
	lock_acquire(&console_lock);
}

/* 释放终端 */
void console_release() {
	lock_release(&console_lock);
}

/* 终端中输出字符串 */
void console_put_str(char* str) {
	console_acquire(); 
	serial_ns16550a_puts(str); 
	console_release();
}
/* 终端中输出字符 */
void console_put_char(uint8_t char_asci) {
	console_acquire(); 
	serial_ns16550a_putc(char_asci); 
	console_release();
}

void sys_putchar(uint8_t c) { console_put_char(c); }