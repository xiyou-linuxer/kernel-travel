#ifndef EXT4_SUPER_H_
#define EXT4_SUPER_H_

#include <xkernel/types.h>
#include <fs/ext4.h>

/**
 * @brief   块计数存储在超级块中
 * @param   s 超级块描述符
 * @return  块数
 **/
static inline uint64_t ext4_sb_get_blocks_cnt(struct ext4_sblock *s)
{
	return ((uint64_t)to_le32(s->blocks_count_hi) << 32) |
	       to_le32(s->blocks_count_lo);
}

/**
 * @brief   超级块中设置的块计数
 * @param   s 超级块描述符
 * @param   cnt 块数
 * */
static inline void ext4_sb_set_blocks_cnt(struct ext4_sblock *s, uint64_t cnt)
{
	s->blocks_count_lo = to_le32((cnt << 32) >> 32);
	s->blocks_count_hi = to_le32(cnt >> 32);
}

/**
 * @brief   空闲块计数存储在超级块中
 * @param   s 超级块描述符
 * @return  空闲块数
 **/
static inline uint64_t ext4_sb_get_free_blocks_cnt(struct ext4_sblock *s)
{
	return ((uint64_t)to_le32(s->free_blocks_count_hi) << 32) |
	       to_le32(s->free_blocks_count_lo);
}

/**
 * @brief   Free blocks count set.
 * @param   s 超级块描述符
 * @param   cnt 空闲块的新值
 **/
static inline void ext4_sb_set_free_blocks_cnt(struct ext4_sblock *s,
					       uint64_t cnt)
{
	s->free_blocks_count_lo = to_le32((cnt << 32) >> 32);
	s->free_blocks_count_hi = to_le32(cnt >> 32);
}

/**
 * @brief 块大小从超级块获取
 * @param   s 超级块描述符
 * @return  block size in bytes
 **/
static inline uint32_t ext4_sb_get_block_size(struct ext4_sblock *s)
{
	return 1024 << to_le32(s->log_block_size);
}

/**
 * @brief 块组描述符大小。
 * @param s 超级块描述符
 * @return 块组描述符大小（以字节为单位）
 **/
static inline uint16_t ext4_sb_get_desc_size(struct ext4_sblock *s)
{
	uint16_t size = to_le16(s->desc_size);

	return size < EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE
		   ? EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE
		   : size;
}

/**
 * @brief 支持标志检查。
 * @param s 超级块描述符
 * @param v 要检查的标志
 * @return true 如果支持flag
 **/
static inline bool ext4_sb_check_flag(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->flags) & v;
}

/**
 * @brief 支持功能兼容检查。
 * @param s 超级块描述符
 * @param v 要检查的功能
 * @return true 如果支持该功能
 **/
static inline bool ext4_sb_feature_com(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_compatible) & v;
}

/**
 * @brief 支持功能不兼容检查。
 * @param s 超级块描述符
 * @param v 要检查的功能
 * @return true 如果支持该功能
 **/
static inline bool ext4_sb_feature_incom(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_incompatible) & v;
}

/**
 * @brief 支持只读标志检查。
 * @param s 超级块描述符
 * @param v 要检查的标志
 * @return true 如果支持flag
 **/
static inline bool ext4_sb_feature_ro_com(struct ext4_sblock *s, uint32_t v)
{
	return to_le32(s->features_read_only) & v;
}

/**
 * @brief 块组到弹性组。
 * @param s 超级块描述符
 * @param block_group 块组
 * @return 弹性组 ID
 **/
static inline uint32_t ext4_sb_bg_to_flex(struct ext4_sblock *s,
					  uint32_t block_group)
{
	return block_group >> to_le32(s->log_groups_per_flex);
}

/**
 * @brief Flex 块组大小。
 * @param s 超级块描述符
 * @return flex 背景大小
 **/
static inline uint32_t ext4_sb_flex_bg_size(struct ext4_sblock *s)
{
	return 1 << to_le32(s->log_groups_per_flex);
}

/**
 * @brief 返回第一个元块组 ID。
 * @param s 超级块描述符
 * @return 第一个meta_bg id
 **/
static inline uint32_t ext4_sb_first_meta_bg(struct ext4_sblock *s)
{
	return to_le32(s->first_meta_bg);
}

uint32_t ext4_block_group_cnt(struct ext4_sblock* s);
uint32_t ext4_blocks_in_group_cnt(struct ext4_sblock* s, uint32_t bgid);
uint32_t ext4_inodes_in_group_cnt(struct ext4_sblock* s, uint32_t bgid);
bool ext4_sb_check(struct ext4_sblock* s);
bool ext4_sb_sparse(uint32_t group);
bool ext4_sb_is_super_in_bg(struct ext4_sblock* s, uint32_t group);
#endif