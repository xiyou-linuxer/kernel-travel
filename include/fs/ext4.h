#ifndef EXT4_TYPES_H_
#define EXT4_TYPES_H_

#include <xkernel/types.h>
#include <fs/buf.h>
#include <fs/ext4.h>


typedef struct FileSystem FileSystem;
typedef struct Dirent Dirent;

extern FileSystem *ext4Fs;
extern FileSystem *procFs;

#define EXT4_DIR_ENTRY_OFFSET_TERM (uint64_t)(-1)

/*存放如ext4文件系统的信息*/
#define UUID_SIZE 16
#define EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE 32
#define EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE 64

#define EXT4_MIN_BLOCK_SIZE 1024  /* 最小块大小：1 KiB */
#define EXT4_MAX_BLOCK_SIZE 65536 /* 最大块大小：64 KiB */
#define EXT4_REV0_INODE_SIZE 128  /* EXT4 版本0的 inode 大小 */

#define EXT4_INODE_BLOCK_SIZE 512 /* inode 块大小 */
#define PH_BLOCK_SIZE 512/*物理扇区的大小*/

#define EXT4_INODE_DIRECT_BLOCK_COUNT 12 /* inode 的直接块指针数量 */
#define EXT4_INODE_INDIRECT_BLOCK EXT4_INODE_DIRECT_BLOCK_COUNT /* 间接块指针的索引 */
#define EXT4_INODE_DOUBLE_INDIRECT_BLOCK (EXT4_INODE_INDIRECT_BLOCK + 1) /* 双重间接块指针的索引 */
#define EXT4_INODE_TRIPPLE_INDIRECT_BLOCK (EXT4_INODE_DOUBLE_INDIRECT_BLOCK + 1) /* 三重间接块指针的索引 */
#define EXT4_INODE_BLOCKS (EXT4_INODE_TRIPPLE_INDIRECT_BLOCK + 1) /* inode 块指针总数 */
#define EXT4_INODE_INDIRECT_BLOCK_COUNT                                        \
	(EXT4_INODE_BLOCKS - EXT4_INODE_DIRECT_BLOCK_COUNT) /* 间接块指针的数量 */

#define to_le64(_n) _n
#define to_le32(_n) _n
#define to_le16(_n) _n

#define EXT4_CHECKSUM_CRC32C 1
#define EXT4_CRC32_INIT (0xFFFFFFFFUL)

#define ext4_fsblk_t uint64_t
#define ext4_lblk_t uint32_t

/*ext4原生的超级块属性*/
struct ext4_sblock {
    uint32_t inodes_count;           /* 文件系统中 I-node 的总数 */
    uint32_t blocks_count_lo;        /* 文件系统中块的总数（低 32 位） */
    uint32_t reserved_blocks_count_lo; /* 预留块的总数（低 32 位） */
    uint32_t free_blocks_count_lo;   /* 空闲块的总数（低 32 位） */
    uint32_t free_inodes_count;      /* 空闲 I-node 的总数 */
    uint32_t first_data_block;       /* 文件系统中的第一个数据块 */
    uint32_t log_block_size;         /* 块大小，以 1024 字节为单位的对数值 */
    uint32_t log_cluster_size;       /* 已废弃的片段大小，以 1024 字节为单位的对数值 */
    uint32_t blocks_per_group;       /* 每组的块数 */
    uint32_t frags_per_group;        /* 已废弃，每组的片段数 */
    uint32_t inodes_per_group;       /* 每组的 I-node 数 */
    uint32_t mount_time;             /* 挂载时间 */
    uint32_t write_time;             /* 最后写入时间 */
    uint16_t mount_count;            /* 挂载次数 */
    uint16_t max_mount_count;        /* 最大挂载次数 */
    uint16_t magic;                  /* 魔数，用于标识文件系统类型 */
    uint16_t state;                  /* 文件系统状态 */
    uint16_t errors;                 /* 检测到错误时的行为 */
    uint16_t minor_rev_level;        /* 次要修订级别 */
    uint32_t last_check_time;        /* 最后一次检查的时间 */
    uint32_t check_interval;         /* 检查之间的最大时间间隔 */
    uint32_t creator_os;             /* 创建文件系统的操作系统 */
    uint32_t rev_level;              /* 修订级别 */
    uint16_t def_resuid;             /* 预留块的默认用户 ID */
    uint16_t def_resgid;             /* 预留块的默认组 ID */

    /* 仅对 EXT4_DYNAMIC_REV 超级块字段 */
    uint32_t first_inode;            /* 第一个非保留的 I-node */
    uint16_t inode_size;             /* I-node 结构的大小 */
    uint16_t block_group_index;      /* 此超级块的块组索引 */
    uint32_t features_compatible;    /* 兼容特性集 */
    uint32_t features_incompatible;  /* 不兼容特性集 */
    uint32_t features_read_only;     /* 只读兼容特性集 */
    uint8_t uuid[UUID_SIZE];         /* 卷的 128 位 UUID */
    char volume_name[16];            /* 卷名 */
    char last_mounted[64];           /* 最后一次挂载的目录 */
    uint32_t algorithm_usage_bitmap; /* 压缩算法使用的位图 */

    /*
     * 性能提示。只有在设置了 EXT4_FEATURE_COMPAT_DIR_PREALLOC 标志时，
     * 才会进行目录预分配。
     */
    uint8_t s_prealloc_blocks;       /* 尝试预分配的块数 */
    uint8_t s_prealloc_dir_blocks;   /* 目录预分配的块数 */
    uint16_t s_reserved_gdt_blocks;  /* 在线增长时每组的描述符块数 */

    /*
     * 如果设置了 EXT4_FEATURE_COMPAT_HAS_JOURNAL，支持日志记录。
     */
    uint8_t journal_uuid[UUID_SIZE]; /* 日志超级块的 UUID */
    uint32_t journal_inode_number;   /* 日志文件的 I-node 编号 */
    uint32_t journal_dev;            /* 日志文件的设备编号 */
    uint32_t last_orphan;            /* 要删除的 I-node 链表的头 */
    uint32_t hash_seed[4];           /* HTREE 哈希种子 */
    uint8_t default_hash_version;    /* 使用的默认哈希版本 */
    uint8_t journal_backup_type;
    uint16_t desc_size;              /* 组描述符的大小 */
    uint32_t default_mount_opts;     /* 默认挂载选项 */
    uint32_t first_meta_bg;          /* 第一个元数据块组 */
    uint32_t mkfs_time;              /* 文件系统创建时间 */
    uint32_t journal_blocks[17];     /* 日志 I-node 的备份 */

    /* 如果设置了 EXT4_FEATURE_COMPAT_64BIT，支持 64 位 */
    uint32_t blocks_count_hi;        /* 块总数（高 32 位） */
    uint32_t reserved_blocks_count_hi; /* 预留块的总数（高 32 位） */
    uint32_t free_blocks_count_hi;   /* 空闲块的总数（高 32 位） */
    uint16_t min_extra_isize;        /* 所有 I-node 至少有 # 字节 */
    uint16_t want_extra_isize;       /* 新 I-node 应预留 # 字节 */
    uint32_t flags;                  /* 杂项标志 */
    uint16_t raid_stride;            /* RAID 步幅 */
    uint16_t mmp_interval;           /* 在 MMP 检查中等待的秒数 */
    uint64_t mmp_block;              /* 多重挂载保护块 */
    uint32_t raid_stripe_width;      /* 所有数据磁盘上的块数（N * 步幅） */
    uint8_t log_groups_per_flex;     /* FLEX_BG 组大小 */
    uint8_t checksum_type;
    uint16_t reserved_pad;
    uint64_t kbytes_written;         /* 写入的总千字节数 */
    uint32_t snapshot_inum;          /* 活动快照的 I-node 编号 */
    uint32_t snapshot_id;            /* 活动快照的顺序 ID */
    uint64_t snapshot_r_blocks_count; /* 预留给活动快照的块数 */
    uint32_t snapshot_list;          /* 磁盘上快照链表的头部 I-node 编号 */
    uint32_t error_count;            /* 文件系统错误的次数 */
    uint32_t first_error_time;       /* 第一次发生错误的时间 */
    uint32_t first_error_ino;        /* 第一次发生错误涉及的 I-node */
    uint64_t first_error_block;      /* 第一次发生错误涉及的块 */
    uint8_t first_error_func[32];    /* 第一次发生错误的函数 */
    uint32_t first_error_line;       /* 第一次发生错误的行号 */
    uint32_t last_error_time;        /* 最近一次发生错误的时间 */
    uint32_t last_error_ino;         /* 最近一次发生错误涉及的 I-node */
    uint32_t last_error_line;        /* 最近一次发生错误的行号 */
    uint64_t last_error_block;       /* 最近一次发生错误涉及的块 */
    uint8_t last_error_func[32];     /* 最近一次发生错误的函数 */
    uint8_t mount_opts[64];
    uint32_t usr_quota_inum;         /* 用于跟踪用户配额的 I-node */
    uint32_t grp_quota_inum;         /* 用于跟踪组配额的 I-node */
    uint32_t overhead_clusters;      /* 文件系统中的开销块/簇 */
    uint32_t backup_bgs[2];          /* 具有稀疏超级块的组 */
    uint8_t encrypt_algos[4];        /* 使用的加密算法 */
    uint8_t encrypt_pw_salt[16];     /* 用于 string2key 算法的盐值 */
    uint32_t lpf_ino;                /* lost+found I-node 的位置 */
    uint32_t padding[100];           /* 填充到块的末尾 */
    uint32_t checksum;               /* 超级块的 CRC32 校验和 */
};

/*
 * 日志文件系统
 */
#define JBD_CRC32_CHKSUM   1
#define JBD_MD5_CHKSUM     2
#define JBD_SHA1_CHKSUM    3
#define JBD_CRC32C_CHKSUM  4
#define JBD_USERS_MAX 48
#define JBD_USERS_SIZE (UUID_SIZE * JBD_USERS_MAX)
#define JBD_CRC32_CHKSUM_SIZE 4
#define JBD_CHECKSUM_BYTES (32 / sizeof(uint32_t))


struct jbd_bhdr {
	uint32_t		magic;
	uint32_t		blocktype;
	uint32_t		sequence;
};

struct jbd_sb {
/* 0x0000 */
	struct jbd_bhdr header;

/* 0x000C */
	/* Static information describing the journal */
	uint32_t	blocksize;		/* journal device blocksize */
	uint32_t	maxlen;		/* total blocks in journal file */
	uint32_t	first;		/* first block of log information */

/* 0x0018 */
	/* Dynamic information describing the current state of the log */
	uint32_t	sequence;		/* first commit ID expected in log */
	uint32_t	start;		/* blocknr of start of log */

/* 0x0020 */
	/* Error value, as set by journal_abort(). */
	int32_t 	error_val;

/* 0x0024 */
	/* Remaining fields are only valid in a version-2 superblock */
	uint32_t	feature_compat; 	/* compatible feature set */
	uint32_t	feature_incompat; 	/* incompatible feature set */
	uint32_t	feature_ro_compat; 	/* readonly-compatible feature set */
/* 0x0030 */
	uint8_t 	uuid[UUID_SIZE];		/* 128-bit uuid for journal */

/* 0x0040 */
	uint32_t	nr_users;		/* Nr of filesystems sharing log */

	uint32_t	dynsuper;		/* Blocknr of dynamic superblock copy*/

/* 0x0048 */
	uint32_t	max_transaction;	/* Limit of journal blocks per trans.*/
	uint32_t	max_trandata;	/* Limit of data blocks per trans. */

/* 0x0050 */
	uint8_t 	checksum_type;	/* checksum type */
	uint8_t 	padding2[3];
	uint32_t	padding[42];
	uint32_t	checksum;		/* crc32c(superblock) */

/* 0x0100 */
	uint8_t 	users[JBD_USERS_SIZE];		/* ids of all fs'es sharing the log */

/* 0x0400 */
};

/*inode信息*/
struct ext4_inode {
	uint16_t mode;		    /* 文件模式 */
	uint16_t uid;		    /* 低16位的所有者用户ID */
	uint32_t size_lo;	    /* 文件大小（低32位） */
	uint32_t access_time;       /* 最后访问时间 */
	uint32_t change_inode_time; /* inode 修改时间 */
	uint32_t modification_time; /* 最后修改时间 */
	uint32_t deletion_time;     /* 删除时间 */
	uint16_t gid;		    /* 低16位的组ID */
	uint16_t links_count;       /* 链接计数 */
	uint32_t blocks_count_lo;   /* 块计数（低32位） */
	uint32_t flags;		    /* 文件标志 */
	uint32_t unused_osd1;       /* 操作系统依赖 - 在 HelenOS 中未使用 */
	uint32_t blocks[EXT4_INODE_BLOCKS]; /* 块指针数组 */
	uint32_t generation;		    /* 文件版本（用于 NFS） */
	uint32_t file_acl_lo;		    /* 文件 ACL（低32位） */
	uint32_t size_hi;                /* 文件大小（高32位） */
	uint32_t obso_faddr; /* 废弃的碎片地址 */

	union {
		struct {
			uint16_t blocks_high;     /* 块计数（高16位） */
			uint16_t file_acl_high;   /* 文件 ACL（高16位） */
			uint16_t uid_high;        /* 所有者用户ID（高16位） */
			uint16_t gid_high;        /* 组ID（高16位） */
			uint16_t checksum_lo;     /* CRC32 校验和（低16位） */
			uint16_t reserved2;       /* 保留 */
		} linux2;
		struct {
			uint16_t reserved1;       /* 保留 */
			uint16_t mode_high;       /* 文件模式（高16位） */
			uint16_t uid_high;        /* 所有者用户ID（高16位） */
			uint16_t gid_high;        /* 组ID（高16位） */
			uint32_t author;          /* 作者 */
		} hurd2;
	} osd2;

	uint16_t extra_isize;          /* 额外的 inode 大小 */
	uint16_t checksum_hi;	      /* CRC32 校验和（高16位） */
	uint32_t ctime_extra;         /* 额外的修改时间（纳秒 << 2 | 纪元） */
	uint32_t mtime_extra;         /* 额外的修改时间（纳秒 << 2 | 纪元） */
	uint32_t atime_extra;         /* 额外的访问时间（纳秒 << 2 | 纪元） */
	uint32_t crtime;              /* 文件创建时间 */
	uint32_t crtime_extra;        /* 额外的文件创建时间（纳秒 << 2 | 纪元） */
	uint32_t version_hi;          /* 64 位版本的高 32 位 */
};

union ext4_dir_en_internal {
	uint8_t name_length_high; /* 文件名长度的高8位 */
	uint8_t inode_type;       /* 引用的 inode 类型（在 rev >= 0.5 时有效） */
};

/* 目录项结构 */
struct ext4_dir_en {
	uint32_t inode;	/* 与目录项对应的inode号 */
	uint16_t entry_len; /*到下一个目录条目的距离*/
	uint8_t name_len;   /* 名称长度的低8位 */
	union ext4_dir_en_internal in;
	uint8_t name[]; /* 目录项的名字 */
};

/* 目录结构 */
struct ext4_dir {
	/** @brief 当前dir结构体对应的Dirent项 */
	Dirent *pdirent;
	/** @brief 当前目录项。 */
	Dirent *de;
	/** @brief 下一个目录项的偏移量。 */
	uint64_t next_off;
};

/*对于块缓冲的包装*/
struct ext4_block {
	/**@brief   Logical block ID*/
	uint64_t lb_id;

	/**@brief   Buffer */
	Buffer *buf;
};

/* 表示块组中的inode位图未初始化或未使用 */
#define EXT4_BLOCK_GROUP_INODE_UNINIT 0x0001
/* 表示块组中的块位图未初始化或未使用 */
#define EXT4_BLOCK_GROUP_BLOCK_UNINIT 0x0002
/* 磁盘上的 i-node 表已初始化为零 */
#define EXT4_BLOCK_GROUP_ITABLE_ZEROED 0x0004

/*ext4文件系统的魔数*/
#define EXT4_SUPERBLOCK_MAGIC 0xEF53
#define EXT4_SUPERBLOCK_SIZE 1024
#define EXT4_SUPERBLOCK_OFFSET 1024

/*ext4超级块中的操作系统标识*/
#define EXT4_SUPERBLOCK_OS_LINUX 0
#define EXT4_SUPERBLOCK_OS_HURD 1


/*
 * 文件系统的各种标志位
 */
#define EXT4_SUPERBLOCK_FLAGS_SIGNED_HASH       0x0001  /* 文件系统使用带符号哈希用于元数据 */
#define EXT4_SUPERBLOCK_FLAGS_UNSIGNED_HASH     0x0002  /* 文件系统使用无符号哈希用于元数据 */
#define EXT4_SUPERBLOCK_FLAGS_TEST_FILESYS      0x0004  /* 测试文件系统，用于测试目的 */

/*
 * 文件系统状态
 */
#define EXT4_SUPERBLOCK_STATE_VALID_FS          0x0001  /* 文件系统已干净卸载 */
#define EXT4_SUPERBLOCK_STATE_ERROR_FS          0x0002  /* 文件系统检测到错误 */
#define EXT4_SUPERBLOCK_STATE_ORPHAN_FS         0x0004  /* 孤儿正在恢复中 */

/*
 * 检测到错误时的行为
 */
#define EXT4_SUPERBLOCK_ERRORS_CONTINUE         1       /* 错误时继续执行 */
#define EXT4_SUPERBLOCK_ERRORS_RO               2       /* 错误时将文件系统重新挂载为只读 */
#define EXT4_SUPERBLOCK_ERRORS_PANIC            3       /* 错误时系统紧急停机 */
#define EXT4_SUPERBLOCK_ERRORS_DEFAULT          EXT4_ERRORS_CONTINUE  /* 默认错误处理 */

/*
 * 兼容特性
 */
#define EXT4_FCOM_DIR_PREALLOC                  0x0001  /* 目录预分配 */
#define EXT4_FCOM_IMAGIC_INODES                 0x0002  /* IMagic节点支持 */
#define EXT4_FCOM_HAS_JOURNAL                   0x0004  /* 文件系统具有日志 */
#define EXT4_FCOM_EXT_ATTR                      0x0008  /* 扩展属性 */
#define EXT4_FCOM_RESIZE_INODE                  0x0010  /* 调整inode大小 */
#define EXT4_FCOM_DIR_INDEX                     0x0020  /* 目录索引 */

/*
 * 只读兼容特性
 */
#define EXT4_FRO_COM_SPARSE_SUPER               0x0001  /* 稀疏超级块 */
#define EXT4_FRO_COM_LARGE_FILE                 0x0002  /* 大文件支持 (> 4GB) */
#define EXT4_FRO_COM_BTREE_DIR                  0x0004  /* B树目录索引 */
#define EXT4_FRO_COM_HUGE_FILE                  0x0008  /* 巨大文件支持 (> 2TB) */
#define EXT4_FRO_COM_GDT_CSUM                   0x0010  /* 组描述符校验和 */
#define EXT4_FRO_COM_DIR_NLINK                  0x0020  /* 目录链接计数 */
#define EXT4_FRO_COM_EXTRA_ISIZE                0x0040  /* 额外inode大小 */
#define EXT4_FRO_COM_QUOTA                      0x0100  /* 配额 */
#define EXT4_FRO_COM_BIGALLOC                   0x0200  /* 大块分配 */
#define EXT4_FRO_COM_METADATA_CSUM              0x0400  /* 元数据校验和 */

/*
 * 不兼容特性
 */
#define EXT4_FINCOM_COMPRESSION                 0x0001  /* 压缩 */
#define EXT4_FINCOM_FILETYPE                    0x0002  /* 文件类型 */
#define EXT4_FINCOM_RECOVER                     0x0004  /* 需要恢复 */
#define EXT4_FINCOM_JOURNAL_DEV                 0x0008  /* 日志设备 */
#define EXT4_FINCOM_META_BG                     0x0010  /* 组描述符中的元数据校验 */
#define EXT4_FINCOM_EXTENTS                     0x0040  /* Extents支持 */
#define EXT4_FINCOM_64BIT                       0x0080  /* 64位文件系统 */
#define EXT4_FINCOM_MMP                         0x0100  /* 多重挂载保护 */
#define EXT4_FINCOM_FLEX_BG                     0x0200  /* 灵活的块组 */
#define EXT4_FINCOM_EA_INODE                    0x0400  /* inode中的扩展属性 */
#define EXT4_FINCOM_DIRDATA                     0x1000  /* 目录条目中的数据 */
#define EXT4_FINCOM_BG_USE_META_CSUM            0x2000  /* 使用CRC32c进行块组校验 */
#define EXT4_FINCOM_LARGEDIR                    0x4000  /* 大目录 (> 2GB 或 3级哈希树) */
#define EXT4_FINCOM_INLINE_DATA                 0x8000  /* inode中的内联数据 */

/*
 * EXT2 supported feature set
 */
#define EXT2_SUPPORTED_FCOM 0x0000

#define EXT2_SUPPORTED_FINCOM                                   \
	(EXT4_FINCOM_FILETYPE | EXT4_FINCOM_META_BG)

#define EXT2_SUPPORTED_FRO_COM                                  \
	(EXT4_FRO_COM_SPARSE_SUPER |                            \
	 EXT4_FRO_COM_LARGE_FILE)

/*
 * EXT3 supported feature set
 */
#define EXT3_SUPPORTED_FCOM (EXT4_FCOM_DIR_INDEX)

#define EXT3_SUPPORTED_FINCOM                                 \
	(EXT4_FINCOM_FILETYPE | EXT4_FINCOM_META_BG)

#define EXT3_SUPPORTED_FRO_COM                                \
	(EXT4_FRO_COM_SPARSE_SUPER | EXT4_FRO_COM_LARGE_FILE)

/*
 * EXT4 supported feature set
 */
#define EXT4_SUPPORTED_FCOM (EXT4_FCOM_DIR_INDEX)

#define EXT4_SUPPORTED_FINCOM                              \
	(EXT4_FINCOM_FILETYPE | EXT4_FINCOM_META_BG |      \
	 EXT4_FINCOM_EXTENTS | EXT4_FINCOM_FLEX_BG |       \
	 EXT4_FINCOM_64BIT)

#define EXT4_SUPPORTED_FRO_COM                             \
	(EXT4_FRO_COM_SPARSE_SUPER |                       \
	 EXT4_FRO_COM_METADATA_CSUM |                      \
	 EXT4_FRO_COM_LARGE_FILE | EXT4_FRO_COM_GDT_CSUM | \
	 EXT4_FRO_COM_DIR_NLINK |                          \
	 EXT4_FRO_COM_EXTRA_ISIZE | EXT4_FRO_COM_HUGE_FILE)

/*Ignored features:
 * RECOVER - journaling in lwext4 is not supported
 *           (probably won't be ever...)
 * MMP - multi-mout protection (impossible scenario)
 * */
#define EXT_FINCOM_IGNORED                                 \
	EXT4_FINCOM_RECOVER | EXT4_FINCOM_MMP

/*索引节点表/位图未使用*/
#define EXT4_BLOCK_GROUP_INODE_UNINIT 0x0001
/* Block bitmap not in use */
#define EXT4_BLOCK_GROUP_BLOCK_UNINIT 0x0002
/* On-disk itable initialized to zero */
#define EXT4_BLOCK_GROUP_ITABLE_ZEROED 0x0004
/*用于将逻辑块号转化为物理扇区号*/
#define EXT4_LBA2PBA(block_idx) (block_idx* (1024 << ext4Fs->superBlock.ext4_sblock.log_block_size)/PH_BLOCK_SIZE)

/*ext文件系统支持的文件类型*/
enum 
{   
	EXT4_DE_UNKNOWN = 0,	// 未知类型
	EXT4_DE_REG_FILE,		// 常规文件
	EXT4_DE_DIR,			// 目录
	EXT4_DE_CHRDEV,			// 字符设备
	EXT4_DE_BLKDEV,			// 块设备
	EXT4_DE_FIFO,			// 有名管道（FIFO）
	EXT4_DE_SOCK,			// 套接字
	EXT4_DE_SYMLINK			// 符号链接（软链接）
};

#define EXT4_DIRENTRY_DIR_CSUM 0xDE
#define EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE 32
// EXT4 文件系统中块组描述符的最小大小（32字节）。
#define EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE 64
// EXT4 文件系统中块组描述符的最大大小（64字节）。
#define EXT4_MIN_BLOCK_SIZE 1024  /* 1 KiB */
// EXT4 文件系统中支持的最小块大小（1024字节，即1 KiB）。
#define EXT4_MAX_BLOCK_SIZE 65536 /* 64 KiB */
// EXT4 文件系统中支持的最大块大小（65536字节，即64 KiB）。
#define EXT4_REV0_INODE_SIZE 128
// EXT4 文件系统中，修订版0的inode大小（128字节）。
#define EXT4_INODE_BLOCK_SIZE 512
// EXT4 文件系统中，inode表的块大小（512字节）。
#define EXT4_INODE_DIRECT_BLOCK_COUNT 12
// EXT4 inode 结构中直接块指针的数量（12个）。
#define EXT4_INODE_INDIRECT_BLOCK EXT4_INODE_DIRECT_BLOCK_COUNT
// EXT4 inode 结构中间接块指针的索引，与直接块指针的数量相同（12）。
#define EXT4_INODE_DOUBLE_INDIRECT_BLOCK (EXT4_INODE_INDIRECT_BLOCK + 1)
// EXT4 inode 结构中双重间接块指针的索引（13）。
#define EXT4_INODE_TRIPPLE_INDIRECT_BLOCK (EXT4_INODE_DOUBLE_INDIRECT_BLOCK + 1)
// EXT4 inode 结构中三重间接块指针的索引（14）。
#define EXT4_INODE_BLOCKS (EXT4_INODE_TRIPPLE_INDIRECT_BLOCK + 1)
// EXT4 inode 结构中总的块指针数量（15）。
#define EXT4_INODE_INDIRECT_BLOCK_COUNT (EXT4_INODE_BLOCKS - EXT4_INODE_DIRECT_BLOCK_COUNT)
// EXT4 inode 结构中间接块指针的总数量（3），包括单重、双重和三重间接块。
#define IN_RANGE(b, first, len)	((b) >= (first) && (b) <= (first) + (len) - 1)
void ext4_init(FileSystem* fs);
#endif /* EXT4_TYPES_H_ */
