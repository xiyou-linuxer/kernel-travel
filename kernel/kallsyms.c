
#include <linux/kallsyms.h>
// #include <dim-sum/fs.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <asm/sections.h>

#ifdef CONFIG_KALLSYMS_ALL
#define all_var 1
#else
#define all_var 0
#endif

/*
 * These will be re-linked against their real values
 * during the second link stage.
 */
extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));

/*
 * Tell the compiler that the count isn't in the small data section if the arch
 * has one (eg: FRV).
 */
extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));

extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));

extern const unsigned long kallsyms_markers[] __attribute__((weak));

/* 是否是内核初始化代码段地址 */
static inline int is_kernel_inittext(unsigned long addr)
{
	if (addr >= (unsigned long)_sinittext
	    && addr <= (unsigned long)_einittext)
		return 1;
	return 0;
}

/* 是否是内核代码段地址 */
static inline int is_kernel_text(unsigned long addr)
{
	if ((addr >= (unsigned long)kernel_text_start && addr <= (unsigned long)kernel_text_end) ||
	    arch_is_kernel_text(addr))
		return 1;

	return 0;
}

/* 是否是内核地址 */
static inline int is_kernel(unsigned long addr)
{
	if (addr >= (unsigned long)kernel_text_start && addr <= (unsigned long)kernel_text_end)
		return 1;
	
	return 0;
}

/* 是否是内核符号地址 */
static int is_ksym_addr(unsigned long addr)
{
	if (all_var)
		return is_kernel(addr);

	return is_kernel_text(addr) || is_kernel_inittext(addr);
}

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * if uncompressed string is too long (>= maxlen), it will be truncated,
 * given the offset to where the symbol is in the compressed stream.
 */
static unsigned int kallsyms_expand_symbol(unsigned int off,
					   char *result, size_t maxlen)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

/*
 * Find the offset on the compressed stream given and index in the
 * kallsyms array.
 */
static unsigned int get_symbol_offset(unsigned long pos)
{
	const u8 *name;
	int i;

	/*
	 * Use the closest marker we have. We have markers every 256 positions,
	 * so that should be close enough.
	 */
	name = &kallsyms_names[kallsyms_markers[pos >> 8]];

	/*
	 * Sequentially scan all the symbols up to the point we're searching
	 * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
	 * so we just need to add the len to the current pointer for every
	 * symbol we wish to skip.
	 */
	for (i = 0; i < (pos & 0xFF); i++)
		name = name + (*name) + 1;

	return name - kallsyms_names;
}

/* 根据符号名称查找符号地址 */
unsigned long kallsyms_lookup_name(const char *name)
{
	char namebuf[KSYM_NAME_LEN];
	unsigned long i;
	unsigned int off;

	for (i = 0, off = 0; i < kallsyms_num_syms; i++) {
		off = kallsyms_expand_symbol(off, namebuf, ARRAY_SIZE(namebuf));

		if (strcmp(namebuf, name) == 0)
			return kallsyms_addresses[i];
	}

	return 0;
}

static unsigned long get_symbol_pos(unsigned long addr,
											unsigned long *symbolsize,
											unsigned long *offset)
{
	unsigned long symbol_start = 0, symbol_end = 0;
	unsigned long i, low, high, mid;

	/* This kernel should never had been booted. */
	BUG_ON(!kallsyms_addresses);

	/* Do a binary search on the sorted kallsyms_addresses array. */
	low = 0;
	high = kallsyms_num_syms;

	while (high - low > 1) {
		mid = low + (high - low) / 2;
		if (kallsyms_addresses[mid] <= addr)
			low = mid;
		else
			high = mid;
	}

	/*
	 * Search for the first aliased symbol. Aliased
	 * symbols are symbols with the same address.
	 */
	while (low && kallsyms_addresses[low-1] == kallsyms_addresses[low])
		--low;

	symbol_start = kallsyms_addresses[low];

	/* Search for next non-aliased symbol. */
	for (i = low + 1; i < kallsyms_num_syms; i++) {
		if (kallsyms_addresses[i] > symbol_start) {
			symbol_end = kallsyms_addresses[i];
			break;
		}
	}

	/* If we found no next symbol, we use the end of the section. */
	if (!symbol_end) {
		if (is_kernel_inittext(addr))
			symbol_end = (unsigned long)_einittext;
		else if (all_var)
			symbol_end = (unsigned long)kernel_text_end;
		else
			symbol_end = (unsigned long)_etext;
	}

	if (symbolsize)
		*symbolsize = symbol_end - symbol_start;
	if (offset)
		*offset = addr - symbol_start;

	return low;
}

/* 根据地址查找地址对应符号的大小，以及地址在函数中的偏移 */
int kallsyms_lookup_size_offset(unsigned long addr, unsigned long *symbolsize,
										unsigned long *offset)
{
	if (is_ksym_addr(addr))
		return !!get_symbol_pos(addr, symbolsize, offset);

	return -1;
}

/* 根据地址查找对应的符号信息 */
const char *kallsyms_lookup(unsigned long addr,
			    unsigned long *symbolsize,
			    unsigned long *offset,
			    char **modname, char *namebuf)
{
	namebuf[KSYM_NAME_LEN - 1] = 0;
	namebuf[0] = 0;

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, symbolsize, offset);
		/* Grab name */
		kallsyms_expand_symbol(get_symbol_offset(pos),
				       namebuf, KSYM_NAME_LEN);
		if (modname)
			*modname = NULL;
		return namebuf;
	}

	return NULL;
}

/* 根据地址查找对应的符号名 */
int lookup_symbol_name(unsigned long addr, char *symname)
{
	symname[0] = '\0';
	symname[KSYM_NAME_LEN - 1] = '\0';

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, NULL, NULL);
		/* Grab name */
		kallsyms_expand_symbol(get_symbol_offset(pos),
				       symname, KSYM_NAME_LEN);
		return 0;
	}

	return -ERANGE;
}

/* 根据地址查找对应符号属性 */
int lookup_symbol_attrs(unsigned long addr, unsigned long *size,
								unsigned long *offset, char *modname, char *name)
{
	name[0] = '\0';
	name[KSYM_NAME_LEN - 1] = '\0';

	if (is_ksym_addr(addr)) {
		unsigned long pos;

		pos = get_symbol_pos(addr, size, offset);
		/* Grab name */
		kallsyms_expand_symbol(get_symbol_offset(pos),
				       name, KSYM_NAME_LEN);
		modname[0] = '\0';
		return 0;
	}

	return -ERANGE;
}

/* 
 * 打印符号信息
 * buffer			-- 符号名称字符串
 * address			-- 符号地址
 * symbolf_offset	-- 符号偏移
 * add_offset		-- 是否输出偏移
 */
static int __sprint_symbol(char *buffer, unsigned long address,
			   int symbol_offset, int add_offset)
{
	char *modname;
	const char *name;
	unsigned long offset, size;
	int len;

	address += symbol_offset;
	name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
	if (!name)
		return sprintf(buffer, "0x%lx", address);

	if (name != buffer)
		strcpy(buffer, name);
	len = strlen(buffer);
	offset -= symbol_offset;

	if (add_offset)
		len += sprintf(buffer + len, "+%#lx/%#lx", offset, size);

	if (modname)
		len += sprintf(buffer + len, " [%s]", modname);

	return len;
}

/* 输出符号信息带偏移地址 */
int sprint_symbol(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, 0, 1);
}

/* 输出符号信息不带偏移地址 */
int sprint_symbol_no_offset(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, 0, 0);
}

/* 输出符号栈桢带偏移地址 */
int sprint_backtrace(char *buffer, unsigned long address)
{
	return __sprint_symbol(buffer, address, -1, 1);
}

/* Look up a kernel symbol and print it to the kernel messages. */
void __print_symbol(const char *fmt, unsigned long address)
{
	char buffer[KSYM_SYMBOL_LEN];

	sprint_symbol(buffer, address);

	printk(fmt, buffer);
}
