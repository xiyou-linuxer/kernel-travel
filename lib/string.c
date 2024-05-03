#include <linux/string.h>
#include <linux/printk.h>
#include <debug.h>
void memset(void* dst_, uint8_t value, uint32_t size) {
	uint8_t* dst = (uint8_t*)dst_;
	if(dst_ == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	while (size-- > 0)
		*dst++ = value;
}

void memcpy(void* dst_, const void* src_, uint32_t size)
{
	uint8_t* dst = dst_;
	const uint8_t* src = src_;
	if(dst_ == NULL || src_ == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	while (size-- > 0)
	*dst++ = *src++;
}

int memcmp(const void* a_, const void* b_, uint32_t size)
{
	const char* a = a_;
	const char* b = b_;
	if(a == NULL || b == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	while (size-- > 0) {
		if(*a != *b) {
			return *a > *b ? 1 : -1; 
		}
		a++;
		b++;
	}
	return 0;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
	/* nothing */;
	return tmp;
}

uint32_t strlen(const char* str)
{
	const char* p = str;
	if(str == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	while(*p++);
	return (p - str - 1);
}

uint32_t strnlen(const char* str, uint32_t max)
{
	uint32_t len = strlen(str);
	if(str == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	return len >= max ? max : len;
}

int strncmp(const char *p, const char *q, unsigned int n) 
{
	while (n > 0 && *p && *p == *q) {
		n--, p++, q++;
	}
	if (n == 0 || (*p == 0 && *q == 0)) {
		return 0;
	}
	return (unsigned char)*p - (unsigned char)*q;
}

/* 从后往前查找字符串str中首次出现字符ch的地址(不是下标,是地址) */
char* strrchr(const char* str, const uint8_t ch) 
{
	ASSERT(str != NULL);
	const char* last_char = NULL;
	/* 从头到尾遍历一次,若存在ch字符,last_char总是该字符最后一次出现在串中的地址(不是下标,是地址)*/
	while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

/* 将字符串src_拼接到dst_后,将回拼接的串地址 */
char* strcat(char* dst_, const char* src_) 
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* str = dst_;
	while (*str++);
	--str;
	while((*str++ = *src_++));	//1、*str=*src  2、判断*str     3、str++与src++，这一步不依赖2
	return dst_;
}

/* 在字符串str中查找指定字符ch出现的次数 */
uint32_t strchrs(const char* str, uint8_t ch) 
{
	ASSERT(str != NULL);
	uint32_t ch_cnt = 0;
	const char* p = str;
	while(*p != 0) {
		if (*p == ch) {
			ch_cnt++;
		}
		p++;
	}
	return ch_cnt;
}
