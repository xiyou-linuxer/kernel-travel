#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <fs/file_time.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/buf.h>
#include <fs/fd.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <debug.h>
#include <xkernel/memory.h>
int get_file_raw(Dirent *baseDir, char *path, Dirent **pfile) {
	//lock_acquire(&mtx_file);
	
	Dirent *file;
	longEntSet longSet;
	FileSystem *fs;

	if (path == NULL) {
		printk("get_file_raw: baseDir is NULL and path == NULL, may be the fd associate with it is invalid\n");
	}
	int r = walk_path(fs, path, baseDir, 0, &file, 0, &longSet);
	printk("get_file_raw\n");
	if (r < 0) {
		//lock_release(&mtx_file);
		return r;
	} else {
		// 首次打开，更新pointer
		filepnt_init(file);
		//lock_release(&mtx_file);
		*pfile = file;
		return 0;
	}
}
int getFile(Dirent *baseDir, char *path, Dirent **pfile) {
	Dirent *file;
	int ret = get_file_raw(baseDir, path, &file);
	if (ret < 0) {
		return ret;
	}
	*pfile = file;
	return 0;
}

void pre_read(struct Dirent *file,unsigned long dst,unsigned int n)
{
	u32 clus = filepnt_getclusbyno(file, 0);
	for (int i = clus; i < clus+n; i++)
	{
		clusterRead(file->file_system, i, 0, (void *)(dst + 4096*i),4096,1);
	}
}
/**
 * @param dst 缓冲区地址
 * @param off 文件指针的偏移
 * @param n 要读取的长度
*/
int Fatfile_read(struct Dirent *file, unsigned long dst, unsigned int off, unsigned int n) {
	lock_acquire(&mtx_file);
	if (off >= file->file_size) {
		// 起始地址超出文件的最大范围，遇到文件结束，返回0
		lock_release(&mtx_file);
		return 0;
	} else if (off + n > file->file_size) {
		n = file->file_size - off;
	}
	if (n == 0) {
		lock_release(&mtx_file);
		return 0;
	}	
	u64 start = off, end = off + n - 1;
	u32 clusSize = file->file_system->superBlock.bytes_per_clus;
	u32 offset = off % clusSize;

	// 寻找第一个cluster
	u32 clusIndex = start / clusSize;
	u32 clus = filepnt_getclusbyno(file, clusIndex);
	u32 len = 0; // 累计读取的字节数
	// 读取第一块
	clusterRead(file->file_system, clus, offset, (void *)dst, MIN(n, clusSize - offset), 0);
	len += MIN(n, clusSize - offset);
	// 之后的块
	clusIndex += 1;
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus ++;
		clusterRead(file->file_system, clus, 0, (void *)(dst + len), MIN(clusSize, n - len),0);
		len += MIN(clusSize, n - len);
	}
	lock_release(&mtx_file);
	return n;
}

void file_extend(struct Dirent *file, int newSize) {
	ASSERT(file->file_size < newSize);

	u32 oldSize = file->file_size;
	file->file_size = newSize;
	FileSystem *fs = file->file_system;

	u32 clusSize = CLUS_SIZE(file->file_system);
	u32 clusIndex = 0;
	u32 clus;

	// 1. 处理空文件的情况
	if (file->first_clus != 0) {
		clus = file->first_clus;
	} else {
		file->first_clus = clus = clusterAlloc(file->file_system, 0);
		filepnt_setval(&file->pointer, 0, clus);
		oldSize = 1; // 扩充文件
	}

	// 2. 计算最后一个簇的簇号和index
	u32 old_clusters = (oldSize + clusSize - 1) / clusSize;
	clus = filepnt_getclusbyno(file, old_clusters-1); // 最后簇的簇号
	clusIndex = old_clusters - 1;

	// 3. 分配簇，并更新pointer簇号表
	while (newSize > (clusIndex + 1) * clusSize) {
		clus = clusterAlloc(fs, clus);
		clusIndex += 1;
		// 同时将增加的簇数加入到簇号指针表中
		filepnt_setval(&file->pointer, clusIndex, clus);
	}

	// 4. 写回目录项
	sync_dirent_rawdata_back(file);
}

/**
 * @param dst 缓冲区地址
 * @param off 文件指针的偏移
 * @param n 要写入的长度
*/
int Fatfile_write(struct Dirent *file, unsigned long src, unsigned int off, unsigned int n) {
	lock_acquire(&mtx_file);
	ASSERT(n != 0);

	// Note: 支持off在任意位置的写入（允许超过file->size），[file->size, off)的部分将被填充为0
	if (off + n > file->file_size) {
		// 超出文件的最大范围
		// Note: 扩充
		file_extend(file, off + n);
	}

	u64 start = off, end = off + n - 1;
	u32 clusSize = file->file_system->superBlock.bytes_per_clus;
	u32 offset = off % clusSize;

	// 寻找第一个cluster
	u32 clusIndex = start / clusSize;
	u32 clus = filepnt_getclusbyno(file, clusIndex);
	u32 len = 0; // 累计读取的字节数

	// 读取第一块
	clusterWrite(file->file_system, clus, offset, (void *)src, MIN(n, clusSize - offset), 0);
	len += MIN(n, clusSize - offset);

	// 之后的块
	clusIndex += 1;
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus = filepnt_getclusbyno(file, clusIndex);
		clusterWrite(file->file_system, clus, 0, (void *)(src + len),
			     MIN(clusSize, n - len), 0);
		len += MIN(clusSize, n - len);
	}

	lock_release(&mtx_file);
	return n;
}



void file_shrink(Dirent *file, u64 newsize) 
{
	lock_acquire(&mtx_file);

	ASSERT(file != NULL);
	ASSERT(file->file_size >= newsize);

	u32 oldsize = file->file_size;
	u32 clusSize = CLUS_SIZE(file->file_system);

	// 1. 清空文件最后一个簇的剩余内容
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	if (oldsize % PAGE_SIZE != 0) {
		for (int i = newsize; i < PGROUNDUP(newsize); i += sizeof(buf)) {
			
			Fatfile_write(file, (u64)buf, i, MIN(sizeof(buf), PGROUNDUP(newsize) - i));
		}
	}

	// 2. 释放后面的簇
	u32 new_clusters = (newsize + clusSize - 1) / clusSize;
	u32 old_clusters = (oldsize + clusSize - 1) / clusSize;
	if (new_clusters < old_clusters) {
		// 获取新文件大小之后的第一个簇
		u32 last_clus = filepnt_getclusbyno(file, new_clusters);
		if (new_clusters != 0) {
			u32 prev_clus = filepnt_getclusbyno(file, new_clusters-1);
			fatWrite(file->file_system, prev_clus, FAT32_EOF); // 标识最后一个簇
		}
		clus_sequence_free(file->file_system, last_clus);
		for (int i = new_clusters; i < old_clusters; i++) {
			// 清空簇号表的对应位置
			filepnt_setval(&file->pointer, i, 0);
		}
	}

	// 2. 缩小文件
	file->file_size = newsize;
	if (newsize == 0) {
		file->first_clus = 0;
	}

	// 3. 写回
	sync_dirent_rawdata_back(file);
	lock_release(&mtx_file);
}

static mode_t get_file_mode(struct Dirent *file) {
	// 默认给予RWX权限
	mode_t mode = file->mode;

	if (strncmp(file->name, "ssh_host_", 9) == 0) {
		// ssh_host_*文件的权限为600
		mode = 0600;
	} else if (strncmp(file->name, "empty", 5) == 0) {
		mode = 0711;
	}

	if (file->type == DIRENT_DIR) {
		mode |= __S_IFDIR;
	} else if (file->type == DIRENT_FILE) {
		mode |= __S_IFREG;
	} else if (file->type == DIRENT_CHARDEV) {
		// 我们默认设备总是字符设备
		mode |= __S_IFCHR;
	} else if (file->type == DIRENT_BLKDEV) {
		mode |= __S_IFBLK;
	} else {
		//printk("unknown file type: %x\n", file->type);
		mode |= __S_IFREG; // 暂时置为REGULAR FILE
	}

	return mode;
}

/**
 * @brief 获取文件状态信息
 * @param kstat 内核态指针，指向文件信息结构体
 */
void fileStat(struct Dirent *file, struct kstat *pKStat) {
	//mtx_lock_sleep(&mtx_file);

	memset(pKStat, 0, sizeof(struct kstat));
	// P262 Linux-Unix系统编程手册
	pKStat->st_dev = file->file_system->deviceNumber;

	// 并未实现inode，使用Dirent编号替代inode编号
	pKStat->st_ino = ((u64)file - 0x90000000ul);

	pKStat->st_mode = get_file_mode(file);
	pKStat->st_nlink = 1; // 文件的链接数，无链接时为1
	pKStat->st_uid = 0;
	pKStat->st_gid = 0;
	pKStat->st_rdev = 0;
	pKStat->st_size = file->file_size;
	pKStat->st_blksize = CLUS_SIZE(file->file_system);
	pKStat->st_blocks = ROUNDUP(file->file_size, pKStat->st_blksize);

	// 时间相关
	pKStat->st_atime_sec = file->time.st_atime_sec;
	pKStat->st_atime_nsec = file->time.st_atime_nsec;
	pKStat->st_mtime_sec = file->time.st_mtime_sec;
	pKStat->st_mtime_nsec = file->time.st_mtime_nsec;
	pKStat->st_ctime_sec = file->time.st_ctime_sec;
	pKStat->st_ctime_nsec = file->time.st_ctime_nsec;

} 

/**
 * @brief 需要保证传入的file->refcnt == 0
 */

int rm_unused_file(struct Dirent *file) {
	ASSERT(file->refcnt == 0);
	char linked_file_path[MAX_NAME_LEN];
	int cnt = get_entry_count_by_name(file->name);
	char data = FAT32_INVALID_ENTRY;


	// 2. 断开父子关系
	ASSERT(file->parent_dirent != NULL); // 不处理根目录的情况
	// 先递归删除子Dirent
	if (file->type == DIRENT_DIR) {
		struct list_elem *tmp = file->child_list.head.next;
		while (tmp!=&file->child_list.tail) {
			struct list_elem *next = tmp->next;
			Dirent *file_child = elem2entry(Dirent,dirent_tag,tmp);
			rmfile(file_child);
			tmp = next;
		}
	}

	list_remove(&file->dirent_tag);// 从父亲的子Dirent列表删除
	// 3. 释放其占用的Cluster
	file_shrink(file, 0);

	// 4. 清空目录项
	for (int i = 0; i < cnt; i++) {
		int ret = Fatfile_write(file->parent_dirent, (unsigned long)&data, file->parent_dir_off - i * DIR_SIZE, 1) < 0;
		if (ret != 0)
		{
			/* code */
		}
	}
	dirent_dealloc(file); // 释放目录项
	return 0;
}

/**
 * @brief 删除文件。支持递归删除文件夹
 */
int rmfile(Dirent *file) 
{
	//若引用计数大于则报错返回
	if (file->refcnt > 1) {
		printk("File is in use\n");
		return -1;
	}
	//return 0;
	return rm_unused_file(file);
}