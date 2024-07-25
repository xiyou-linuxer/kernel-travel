#ifndef EXT4_BITMAP_H_
#define EXT4_BITMAP_H_

#include <xkernel/types.h>

/**
 * @brief 设置位图位。
 * @param bmap 位图
 * @param bit 要设置的位
 **/
static inline void ext4_bmap_bit_set(uint8_t *bmap, uint32_t bit)
{
	*(bmap + (bit >> 3)) |= (1 << (bit & 7));
}

/**
 * @brief 清除位图位。
 * @param bmap 位图缓冲区
 * @param bit 要清除的位
 **/
static inline void ext4_bmap_bit_clr(uint8_t *bmap, uint32_t bit)
{
	*(bmap + (bit >> 3)) &= ~(1 << (bit & 7));
}

/**
 * @brief 检查位图位是否已设置。
 * @param bmap 位图缓冲区
 * @param bit 要检查的位
 **/
static inline bool ext4_bmap_is_bit_set(uint8_t *bmap, uint32_t bit)
{
	return (*(bmap + (bit >> 3)) & (1 << (bit & 7)));
}

/**
 * @brief 检查位图位是否已清除。
 * @param bmap 位图缓冲区
 * @param bit 要检查的位
 **/
static inline bool ext4_bmap_is_bit_clr(uint8_t *bmap, uint32_t bit)
{
	return !ext4_bmap_is_bit_set(bmap, bit);
}

/**
 * @brief 位图中的可用位范围。
 * @param bmap 位图缓冲区
 * @param sbit 起始位
 * @param bcnt 位数
 **/
void ext4_bmap_bits_free(uint8_t *bmap, uint32_t sbit, uint32_t bcnt);

/**
 * @brief 查找位图中的第一个清除位。
 * @param sbit 搜索的起始位
 * @param ebit 搜索结束位
 * @param bit_id 输出参数（第一个空闲位）
 * @return 标准错误码
 **/
int ext4_bmap_bit_find_clr(uint8_t *bmap, uint32_t sbit, uint32_t ebit,
			   uint32_t *bit_id);

#endif /* EXT4_BITMAP_H_ */
