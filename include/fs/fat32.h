#ifndef _FS_FAT32_H
#define _FS_FAT32_H

#include <xkernel/block_device.h>
#include <xkernel/types.h>
#include <fs/fs.h>
typedef struct Dirent Dirent;
typedef struct FAT32BootParamBlock {
	// 通用的引导扇区属性
	u8 BS_jmpBoot[3];
	u8 BS_OEMName[8];
	u16 BPB_BytsPerSec;		// 扇区大小（字节）
	u8 BPB_SecPerClus;		// 簇大小（扇区）
	u16 BPB_RsvdSecCnt;		// 保留扇区数
	u8 BPB_NumFATs;			// FAT 数量
	u16 BPB_RootEntCnt;		// 根目录条目数量
	u16 BPB_TotSec16;		// 逻辑分区中的扇区数量
	u8 BPB_Media;			// 硬盘介质类型
	u16 BPB_FATSz16;		// FAT 的大小（扇区），只有FAT12/16使用
	u16 BPB_SecPerTrk;		// 一个磁道的扇区数
	u16 BPB_NumHeads;		// 磁头的数量
	u32 BPB_HiddSec;		// 隐藏扇区数量
	u32 BPB_TotSec32;		// 逻辑分区中的扇区数量，FAT32使用
	// FAT32 引导扇区属性
	u32 BPB_FATSz32;		// FAT的大小（扇区），FAT32使用
	u16 BPB_ExtFlags;		// 扩展标志
	u16 BPB_FSVer;			// 高字节是主版本号，低字节是次版本号
	u32 BPB_RootClus;		// 根目录簇号
	u16 BPB_FSInfo;			// 文件系统信息结构扇区号
	u16 BPB_BkBootSec;		// 备份引导扇区号
	u8 BPB_Reserved[12];	// 保留
	u8 BS_DrvNum;			// 物理驱动器号，硬盘通常是 0x80 开始的数字
	u8 BS_Reserved1;		// 为 Windows NT 保留的标志位
	u8 BS_BootSig;			// 固定的数字签名，必须是 0x28 或 0x29 
	u32 BS_VolID;			// 分区的序列号，可以忽略
	u8 BS_VolLab[11];		// 卷标 
	u8 BS_FilSysType[8];	// 文件系统 ID，通常是"FAT32"
	u8 BS_CodeReserved[420];// 引导代码
	u8 BS_Signature[2];		// 可引导分区签名，0xAA55
} __attribute__((packed)) FAT32BootParamBlock;

/*fat32文件系统中的目录项*/
typedef struct FAT32Directory {
	u8 DIR_Name[11];
	u8 DIR_Attr;
	u8 DIR_NTRes;
	u8 DIR_CrtTimeTenth;
	u16 DIR_CrtTime;
	u16 DIR_CrtDate;
	u16 DIR_LstAccDate;
	u16 DIR_FstClusHI;
	u16 DIR_WrtTime;
	u16 DIR_WrtDate;
	u16 DIR_FstClusLO;
	u32 DIR_FileSize;
} __attribute__((packed)) FAT32Directory;

typedef struct FAT32LongDirectory {
	u8 LDIR_Ord;
	unsigned short LDIR_Name1[5];
	u8 LDIR_Attr;
	u8 LDIR_Type;
	u8 LDIR_Chksum;
	unsigned short LDIR_Name2[6];
	u16 LDIR_FstClusLO;
	unsigned short LDIR_Name3[2];
} __attribute__((packed)) FAT32LongDirectory;

struct bpb{
	u16 bytes_per_sec;   /* 每扇区字节数 */
	u8 sec_per_clus;     /* 每簇扇区数 */
	u16 rsvd_sec_cnt;    /* 保留扇区数 */
	u8 fat_cnt;          /* FAT 表数量 */
	u32 hidd_sec;        /* 隐藏扇区数量 */
	u32 tot_sec;         /* 总扇区数（包括所有区域） */
	u32 fat_sz;          /* FAT 区域的扇区数 */
	u32 root_clus;       /* 根簇号 */
};

#define BYTES_LONGENT 13

#define BPB_SIZE sizeof(FAT32BootParamBlock)
#define DIR_SIZE sizeof(FAT32Directory)

// 如果DIR_Name[0] == 0xE5，则表示该目录项已经被删除
#define FAT32_INVALID_ENTRY 0xE5

// FAT32 文件、目录属性
#define ATTR_READ_ONLY 0x01		//只读属性
#define ATTR_HIDDEN 0x02		//隐藏属性
#define ATTR_SYSTEM 0x04		//系统属性
#define ATTR_VOLUME_ID 0x08		//卷标属性
#define ATTR_DIRECTORY 0x10		//目录属性
#define ATTR_ARCHIVE 0x20		//归档属性
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define CHAR2LONGENT 26
#define LAST_LONG_ENTRY 0x40
#define DIRENT_SIZE 32

#define CLUS_SIZE(fs) ((fs)->superBlock.bytes_per_clus)

void init_root_fs(void);
int Fatfile_write(struct Dirent *file, unsigned long src, unsigned int off, unsigned int n);
int Fatfile_read(struct Dirent *file, unsigned long dst, unsigned int off, unsigned int n);
int rmfile(Dirent* file);
#endif