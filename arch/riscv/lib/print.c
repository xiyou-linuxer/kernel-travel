#include <asm/stdio.h>
#include <xkernel/types.h>
void print_str(char *str){
    while(*str)
        {
            
            char ch   = *str;
            register uintptr_t a0 asm("x10") = (uintptr_t)ch;
            register uintptr_t a1 asm("x11") = 0;
            register uintptr_t a2 asm("x12") = 0;
            register uintptr_t a7 asm("x17") =  1;
             // which = 1 (SBI_CONSOLE_PUTCHAR)
            register uintptr_t ret asm("x10");

            asm volatile("ecall"
                         : "=r"(ret)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a7)
                         : "memory");
            (void)ret; // 防止未使用变量警告
            str++;
        }
}