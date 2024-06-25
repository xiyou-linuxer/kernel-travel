#ifndef EXT4_TYPES_H_
#define EXT4_TYPES_H_

#include <xkernel/types.h>

#define UUID_SIZE 16

/*ext4原生的超级块属性*/
struct ext4_sblock {
	uint32_t inodes_count;		   /* 文件系统中 I-node 的总数。 */
	uint32_t blocks_count_lo;	  /* 文件系统中块的总数 */
	uint32_t reserved_blocks_count_lo; /* 预留块的总数（低 32 位）*/
	uint32_t free_blocks_count_lo;     /* 空闲块的总数（低 32 位) */
	uint32_t free_inodes_count;	/* 空闲 I-node 的总数 */
	uint32_t first_data_block;	 /* 文件系统中的第一个数据块 */
	uint32_t log_block_size;	   /* Block size */
	uint32_t log_cluster_size;	 /* Obsoleted fragment size */
	uint32_t blocks_per_group;	 /* Number of blocks per group */
	uint32_t frags_per_group;	  /* Obsoleted fragments per group */
	uint32_t inodes_per_group;	 /* Number of inodes per group */
	uint32_t mount_time;		   /* Mount time */
	uint32_t write_time;		   /* Write time */
	uint16_t mount_count;		   /* Mount count */
	uint16_t max_mount_count;	  /* Maximal mount count */
	uint16_t magic;			   /* Magic signature */
	uint16_t state;			   /* File system state */
	uint16_t errors;		   /* Behavior when detecting errors */
	uint16_t minor_rev_level;	  /* Minor revision level */
	uint32_t last_check_time;	  /* Time of last check */
	uint32_t check_interval;	   /* Maximum time between checks */
	uint32_t creator_os;		   /* Creator OS */
	uint32_t rev_level;		   /* Revision level */
	uint16_t def_resuid;		   /* Default uid for reserved blocks */
	uint16_t def_resgid;		   /* Default gid for reserved blocks */

	/* Fields for EXT4_DYNAMIC_REV superblocks only. */
	uint32_t first_inode;	 /* First non-reserved inode */
	uint16_t inode_size;	  /* Size of inode structure */
	uint16_t block_group_index;   /* Block group index of this superblock */
	uint32_t features_compatible; /* Compatible feature set */
	uint32_t features_incompatible;  /* Incompatible feature set */
	uint32_t features_read_only;     /* Readonly-compatible feature set */
	uint8_t uuid[UUID_SIZE];		 /* 128-bit uuid for volume */
	char volume_name[16];		 /* Volume name */
	char last_mounted[64];		 /* Directory where last mounted */
	uint32_t algorithm_usage_bitmap; /* For compression */

	/*
	 * Performance hints. Directory preallocation should only
	 * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
	 */
	uint8_t s_prealloc_blocks; /* Number of blocks to try to preallocate */
	uint8_t s_prealloc_dir_blocks;  /* Number to preallocate for dirs */
	uint16_t s_reserved_gdt_blocks; /* Per group desc for online growth */

	/*
	 * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	uint8_t journal_uuid[UUID_SIZE];      /* UUID of journal superblock */
	uint32_t journal_inode_number; /* Inode number of journal file */
	uint32_t journal_dev;	  /* Device number of journal file */
	uint32_t last_orphan;	  /* Head of list of inodes to delete */
	uint32_t hash_seed[4];	 /* HTREE hash seed */
	uint8_t default_hash_version;  /* Default hash version to use */
	uint8_t journal_backup_type;
	uint16_t desc_size;	  /* Size of group descriptor */
	uint32_t default_mount_opts; /* Default mount options */
	uint32_t first_meta_bg;      /* First metablock block group */
	uint32_t mkfs_time;	  /* When the filesystem was created */
	uint32_t journal_blocks[17]; /* Backup of the journal inode */

	/* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
	uint32_t blocks_count_hi;	  /* Blocks count */
	uint32_t reserved_blocks_count_hi; /* Reserved blocks count */
	uint32_t free_blocks_count_hi;     /* Free blocks count */
	uint16_t min_extra_isize;    /* All inodes have at least # bytes */
	uint16_t want_extra_isize;   /* New inodes should reserve # bytes */
	uint32_t flags;		     /* Miscellaneous flags */
	uint16_t raid_stride;	/* RAID stride */
	uint16_t mmp_interval;       /* # seconds to wait in MMP checking */
	uint64_t mmp_block;	  /* Block for multi-mount protection */
	uint32_t raid_stripe_width;  /* Blocks on all data disks (N * stride) */
	uint8_t log_groups_per_flex; /* FLEX_BG group size */
	uint8_t checksum_type;
	uint16_t reserved_pad;
	uint64_t kbytes_written; /* Number of lifetime kilobytes written */
	uint32_t snapshot_inum;  /* I-node number of active snapshot */
	uint32_t snapshot_id;    /* Sequential ID of active snapshot */
	uint64_t
	    snapshot_r_blocks_count; /* Reserved blocks for active snapshot's
					future use */
	uint32_t
	    snapshot_list; /* I-node number of the head of the on-disk snapshot
			      list */
	uint32_t error_count;	 /* Number of file system errors */
	uint32_t first_error_time;    /* 第一次发生错误的时间 */
	uint32_t first_error_ino;     /* I-node involved in first error */
	uint64_t first_error_block;   /* Block involved of first error */
	uint8_t first_error_func[32]; /* Function where the error happened */
	uint32_t first_error_line;    /* Line number where error happened */
	uint32_t last_error_time;     /* 最近一次发生错误的时间 */
	uint32_t last_error_ino;      /* I-node involved in last error */
	uint32_t last_error_line;     /* Line number where error happened */
	uint64_t last_error_block;    /* Block involved of last error */
	uint8_t last_error_func[32];  /* Function where the error happened */
	uint8_t mount_opts[64];
	uint32_t usr_quota_inum;	/* 用于跟踪用户配额的 I-node */
	uint32_t grp_quota_inum;	/* 用于跟踪组配额的 I-node */
	uint32_t overhead_clusters;	/* overhead blocks/clusters in fs */
	uint32_t backup_bgs[2];	/* groups with sparse_super2 SBs */
	uint8_t  encrypt_algos[4];	/* Encryption algorithms in use  */
	uint8_t  encrypt_pw_salt[16];	/* Salt used for string2key algorithm */
	uint32_t lpf_ino;		/* Location of the lost+found inode */
	uint32_t padding[100];	/* Padding to the end of the block */
	uint32_t checksum;		/*超级块的 CRC32 校验和 */
};
#endif /* EXT4_TYPES_H_ */