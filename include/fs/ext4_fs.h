#ifndef _EXT4_FS_H_
#define _EXT4_FS_H_

#include <xkernel/types.h>
#include <fs/ext4.h>
#include <fs/ext4_inode.h>

/*块组的引用*/
struct ext4_block_group_ref {
	struct ext4_block block; // 用于引用块组描述符表中的块
	struct ext4_bgroup *block_group; // 指向块组描述符的指针
	struct ext4_fs *fs; // 文件系统的引用
	uint32_t index; // 块组的索引
	bool dirty;//标致ext4_block_group_ref是否为脏
};

/*块组描述符表，位于超级块之后*/
struct ext4_bgroup {
    uint32_t block_bitmap_lo;           /* 低32位块位图块地址 */
    uint32_t inode_bitmap_lo;           /* 低32位inode位图块地址 */
    uint32_t inode_table_first_block_lo;/* 低32位inode表起始块地址 */
    uint16_t free_blocks_count_lo;      /* 低16位空闲块计数 */
    uint16_t free_inodes_count_lo;      /* 低16位空闲inode计数 */
    uint16_t used_dirs_count_lo;        /* 低16位已使用目录计数 */
    uint16_t flags;                     /* EXT4_BG_flags (如INODE_UNINIT等) */
    uint32_t exclude_bitmap_lo;         /* 低32位快照排除位图块地址 */
    uint16_t block_bitmap_csum_lo;      /* 低16位块位图校验和 (LE，crc32c(s_uuid+grp_num+bbitmap)) */
    uint16_t inode_bitmap_csum_lo;      /* 低16位inode位图校验和 (LE，crc32c(s_uuid+grp_num+ibitmap)) */
    uint16_t itable_unused_lo;          /* 低16位未使用的inode计数 */
    uint16_t checksum;                  /* 块组描述符校验和 (crc16(sb_uuid+group+desc)) */

    uint32_t block_bitmap_hi;           /* 高32位块位图块地址 */
    uint32_t inode_bitmap_hi;           /* 高32位inode位图块地址 */
    uint32_t inode_table_first_block_hi;/* 高32位inode表起始块地址 */
    uint16_t free_blocks_count_hi;      /* 高16位空闲块计数 */
    uint16_t free_inodes_count_hi;      /* 高16位空闲inode计数 */
    uint16_t used_dirs_count_hi;        /* 高16位已使用目录计数 */
    uint16_t itable_unused_hi;          /* 高16位未使用的inode计数 */
    uint32_t exclude_bitmap_hi;         /* 高32位快照排除位图块地址 */
    uint16_t block_bitmap_csum_hi;      /* 高16位块位图校验和 (BE，crc32c(s_uuid+grp_num+bbitmap)) */
    uint16_t inode_bitmap_csum_hi;      /* 高16位inode位图校验和 (BE，crc32c(s_uuid+grp_num+ibitmap)) */
    uint32_t reserved;                  /* 保留字段，用于填充 */
};

/**
 * @brief 将块地址转换为块组中的相对索引。
 * @param s 超级块指针
 * @param baddr 要转换的块号
 * @return 相对区块数
 */
static inline uint32_t ext4_fs_addr_to_idx_bg(struct ext4_sblock *s,
						     ext4_fsblk_t baddr)
{
	if (ext4_get32(s, first_data_block) && baddr)
		baddr--;

	return baddr % ext4_get32(s, blocks_per_group);
}

/**
 * @brief 将组中的相对块地址转换为绝对地址。
 * @param s 超级块指针
 * @param index 相对块地址
 * @param bgid 块组
 * @return 绝对块地址
 */
static inline ext4_fsblk_t ext4_fs_bg_idx_to_addr(struct ext4_sblock *s, uint32_t index, uint32_t bgid)
{
	if (ext4_get32(s, first_data_block))
		index++;

	return ext4_get32(s, blocks_per_group) * bgid + index;
}

/**
 * @brief TODO: 
*/
static inline ext4_fsblk_t ext4_fs_first_bg_block_no(struct ext4_sblock *s,
						 uint32_t bgid)
{
	return (uint64_t)bgid * ext4_get32(s, blocks_per_group) +
	       ext4_get32(s, first_data_block);
}
int ext4_fs_init_inode_dblk_idx(struct ext4_inode_ref *inode_ref, ext4_lblk_t iblock, ext4_fsblk_t *fblock);
int ext4_fs_check_features(struct ext4_fs* fs, bool* read_only);
int ext4_block_readbytes(uint64_t off, void* buf, uint32_t len);
int ext4_block_writebytes(uint64_t off, const void *buf, uint32_t len);
int ext4_fs_get_inode_dblk_idx(struct ext4_inode_ref *inode_ref,uint64_t iblock, uint64_t *fblock,bool support_unwritten);
int ext4_fs_get_block_group_ref(struct FileSystem *fs, uint32_t bgid,struct ext4_block_group_ref *ref);
static void ext4_fs_set_inode_checksum(struct ext4_inode_ref* inode_ref);
int ext4_fs_put_block_group_ref(struct ext4_block_group_ref* ref);
int ext4_fs_append_inode_dblk(struct ext4_inode_ref *inode_ref, ext4_fsblk_t *fblock, ext4_lblk_t *iblock);
#endif