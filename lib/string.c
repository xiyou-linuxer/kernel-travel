#include <linux/string.h>
#include <linux/printk.h>
#include <linux/stdio.h>
#include <debug.h>
void memset(void* dst_, uint8_t value, uint32_t size) {
	
	uint8_t* dst = (uint8_t*)dst_;
	if(dst_ == NULL) {
		efi_puts("BUG!!!");
		while(1);
	}
	//printk("memset");
	while (size-- > 0){
		*dst++ = value;
	}
}

void memcpy(void* dst_, const void* src_, uint32_t size)
{
	uint8_t* dst = dst_;
	const uint8_t* src = src_;
	if(/*dst_ == NULL ||*/ src_ == NULL) {
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

/* 返回字符串长度 */
uint32_t strlen(const char* str) {
    ASSERT(str != NULL);
    const char* p = str;
    while(*p++);                 //1、先取*p的值来进行2的判断     2、判断*p,决定是否执行循环体     3、p++(这一步的执行并不依赖2的判断为真) 
    return (p - str - 1);        //p最后指向'\0'后面第一个元素
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

char strcmp (const char* a, const char* b) {
    ASSERT(a != NULL && b != NULL);
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
/* 如果*a小于*b就返回-1,否则就属于*a大于等于*b的情况。在后面的布尔表达式"*a > *b"中,
 * 若*a大于*b,表达式就等于1,否则就表达式不成立,也就是布尔值为0,恰恰表示*a等于*b */
    return *a < *b ? -1 : *a > *b;
}

/*从前往后查找首次出现字符的首地址*/
char* strchr(const char* str, const uint8_t ch) {
    ASSERT(str != NULL);
    while (*str != 0) {
        if (*str == ch) {
	        return (char*)str;	    // 需要强制转化成和返回值类型一样,否则编译器会报const属性丢失,下同.
        }
        str++;
    }
    return NULL;
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

int wstrlen(const unsigned short *s) 
{
	int n;

	for (n = 0; s[n]; n++) {
		;
	}
	return n;
}

/**
 * @brief 在wchar字符串buf前面插入字符串s。保证buf数组有足够的空间
 */
void wstrnins(unsigned short *buf, const unsigned short *str, int len)
{
	int lbuf = wstrlen(buf);
	int i;
	for (i = lbuf; i >= 0; i--) {
		buf[i + len] = buf[i];
	}
	for (i = 0; i < len; i++) {
		buf[i] = str[i];
	}
}

/**
 * @brief 将wide char字符串转化为char字符串
 * @return 返回写入字符串的长度
 */
int wstr2str(char *dst, const unsigned short *src) 
{
	int i;
	for (i = 0; src[i]; i++) {
		dst[i] = (char)(src[i] & 0xff);
	}
	dst[i] = 0;
	return i;
}

int str2wstr(unsigned short *dst, const char *src) 
{
	int i;
	for (i = 0; src[i]; i++) {
		dst[i] = src[i];
	}
	dst[i] = 0;
	return i;
}

int strn2wstr(unsigned short *dst, const char *src, int n)
{
	int i;
	for (i = 0; src[i] && i < n; i++) {
		dst[i] = src[i];
	}
	if (i < n) {
		dst[i] = 0;
	}
	return i;
}
void strins(char *buf, const char *str) 
{
	int lbuf = strlen(buf);
	int i;
	int len = strlen(str);
	for (i = lbuf; i >= 0; i--) {
		buf[i + len] = buf[i];
	}
	for (i = 0; i < len; i++) {
		buf[i] = str[i];
	}
}

