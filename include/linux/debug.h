#ifndef _LINUX_DEBUG_H
#define _LINUX_DEBUG_H

extern void panic_spin(char* filename, int line, const char* func);

/***************************  __VA_ARGS__  *******************************
 * __VA_ARGS__ 是预处理器所支持的专用标识符。
 * 代表所有与省略号相对应的参数. 
 * "..."表示定义的宏其参数可变.*/
#define PANIC() panic_spin (__FILE__, __LINE__, __func__)
 /***********************************************************************/

#define BUG() PANIC()

#endif /*_LINUX_DEBUG_H*/
