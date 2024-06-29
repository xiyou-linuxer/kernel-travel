#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include <xkernel/types.h>

#define BIT_MASK_TEMP 1

struct bitmap {
    uint64_t btmp_bytes_len;
    uint8_t* bits;
};

void bitmap_init(struct bitmap* btmp);
int bit_scan_test(struct bitmap* btmp,uint32_t bit_idx);
int bit_scan(struct bitmap* btmp,uint32_t cnt);
void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,uint8_t value);

#endif
