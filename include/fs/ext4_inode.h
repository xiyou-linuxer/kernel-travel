#ifndef EXT4_INODE_H_
#define EXT4_INODE_H_

#include <fs/fs.h>
#include <fs/buf.h>

// 定义 EXT4 文件系统中 inode 的文件类型模式
#define EXT4_INODE_MODE_FIFO 0x1000        // FIFO（命名管道）文件模式
#define EXT4_INODE_MODE_CHARDEV 0x2000     // 字符设备文件模式
#define EXT4_INODE_MODE_DIRECTORY 0x4000   // 目录文件模式
#define EXT4_INODE_MODE_BLOCKDEV 0x6000    // 块设备文件模式
#define EXT4_INODE_MODE_FILE 0x8000        // 常规文件模式
#define EXT4_INODE_MODE_SOFTLINK 0xA000    // 符号链接文件模式
#define EXT4_INODE_MODE_SOCKET 0xC000      // 套接字文件模式
#define EXT4_INODE_MODE_TYPE_MASK 0xF000   // 文件类型掩码，用于提取文件类型


/**
 * @brief 获取和设置ext4文件系统中字段值的宏定义
 */

/** 获取32位字段值，并将其转换为小端格式 */
#define ext4_get32(s, f) to_le32((s)->f)

/** 获取16位字段值，并将其转换为小端格式 */
#define ext4_get16(s, f) to_le16((s)->f)

/** 获取8位字段值 */
#define ext4_get8(s, f) (s)->f

/** 设置32位字段值，并将其转换为小端格式 */
#define ext4_set32(s, f, v)                                                    \
	do {                                                                   \
		(s)->f = to_le32(v);                                           \
	} while (0)

/** 设置16位字段值，并将其转换为小端格式 */
#define ext4_set16(s, f, v)                                                    \
	do {                                                                   \
		(s)->f = to_le16(v);                                           \
	} while (0)

/** 设置8位字段值 */
#define ext4_set8(s, f, v)                                                     \
	do {                                                                   \
		(s)->f = (v);                                                   \
	} while (0)


struct ext4_inode_ref {
	Buffer *block;    /**< 关联的块信息。 */
	struct ext4_inode *inode;   /**< 指向具体的 inode 结构。 */
	FileSystem *fs;         /**< 关联的文件系统。 */
	uint32_t index;             /**< inode 的索引。 */
	bool dirty;                 /**< 标志位，表示 inode 是否被修改。 */
};

#define EXT4_IS_DIR(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_DIRECTORY)//判断是否是目录
#define EXT4_IS_FILE(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_FILE)//判断是否是文件

uint32_t ext4_inode_get_mode(struct ext4_sblock* sb, struct ext4_inode* inode);
void ext4_inode_set_mode(struct ext4_sblock *sb, struct ext4_inode *inode, uint32_t mode);
uint32_t ext4_inode_get_uid(struct ext4_inode *inode);
void ext4_inode_set_uid(struct ext4_inode *inode, uint32_t uid);
uint64_t ext4_inode_get_size(struct ext4_sblock *sb, struct ext4_inode *inode);
int ext4_fs_get_inode_ref(FileSystem *fs, uint32_t index,struct ext4_inode_ref *ref);
int ext4_fs_put_inode_ref(struct ext4_inode_ref* ref);
void ext4_fs_inode_blocks_init(struct FileSystem *fs,struct ext4_inode_ref *inode_ref);
#endif