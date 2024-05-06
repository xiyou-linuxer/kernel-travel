#ifndef _LINUX_STRING_H
#define _LINUX_STRING_H

#include <linux/types.h>

/* 将dst_起始的size个字节置为value */
extern void memset(void* dst_, uint8_t value, uint32_t size);
/* 将src_起始的size个字节复制到dst_ */
extern void memcpy(void* dst_,  const void* src_, unsigned int size);
extern int memcmp(const void* a_, const void* b_, uint32_t size);

extern char *strcpy(char *dest, const char *src);
extern uint32_t strlen(const char *str);
extern uint32_t strnlen(const char* str, uint32_t max);
extern int strncmp(const char* p, const char* q, unsigned int n);//比较字符串p与q的内容，若不同则返回第一个不同字符的ascii码差值
char* strrchr(const char* str, const uint8_t ch);
char* strcat(char* dst_, const char* src_);
uint32_t strchrs(const char* str, uint8_t ch);
int wstrlen(const unsigned short* s);
void wstrnins(unsigned short* buf, const unsigned short* str, int len);
int wstr2str(char* dst, const unsigned short* src);
int str2wstr(unsigned short* dst, const char* src);
int strn2wstr(unsigned short* dst, const char* src, int n);
void strins(char* buf, const char* str);
#endif /* _LINUX_STRING_H */
