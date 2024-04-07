#include <cacheflush.h>

/* Cache operations. */
void local_flush_icache_range(unsigned long start, unsigned long end)
{
	asm volatile ("\tibar 0\n"::);
}
