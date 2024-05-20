#include "printf.h"
#include <xkernel/stdarg.h>
#include <asm/syscall.h>

void umemset(void* dst_, uint8_t value, uint32_t size) {
	uint8_t* dst = (uint8_t*)dst_;
	while (size-- > 0){
		*dst++ = value;
	}
}

char *ustrcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
	/* nothing */;
	return tmp;
}

char* ustrcat(char* dst_, const char* src_) 
{
	char* str = dst_;
	while (*str++);
	--str;
	while((*str++ = *src_++));
	return dst_;
}


uint32_t ustrlen(const char* str)
{
	const char* p = str;
	if(str == NULL) {
		while(1);
	}
	while(*p++);
	return (p - str - 1);
}

static void itoa(uint64_t val,char** buf_ptr,uint8_t base)
{
    uint64_t m = val % base;
    uint64_t q = val / base;
    if (q) {
        itoa(q,buf_ptr,base);
    }
    char c;
    if (m>=0 && m<=9) {
        c = m + '0';
    } else if (m>=10 && m<=15) {
        c = m-10 + 'a';
    }
    *((*buf_ptr)++) = c;
}


uint64_t my_vsprintf(char* buf,const char* format,va_list ap)
{
    const char* fmtp = format;
    char* bufp = buf;

    uint64_t val;
    char* arg_str;
    while(*fmtp)
    {
        if (*fmtp != '%'){
            *bufp = *(fmtp++);
            ++bufp;
            continue;
        }
        char next_char = *(++fmtp);
        switch(next_char)
        {
            case 'x':
                val = va_arg(ap,int);
                itoa(val,&bufp,16);
                ++fmtp;
                break;
            case 'd':
                val = va_arg(ap,int);
                if (val < 0) {
                    *(bufp++) = '-';
                    val = -val;
                }
                itoa(val,&bufp,10);
                ++fmtp;
                break;
            case 'c':
                *(bufp++) = va_arg(ap,char);
                ++fmtp;
                break;
            case 's':
                arg_str = va_arg(ap,char*);
                ustrcpy(bufp,arg_str);
                bufp += ustrlen(arg_str);
                ++fmtp;
                break;
        }
    }

    return ustrlen(buf);
}


int myprintf(const char *fmt, ...)
{
	char printf_buf[256];
	va_list args;
	int printed;

	va_start(args, fmt);
	printed = my_vsprintf(printf_buf,fmt,args);
	va_end(args);

	pstr(printf_buf);
	if (printed >= sizeof(printf_buf)) {
		pstr("[Message truncated]\n");
		return -1;
	}

	return printed;
}
