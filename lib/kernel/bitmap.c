#include <bitmap.h>
#include <debug.h>
#include <xkernel/string.h>
#include <xkernel/stdio.h>

void bitmap_init(struct bitmap* btmp)
{
    memset(btmp->bits,0,btmp->btmp_bytes_len);
}

int bit_scan_test(struct bitmap* btmp,uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd  = bit_idx % 8;
    return btmp->bits[byte_idx] & (BIT_MASK << bit_odd);
}

int bit_scan(struct bitmap* btmp,uint32_t bit_cnt)
{
    uint32_t bit_byte = 0;
    while (bit_byte < btmp->btmp_bytes_len && (0xff == btmp->bits[bit_byte]) ){
        bit_byte++;
    }
    ASSERT(bit_byte < btmp->btmp_bytes_len);
    if (bit_byte == btmp->btmp_bytes_len){
        return -1;
    }

    uint32_t bit_idx = bit_byte*8;
    uint32_t count = 0;
    int ret = -1;    //失败时需要返回-1
    while (bit_idx < btmp->btmp_bytes_len * 8)
    {
        if (bit_scan_test(btmp,bit_idx) == 0){
            ++count;
        } else {
            count = 0;
        }
        if (count == bit_cnt){
            ret = bit_idx-count+1;
            break;
        }
        ++bit_idx;
    }

    return ret;
}
void bitmap_set(struct bitmap* btmp,uint32_t bit_idx,uint8_t value)
{
    ASSERT(value==0 || value==1);
    uint32_t bit_byte = bit_idx / 8;
    uint32_t bit_odd  = bit_idx % 8;
    if (value) {
        btmp->bits[bit_byte] |= (BIT_MASK << bit_odd);
    } else {
        btmp->bits[bit_byte] &= ~(BIT_MASK << bit_odd);
    }
}
