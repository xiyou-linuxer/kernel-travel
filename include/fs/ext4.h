#ifndef EXT4_TYPES_H_
#define EXT4_TYPES_H_

#include <xkernel/types.h>
#include <fs/fs.h>
#include <fs/buf.h>

/*存放如ext4文件系统的信息*/
#define UUID_SIZE 16
#define EXT4_MIN_BLOCK_GROUP_DESCRIPTOR_SIZE 32
#define EXT4_MAX_BLOCK_GROUP_DESCRIPTOR_SIZE 64

#define EXT4_MIN_BLOCK_SIZE 1024  /* 最小块大小：1 KiB */
#define EXT4_MAX_BLOCK_SIZE 65536 /* 最大块大小：64 KiB */
#define EXT4_REV0_INODE_SIZE 128  /* EXT4 版本0的 inode 大小 */

#define EXT4_INODE_BLOCK_SIZE 512 /* inode 块大小 */

#define EXT4_INODE_DIRECT_BLOCK_COUNT 12 /* inode 的直接块指针数量 */
#define EXT4_INODE_INDIRECT_BLOCK EXT4_INODE_DIRECT_BLOCK_COUNT /* 间接块指针的索引 */
#define EXT4_INODE_DOUBLE_INDIRECT_BLOCK (EXT4_INODE_INDIRECT_BLOCK + 1) /* 双重间接块指针的索引 */
#define EXT4_INODE_TRIPPLE_INDIRECT_BLOCK (EXT4_INODE_DOUBLE_INDIRECT_BLOCK + 1) /* 三重间接块指针的索引 */
#define EXT4_INODE_BLOCKS (EXT4_INODE_TRIPPLE_INDIRECT_BLOCK + 1) /* inode 块指针总数 */
#define EXT4_INODE_INDIRECT_BLOCK_COUNT                                        \
	(EXT4_INODE_BLOCKS - EXT4_INODE_DIRECT_BLOCK_COUNT) /* 间接块指针的数量 */


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
	uint32_t inode;	/* 目录项对应的 inode 编号 */
	uint16_t entry_len; /* 到下一个目录项的距离 */
	uint8_t name_len;   /* 文件名长度的低8位 */

	union ext4_dir_en_internal in; /* 内部联合体，包含文件名长度的高8位或 inode 类型 */
	uint8_t name[]; /* 目录项名称 */
};

/* 目录结构 */
struct ext4_dir {
	/** @brief 当前dir结构体对应的Dirent项 */
	Dirent *dirent;
	/** @brief 当前目录项。 */
	Dirent de;
	/** @brief 下一个目录项的偏移量。 */
	uint64_t next_off;
} ext4_dir;
#endif /* EXT4_TYPES_H_ */