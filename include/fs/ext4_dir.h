
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
 * @brief 从目录项获取 i 节点号
 * @param de 目录条目
 * @return inode 号
 */
static inline uint32_t
ext4_dir_en_get_inode(struct ext4_dir_en *de)
{
	return to_le32(de->inode);
}

/**
 * @brief 将 i 节点号设置为目录项
 * @param de 目录条目
 * @param inode inode 号
 */
static inline void
ext4_dir_en_set_inode(struct ext4_dir_en *de, uint32_t inode)
{
	de->inode = to_le32(inode);
}

/**
 * @brief 获取目录条目长度
 * @param de 目录条目
 * @return 入口长度
 */
static inline uint16_t ext4_dir_en_get_entry_len(struct ext4_dir_en *de)
{
	return to_le16(de->entry_len);
}

/**
 * @brief 设置目录条目长度。
 * @param de 目录条目
 * @param l 条目长度
 */
static inline void ext4_dir_en_set_entry_len(struct ext4_dir_en *de, uint16_t l)
{
	de->entry_len = to_le16(l);
}

/**
 * @brief 获取目录条目名称长度。
 * @param sb 超级块
 * @param de 目录条目
 * @return 条目名称长度
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

/**
 * @brief 设置目录条目名称长度。
 * @param sb 超级块
 * @param de 目录条目
 * @param len 条目名称长度
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

/**
 * @brief 获取目录项的 i 节点类型。
 * @param sb 超级块
 * @param de 目录条目
 * @return I节点类型（文件、目录等）
 */
static inline uint8_t ext4_dir_en_get_inode_type(struct ext4_sblock *sb,struct ext4_dir_en *de)
{
	if ((ext4_get32(sb, rev_level) > 0) ||
	    (ext4_get32(sb, minor_rev_level) >= 5))
		return de->in.inode_type;

	return EXT4_DE_UNKNOWN;
}
/**
 * @brief 设置目录项的 i 节点类型。
 * @param sb 超级块
 * @param de 目录条目
 * @param t I节点类型（文件、目录等）
 */
static inline void ext4_dir_en_set_inode_type(struct ext4_sblock *sb,
					      struct ext4_dir_en *de, uint8_t t)
{
	if ((ext4_get32(sb, rev_level) > 0) ||
	    (ext4_get32(sb, minor_rev_level) >= 5))
		de->in.inode_type = t;
}


/* EXT3 HTree directory indexing */

/* HTree（哈希树）目录索引类型定义 */
#define EXT2_HTREE_LEGACY 0                 // 传统HTree索引
#define EXT2_HTREE_HALF_MD4 1               // 使用MD4哈希算法的一半（前16位）
#define EXT2_HTREE_TEA 2                    // 使用TEA哈希算法
#define EXT2_HTREE_LEGACY_UNSIGNED 3        // 无符号传统HTree索引
#define EXT2_HTREE_HALF_MD4_UNSIGNED 4      // 无符号MD4哈希算法的一半
#define EXT2_HTREE_TEA_UNSIGNED 5           // 无符号TEA哈希算法

/* 表示HTree索引的结尾 */
#define EXT2_HTREE_EOF 0x7FFFFFFFUL         // HTree索引的结束标记

/* 定义旧版本的inode结构体大小 */
#define EXT4_GOOD_OLD_INODE_SIZE 128        // 旧版本的inode大小（字节)

const Dirent* ext4_dir_entry_next(struct ext4_dir* dir);
#endif