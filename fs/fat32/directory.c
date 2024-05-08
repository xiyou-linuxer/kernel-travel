#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <sync.h>
#include <debug.h>

extern struct lock mtx_file;

/*判断是否是目录项*/
int is_directory(FAT32Directory *f)
{
	return f->DIR_Attr & ATTR_DIRECTORY;
}

/**
 * @brief 从offset偏移开始，查询一个目录项
 * @param offset 开始查询的位置偏移
 * @param next_offset 下一个dirent的开始位置
 * @note 目前设计为仅在初始化时使用，因此使用file_read读取，无需外部加锁
 * @return 读取的内容长度。若为0，表示读到末尾
 */
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet) {
	lock_acquire(&mtx_file);
	char direntBuf[DIRENT_SIZE];
	ASSERT(offset % DIR_SIZE == 0);

	FileSystem *fs = dir->file_system;
	unsigned int j;
	FAT32Directory *f;
	FAT32LongDirectory *longEnt;
	int clusSize = CLUS_SIZE(fs);

	char tmpName[MAX_NAME_LEN];
	char tmpBuf[32] __attribute__((aligned(2)));
	unsigned short fullName[MAX_NAME_LEN]; 
	fullName[0] = 0;	      // 初始化为空字符串

	if (longSet)
		longSet->cnt = 0; // 初始化longSet有0个元素

	// 1. 跳过dir中的无效项目
	for (j = offset; j < dir->file_size; j += DIR_SIZE) {
		file_read(dir, 0, (unsigned long)direntBuf, j, DIR_SIZE);//读取目录项
		f = ((FAT32Directory *)direntBuf);

		// 跳过空项（FAT32_INVALID_ENTRY表示已删除）
		if (f->DIR_Name[0] == 0 || f->DIR_Name[0] == FAT32_INVALID_ENTRY)
			continue;

		// 跳过"."和".." (因为我们在解析路径时使用字符串匹配，不使用FAT32内置的.和..机制)
		if (strncmp((const char *)f->DIR_Name, ".          ", 11) == 0
			|| strncmp((const char *)f->DIR_Name, "..         ", 11) == 0)
			continue;

		// 是长文件名项（可能属于文件也可能属于目录）
		if (f->DIR_Attr == ATTR_LONG_NAME_MASK) {
			longEnt = (FAT32LongDirectory *)f;
			// 是第一项
			if (longEnt->LDIR_Ord & LAST_LONG_ENTRY) {
				tmpName[0] = 0;
				if (longSet)
					longSet->cnt = 0;
			}

			// 向longSet里面存放长文件名项的指针
			if (longSet)
				longSet->longEnt[longSet->cnt++] = longEnt;
			memcpy(tmpBuf, (char *)longEnt->LDIR_Name1, 10);
			memcpy(tmpBuf + 10, (char *)longEnt->LDIR_Name2, 12);
			memcpy(tmpBuf + 22, (char *)longEnt->LDIR_Name3, 4);
			wstrnins(fullName, (const unsigned short *)tmpBuf, 13);
		} else {
			if (wstrlen(fullName) != 0) {
				wstr2str(tmpName, fullName);
			} else {
				strcpy(tmpName, (const char *)f->DIR_Name);
				tmpName[11] = 0;
			}

			printk("find: \"%s\"\n", tmpName);

			// 2. 设置找出的dirent的信息（为NULL的无需设置）
			Dirent *dirent = dirent_alloc();
			strcpy(dirent->name, tmpName);

			/*extern struct FileDev file_dev_file;
			dirent->dev = &file_dev_file; */// 赋值设备指针

			dirent->raw_dirent = *f; // 指向根dirent
			dirent->file_system = fs;
			dirent->first_clus = f->DIR_FstClusHI * 65536 + f->DIR_FstClusLO;
			dirent->file_size = f->DIR_FileSize;
			dirent->parent_dir_off = j; // 父亲目录内偏移
			dirent->type = is_directory(f) ? DIRENT_DIR : DIRENT_FILE;
			dirent->parent_dirent = dir; // 设置父级目录项
			list_init(&dirent->child_list);
			dirent->linkcnt = 1;

			// 对于目录文件的大小，我们将其重置为其簇数乘以簇大小，不再是0
			if (dirent->raw_dirent.DIR_Attr & ATTR_DIRECTORY) {
				dirent->file_size = countClusters(dirent) * clusSize;
			}

			*file = dirent;
			*next_offset = j + DIR_SIZE;

			lock_release(&mtx_file);
			return DIR_SIZE;
		}
	}

	printk("no more dents in dir: %s\n", dir->name);
	*next_offset = dir->file_size;

	lock_release(&mtx_file);
	return 0; // 读到结尾
}

void sync_dirent_rawdata_back(Dirent *dirent) {
	// first_clus, file_size
	// name不需要，因为在文件创建阶段就已经固定了
	dirent->raw_dirent.DIR_FstClusHI = dirent->first_clus / 65536;
	dirent->raw_dirent.DIR_FstClusLO = dirent->first_clus % 65536;
	dirent->raw_dirent.DIR_FileSize = dirent->file_size;

	// 将目录项写回父级目录中
	Dirent *parentDir = dirent->parent_dirent;

	if (parentDir == NULL) {
		// Note: 本目录是root，所以无需将目录大小写回上级目录
		return;
	}

	file_write(parentDir, 0, (unsigned long)&dirent->raw_dirent, dirent->parent_dir_off, DIRENT_SIZE);
}
