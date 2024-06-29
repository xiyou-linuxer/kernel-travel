
#ifndef EXT4_DIR_H_
#define EXT4_DIR_H_
#include <fs/fs.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_inode.h>
/*目录迭代器*/
struct ext4_dir_iter {
	struct Dirent *pdirent; /* 迭代器对应的目录项 */
	struct ext4_inode_ref *inode_ref; /**< 指向目录 i-node 的引用 */
	struct ext4_block curr_blk;       /*< 当前数据块 */
	uint64_t curr_off;                /* 当前偏移量 */
	struct Dirent  *curr;         /* 当前目录项指针 */
};

/**
 * @brief Get i-node number from directory entry.
 * @param de Directory entry
 * @return I-node number
 */
static inline uint32_t
ext4_dir_en_get_inode(struct ext4_dir_en *de)
{
	return to_le32(de->inode);
}

/**
 * @brief Set i-node number to directory entry.
 * @param de Directory entry
 * @param inode I-node number
 */
static inline void
ext4_dir_en_set_inode(struct ext4_dir_en *de, uint32_t inode)
{
	de->inode = to_le32(inode);
}

/**
 * @brief Get directory entry length.
 * @param de Directory entry
 * @return Entry length
 */
static inline uint16_t ext4_dir_en_get_entry_len(struct ext4_dir_en *de)
{
	return to_le16(de->entry_len);
}

/**
 * @brief Set directory entry length.
 * @param de     Directory entry
 * @param l Entry length
 */
static inline void ext4_dir_en_set_entry_len(struct ext4_dir_en *de, uint16_t l)
{
	de->entry_len = to_le16(l);
}

/**@brief Get directory entry name length.
 * @param sb Superblock
 * @param de Directory entry
 * @return Entry name length
 */
static inline uint16_t ext4_dir_en_get_name_len(struct ext4_sblock *sb,
						struct ext4_dir_en *de)
{
	uint16_t v = de->name_len;

	if ((ext4_get32(sb, rev_level) == 0) &&
	    (ext4_get32(sb, minor_rev_level) < 5))
		v |= ((uint16_t)de->in.name_length_high) << 8;

	return v;
}

/**@brief Set directory entry name length.
 * @param sb     Superblock
 * @param de     Directory entry
 * @param len Entry name length
 */
static inline void ext4_dir_en_set_name_len(struct ext4_sblock *sb,
					    struct ext4_dir_en *de,
					    uint16_t len)
{
	de->name_len = (len << 8) >> 8;

	if ((ext4_get32(sb, rev_level) == 0) &&
	    (ext4_get32(sb, minor_rev_level) < 5))
		de->in.name_length_high = len >> 8;
}

/**@brief Get i-node type of directory entry.
 * @param sb Superblock
 * @param de Directory entry
 * @return I-node type (file, dir, etc.)
 */
static inline uint8_t ext4_dir_en_get_inode_type(struct ext4_sblock *sb,
						 struct ext4_dir_en *de)
{
	if ((ext4_get32(sb, rev_level) > 0) ||
	    (ext4_get32(sb, minor_rev_level) >= 5))
		return de->in.inode_type;

	return EXT4_DE_UNKNOWN;
}
/**@brief Set i-node type of directory entry.
 * @param sb   Superblock
 * @param de   Directory entry
 * @param t I-node type (file, dir, etc.)
 */

static inline void ext4_dir_en_set_inode_type(struct ext4_sblock *sb,
					      struct ext4_dir_en *de, uint8_t t)
{
	if ((ext4_get32(sb, rev_level) > 0) ||
	    (ext4_get32(sb, minor_rev_level) >= 5))
		de->in.inode_type = t;
}

#endif