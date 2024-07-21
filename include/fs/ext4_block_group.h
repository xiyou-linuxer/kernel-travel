#ifndef EXT4_BLOCK_GROUP_H_
#define EXT4_BLOCK_GROUP_H_

#include <fs/ext4_sb.h>
#include <fs/ext4.h>
#include <fs/ext4_fs.h>
#include <xkernel/types.h>
/**
 * @brief 获取带有数据块位图的块地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 带有块位图的块地址
 */
static inline uint64_t ext4_bg_get_block_bitmap(struct ext4_bgroup *bg,
						struct ext4_sblock *s)
{
	uint64_t v = to_le32(bg->block_bitmap_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->block_bitmap_hi) << 32;

	return v;
}

/**
 * @brief 设置带有数据块位图的块地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param blk 块来设置
 */
static inline void ext4_bg_set_block_bitmap(struct ext4_bgroup *bg,
					    struct ext4_sblock *s, uint64_t blk)
{

	bg->block_bitmap_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->block_bitmap_hi = to_le32(blk >> 32);

}

/**
 * @brief 获取带有 i 节点位图的块地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 带有 i 节点位图的块地址
 */
static inline uint64_t ext4_bg_get_inode_bitmap(struct ext4_bgroup *bg,
						struct ext4_sblock *s)
{

	uint64_t v = to_le32(bg->inode_bitmap_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->inode_bitmap_hi) << 32;

	return v;
}

/**
 * @brief 设置带有 i 节点位图的块地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param blk 块来设置
 */
static inline void ext4_bg_set_inode_bitmap(struct ext4_bgroup *bg,
					    struct ext4_sblock *s, uint64_t blk)
{
	bg->inode_bitmap_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_bitmap_hi = to_le32(blk >> 32);

}


/**
 * @brief 获取 i 节点表的第一个块的地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return i节点表第一个块的地址
 */
static inline uint64_t
ext4_bg_get_inode_table_first_block(struct ext4_bgroup *bg,
				    struct ext4_sblock *s)
{
	uint64_t v = to_le32(bg->inode_table_first_block_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint64_t)to_le32(bg->inode_table_first_block_hi) << 32;

	return v;
}

/**
 * @brief 设置 i 节点表的第一个块的地址。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param blk 块来设置
 */
static inline void
ext4_bg_set_inode_table_first_block(struct ext4_bgroup *bg,
				    struct ext4_sblock *s, uint64_t blk)
{
	bg->inode_table_first_block_lo = to_le32((uint32_t)blk);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->inode_table_first_block_hi = to_le32(blk >> 32);
}

/**
 * @brief 获取块组中空闲块的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 块组中空闲块的数量
 */
static inline uint32_t ext4_bg_get_free_blocks_count(struct ext4_bgroup *bg,
						     struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->free_blocks_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->free_blocks_count_hi) << 16;

	return v;
}

/**
 * @brief 设置块组中空闲块的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param cnt 块组中空闲块的数量
 */
static inline void ext4_bg_set_free_blocks_count(struct ext4_bgroup *bg,
						 struct ext4_sblock *s,
						 uint32_t cnt)
{
	bg->free_blocks_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_blocks_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief 获取块组中空闲 i 节点的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 块组中空闲i节点的数量
 */
static inline uint32_t ext4_bg_get_free_inodes_count(struct ext4_bgroup *bg,
						     struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->free_inodes_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->free_inodes_count_hi) << 16;

	return v;
}

/**
 * @brief 设置块组中空闲 i 节点的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param cnt 块组中空闲i节点的数量
 */
static inline void ext4_bg_set_free_inodes_count(struct ext4_bgroup *bg,
						 struct ext4_sblock *s,
						 uint32_t cnt)
{
	bg->free_inodes_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->free_inodes_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief 获取块组中已使用目录的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 块组中已使用的目录数
 */
static inline uint32_t ext4_bg_get_used_dirs_count(struct ext4_bgroup *bg,
						   struct ext4_sblock *s)
{
	uint32_t v = to_le16(bg->used_dirs_count_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->used_dirs_count_hi) << 16;

	return v;
}

/**
 * @brief 设置块组中使用的目录数。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param cnt 块组中已使用的目录数
 */
static inline void ext4_bg_set_used_dirs_count(struct ext4_bgroup *bg,
					       struct ext4_sblock *s,
					       uint32_t cnt)
{
	bg->used_dirs_count_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->used_dirs_count_hi = to_le16(cnt >> 16);
}

/**
 * @brief 获取未使用的 i 节点的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @return 未使用的 i 节点数
 */
static inline uint32_t ext4_bg_get_itable_unused(struct ext4_bgroup *bg,
						 struct ext4_sblock *s)
{

	uint32_t v = to_le16(bg->itable_unused_lo);

	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		v |= (uint32_t)to_le16(bg->itable_unused_hi) << 16;

	return v;
}

/**
 * @brief 设置未使用的 i 节点的数量。
 * @param bg 指向块组的指针
 * @param s 指向超级块的指针
 * @param cnt 未使用的 i 节点数
 */
static inline void ext4_bg_set_itable_unused(struct ext4_bgroup *bg,
					     struct ext4_sblock *s,
					     uint32_t cnt)
{
	bg->itable_unused_lo = to_le16((cnt << 16) >> 16);
	if (ext4_sb_get_desc_size(s) > EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE)
		bg->itable_unused_hi = to_le16(cnt >> 16);
}

/**
 * @brief 设置块组的校验和。
 * @param bg 指向块组的指针
 * @param crc 块组校验和
 */
static inline void ext4_bg_set_checksum(struct ext4_bgroup *bg, uint16_t crc)
{
	bg->checksum = to_le16(crc);
}

/**
 * @brief 检查块组是否有标志。
 * @param bg 指向块组的指针
 * @param f 要检查的标志
 * @return True 如果标志设置为 1
 */
static inline bool ext4_bg_has_flag(struct ext4_bgroup *bg, uint32_t f)
{
	return to_le16(bg->flags) & f;
}

/**
 * @brief 设置块组标志。
 * @param bg 指向块组的指针
 * @param f 要设置的标志
 */
static inline void ext4_bg_set_flag(struct ext4_bgroup *bg, uint32_t f)
{
	uint16_t flags = to_le16(bg->flags);
	flags |= f;
	bg->flags = to_le16(flags);
}

/**
 * @brief 清除块组标志。
 * @param bg 指向块组的指针
 * @param f 要清除的标志
 */
static inline void ext4_bg_clear_flag(struct ext4_bgroup *bg, uint32_t f)
{
	uint16_t flags = to_le16(bg->flags);
	flags &= ~f;
	bg->flags = to_le16(flags);
}

#define EXT4_BLOCK_ZERO() 	\
	{.lb_id = 0, .buf = 0}

/**
 * @brief 计算块组的CRC16。
 * @param crc 初始值
 * @param buffer 输入缓冲区
 * @param len 输入缓冲区的大小
 * @return 计算出的CRC16
 **/
uint16_t ext4_bg_crc16(uint16_t crc, const uint8_t *buffer, size_t len);
int ext4_blocks_get_direct(const void *buf,int block_size, uint64_t lba, uint32_t cnt);
int ext4_blocks_set_direct(const void *buf,int block_size,uint64_t lba, uint32_t cnt);
int ext4_balloc_alloc_block(struct ext4_inode_ref *inode_ref, ext4_fsblk_t goal, ext4_fsblk_t *fblock);
int ext4_balloc_free_blocks(struct ext4_inode_ref *inode_ref, ext4_fsblk_t first, uint32_t count);
uint64_t ext4_balloc_get_block_of_bgid(struct ext4_sblock* s, uint32_t bgid);
void ext4_balloc_set_bitmap_csum(struct ext4_sblock *sb, struct ext4_bgroup *bg, void *bitmap __attribute__ ((__unused__)));
uint32_t ext4_balloc_get_bgid_of_block(struct ext4_sblock* s, uint64_t baddr);
int ext4_trans_set_block_dirty(struct Buffer* buf);
#endif /* EXT4_BLOCK_GROUP_H_ */
