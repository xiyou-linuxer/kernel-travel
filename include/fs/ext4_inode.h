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

// 定义EXT4文件系统的特殊inode编号
#define EXT4_BAD_INO 1         // 用于标识坏块的inode编号
#define EXT4_ROOT_INO 2        // 根目录的inode编号
#define EXT4_BOOT_LOADER_INO 5 // 引导加载程序的inode编号
#define EXT4_UNDEL_DIR_INO 6   // 用于未删除目录的inode编号
#define EXT4_RESIZE_INO 7      // 用于在线调整文件系统大小的inode编号
#define EXT4_JOURNAL_INO 8     // 日志文件的inode编号

#define EXT4_INODE_FLAG_SECRM 0x00000001     /* 安全删除 */
#define EXT4_INODE_FLAG_UNRM 0x00000002      /* 恢复删除 */
#define EXT4_INODE_FLAG_COMPR 0x00000004     /* 文件压缩 */
#define EXT4_INODE_FLAG_SYNC 0x00000008      /* 同步更新 */
#define EXT4_INODE_FLAG_IMMUTABLE 0x00000010 /* 文件不可变 */
#define EXT4_INODE_FLAG_APPEND 0x00000020    /* 文件只能追加写入 */
#define EXT4_INODE_FLAG_NODUMP 0x00000040    /* 不转储文件 */
#define EXT4_INODE_FLAG_NOATIME 0x00000080   /* 不更新访问时间（atime） */

/* 压缩相关标志 */
#define EXT4_INODE_FLAG_DIRTY 0x00000100     /* 脏文件（需要写回） */
#define EXT4_INODE_FLAG_COMPRBLK 0x00000200  /* 一个或多个压缩簇 */
#define EXT4_INODE_FLAG_NOCOMPR 0x00000400   /* 不压缩 */
#define EXT4_INODE_FLAG_ECOMPR 0x00000800    /* 压缩错误 */

#define EXT4_INODE_FLAG_INDEX 0x00001000     /* 哈希索引目录 */
#define EXT4_INODE_FLAG_IMAGIC 0x00002000    /* AFS 目录 */
#define EXT4_INODE_FLAG_JOURNAL_DATA 0x00004000 /* 文件数据应记录到日志中 */
#define EXT4_INODE_FLAG_NOTAIL 0x00008000    /* 文件尾部不应合并 */
#define EXT4_INODE_FLAG_DIRSYNC 0x00010000   /* 目录同步行为（仅限目录） */
#define EXT4_INODE_FLAG_TOPDIR 0x00020000    /* 目录层次结构的顶层 */
#define EXT4_INODE_FLAG_HUGE_FILE 0x00040000 /* 设置为每个大文件 */
#define EXT4_INODE_FLAG_EXTENTS 0x00080000   /* inode 使用扩展区 */
#define EXT4_INODE_FLAG_EA_INODE 0x00200000  /* inode 用于大型扩展属性 (EA) */
#define EXT4_INODE_FLAG_EOFBLOCKS 0x00400000 /* 分配了超出 EOF 的块 */
#define EXT4_INODE_FLAG_RESERVED 0x80000000  /* 为 ext4 库保留 */

#define EXT4_INODE_ROOT_INDEX 2               /* 根索引 inode 编号 */

#define EXT4_DIRECTORY_FILENAME_LEN 255       /* ext4 目录中最大文件名长度 */


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

#define ext4_ialloc_bitmap_csum(...) 0
struct ext4_inode_ref {
	Buffer *block;    /**< 关联的块信息。 */
	struct ext4_inode *inode;   /**< 指向具体的 inode 结构。 */
	FileSystem *fs;         /**< 关联的文件系统。 */
	uint32_t index;             /**< inode 的索引。 */
	bool dirty;                 /**< 标志位，表示 inode 是否被修改。 */
};

#define EXT4_IS_DIR(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_DIRECTORY)//判断是否是目录
#define EXT4_IS_FILE(mode) (((mode) & EXT4_INODE_MODE_TYPE_MASK) == EXT4_INODE_MODE_FILE)//判断是否是文件
void ext4_inode_set_indirect_block(struct ext4_inode *inode, uint32_t idx, uint32_t block);
void ext4_inode_set_direct_block(struct ext4_inode *inode, uint32_t idx, uint32_t block);
uint32_t ext4_inode_get_indirect_block(struct ext4_inode* inode, uint32_t idx);
bool ext4_inode_has_flag(struct ext4_inode* inode, uint32_t f);
struct ext4_extent_header * ext4_inode_get_extent_header(struct ext4_inode *inode);
void ext4_inode_set_size(struct ext4_inode* inode, uint64_t size);
uint32_t ext4_inode_get_flags(struct ext4_inode* inode);
bool ext4_inode_is_type(struct ext4_sblock *sb, struct ext4_inode *inode, uint32_t type);
void ext4_inode_clear_flag(struct ext4_inode *inode, uint32_t f);
void ext4_inode_set_flag(struct ext4_inode *inode, uint32_t f);
void ext4_inode_set_csum(struct ext4_sblock *sb, struct ext4_inode *inode,
uint32_t checksum);
uint64_t ext4_inode_get_blocks_count(struct ext4_sblock *sb, struct ext4_inode *inode);
static uint32_t ext4_inode_block_bits_count(uint32_t block_size);
uint32_t ext4_inode_get_csum(struct ext4_sblock* sb, struct ext4_inode* inode);
uint32_t ext4_inode_get_generation(struct ext4_inode* inode);
uint32_t ext4_inode_get_mode(struct ext4_sblock* sb, struct ext4_inode* inode);
void ext4_inode_set_mode(struct ext4_sblock *sb, struct ext4_inode *inode, uint32_t mode);
uint32_t ext4_inode_get_uid(struct ext4_inode *inode);
void ext4_inode_set_uid(struct ext4_inode *inode, uint32_t uid);
uint64_t ext4_inode_get_size(struct ext4_sblock *sb, struct ext4_inode *inode);
int ext4_fs_get_inode_ref(FileSystem *fs, uint32_t index,struct ext4_inode_ref *ref,bool initialized);
int ext4_fs_put_inode_ref(struct ext4_inode_ref* ref);
void ext4_fs_inode_blocks_init(struct FileSystem *fs,struct ext4_inode_ref *inode_ref);
#endif