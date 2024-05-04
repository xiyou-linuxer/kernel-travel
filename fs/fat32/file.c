#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <fs/filedev.h>
#include <fs/file_time.h>
#include <fs/fs.h>
#include <fs/sysfs.h>
#include <fs/filepnt.h>
#include <fs/buf.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <debug.h>
/**
 * @brief mtx_file是负责维护磁盘file访问互斥性的锁。
 * 每次我们携带dirent指针进入文件系统层对文件进行操作，都需要获取这个锁，以保证对dirent的
 * 读、写、删除等是互斥的。之前曾设计过更细粒度的锁（对每个Dirent加锁），但因太复杂而使用此粗粒度的锁。
 */
mutex_t mtx_file;

/**
 * 管理文件相关事务
 * 文件新建、读写、删除
 */

struct FileDev file_dev_file = {
    .dev_id = 'f',
    .dev_name = "file",
    .dev_read = file_read,
    .dev_write = file_write,
};

/**
 * @brief 获取文件Dirent，但并不跟随软链接。每次get引用计数加1，close引用计数减一
 * @param baseDir 文件或目录寻址时的基地址
 * @param path
 * 文件或目录的路径。如果path是绝对路径，则忽略baseDir；如果path是相对路径（不以 '/'
 * 开头，则是相对于baseDir的）。 path指向的地址要求为内核地址
 * @return 负数表示出错
 */
int get_file_raw(Dirent *baseDir, char *path, Dirent **pfile) {
	mtx_lock_sleep(&mtx_file);

	Dirent *file;
	longEntSet longSet;
	FileSystem *fs;

	extern FileSystem *fatFs;

	if (baseDir) {
		fs = baseDir->file_system;
	} else {
		fs = fatFs;
	}

	if (path == NULL) {
		if (baseDir == NULL) {
			// 一般baseDir都是以类似dirFd的形式解析出来的，所以如果baseDir为NULL，表示归属的fd无效
			warn("get_file_raw: baseDir is NULL and path == NULL, may be the fd "
			     "associate with it is invalid\n");
			mtx_unlock_sleep(&mtx_file);
			return -EBADF;
		} else {
			// 如果path为NULL，则直接返回baseDir（即上层的dirfd解析出的Dirent）
			*pfile = baseDir;
			dget_path(baseDir); // 此时复用一次，也需要加引用
			mtx_unlock_sleep(&mtx_file);
			return 0;
		}
	}

	int r = walk_path(fs, path, baseDir, 0, &file, 0, &longSet);
	if (r < 0) {
		mtx_unlock_sleep(&mtx_file);
		return r;
	} else {
		// 首次打开，更新pointer
		filepnt_init(file);
		mtx_unlock_sleep(&mtx_file);
		*pfile = file;
		return 0;
	}
}

static void file_readlink(Dirent *file, char *buf, int size) {
	ASSERT(IS_LINK(&(file->raw_dirent)));
	ASSERT(file->file_size < size);

	file_read(file, 0, (u64)buf, 0, file->file_size);
}

/**
 * @brief 打开路径为path的文件或目录，返回描述项Dirent。每次get引用计数加1，close引用计数减一
 * @param baseDir 文件或目录寻址时的基地址
 * @param path
 * 文件或目录的路径。如果path是绝对路径，则忽略baseDir；如果path是相对路径（不以 '/'
 * 开头，则是相对于baseDir的）。 path指向的地址要求为内核地址
 * @return -ENOENT 找不到文件；
 *  -ENOTDIR 路径中的某一级不是目录；
 */
int getFile(Dirent *baseDir, char *path, Dirent **pfile) {
	Dirent *file;
	int ret = get_file_raw(baseDir, path, &file);
	if (ret < 0) {
		return ret;
	}

	if (IS_LINK(&(file->raw_dirent))) {
		char buf[MAX_NAME_LEN];
		file_readlink(file, buf, MAX_NAME_LEN);
		file_close(file);
		printk("follow link: %s -> %s\n", path, buf);
		ASSERT(buf[0] == '/');		  // 链接文件的路径必须是绝对路径
		return getFile(NULL, buf, pfile); // 递归调用
	} else {
		*pfile = file;
		return 0;
	}
}

/**
 * @brief 关闭Dirent，使其引用计数减一
 */
void file_close(Dirent *file) {
	mtx_lock_sleep(&mtx_file);
	dput_path(file);
	if (file->is_rm && file->refcnt == 0) {
		warn("file close and is_rm is set, rm file %s\n", file->name);
		rm_unused_file(file);
	}
	mtx_unlock_sleep(&mtx_file);
}

// 补充两个不获取锁的_file_read_nolock和_file_write_nolock，以供连续写入时使用

/**
 * @brief 将文件 entry 的 off 偏移往后长度为 n 的内容读到 dst 中。如果 user
 * 为真，则为用户地址，否则为内核地址。
 * @brief 如果遇到文件结束，返回0
 * @return 返回读取文件的字节数
 */
int file_read(struct Dirent *file, int user, u64 dst, uint off, uint n) {
	mtx_lock_sleep(&mtx_file);

	printk("read from file %s: off = %d, n = %d\n", file->name, off, n);
	if (off >= file->file_size) {
		// 起始地址超出文件的最大范围，遇到文件结束，返回0
		mtx_unlock_sleep(&mtx_file);
		return 0;
	} else if (off + n > file->file_size) {
		printk("read too much. shorten read length from %d to %d!\n", n,
		     file->file_size - off);
		n = file->file_size - off;
	}
	if (n == 0) {
		mtx_unlock_sleep(&mtx_file);
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
	clusterRead(file->file_system, clus, offset, (void *)dst, MIN(n, clusSize - offset), user);
	len += MIN(n, clusSize - offset);

	// 之后的块
	clusIndex += 1;
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus = filepnt_getclusbyno(file, clusIndex);
		clusterRead(file->file_system, clus, 0, (void *)(dst + len), MIN(clusSize, n - len),
			    user);
		len += MIN(clusSize, n - len);
	}

	mtx_unlock_sleep(&mtx_file);

	return n;
}

/**
 * @brief 扩充文件到新的大小
 */
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
 * @brief 将 src 写入文件 entry 的 off 偏移往后长度为 n 的内容。如果 user
 * 为真，则为用户地址，否则为内核地址。
 * @note 允许写入的内容超出文件，此时将扩展文件
 * @return 返回写入文件的字节数
 */
int file_write(struct Dirent *file, int user, u64 src, uint off, uint n) {
	mtx_lock_sleep(&mtx_file);

	printk("write file: %s\n", file->name);
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
	clusterWrite(file->file_system, clus, offset, (void *)src, MIN(n, clusSize - offset), user);
	len += MIN(n, clusSize - offset);

	// 之后的块
	clusIndex += 1;
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus = filepnt_getclusbyno(file, clusIndex);
		clusterWrite(file->file_system, clus, 0, (void *)(src + len),
			     MIN(clusSize, n - len), user);
		len += MIN(clusSize, n - len);
	}

	mtx_unlock_sleep(&mtx_file);
	return n;
}

/**
 * @brief 缩小文件到指定的大小，并释放之前的簇
 * 在ftruncate之前首先需要将文件最后一个簇的多余部分清零
 */
void file_shrink(Dirent *file, u64 newsize) {
	mtx_lock_sleep(&mtx_file);

	ASSERT(file != NULL);
	ASSERT(file->file_size >= newsize);

	u32 oldsize = file->file_size;
	u32 clusSize = CLUS_SIZE(file->file_system);

	// 1. 清空文件最后一个簇的剩余内容
	char buf[1024];
	memset(buf, 0, sizeof(buf));
	if (oldsize % PAGE_SIZE != 0) {
		for (int i = newsize; i < PGROUNDUP(newsize); i += sizeof(buf)) {
			file_write(file, 0, (u64)buf, i, MIN(sizeof(buf), PGROUNDUP(newsize) - i));
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
	mtx_unlock_sleep(&mtx_file);
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
		warn("unknown file type: %x\n", file->type);
		mode |= __S_IFREG; // 暂时置为REGULAR FILE
	}

	// 打印文件的类型
	if (S_ISREG(mode)) {
		printk("file type: regular file\n");
	} else if (S_ISDIR(mode)) {
		printk("file type: directory\n");
	} else if (S_ISCHR(mode)) {
		printk("file type: character device\n");
	} else if (S_ISBLK(mode)) {
		printk("file type: block device\n");
	} else {
		printk("file type: unknown\n");
	}

	return mode;
}

/**
 * @brief 获取文件状态信息
 * @param kstat 内核态指针，指向文件信息结构体
 */
void fileStat(struct Dirent *file, struct kstat *pKStat) {
	mtx_lock_sleep(&mtx_file);

	memset(pKStat, 0, sizeof(struct kstat));
	// P262 Linux-Unix系统编程手册
	pKStat->st_dev = file->file_system->deviceNumber;

	// 并未实现inode，使用Dirent编号替代inode编号
	pKStat->st_ino = ((u64)file - 0x80000000ul);

	pKStat->st_mode = get_file_mode(file);
	pKStat->st_nlink = 1; // 文件的链接数，无链接时为1
	pKStat->st_uid = 0;
	pKStat->st_gid = 0;
	pKStat->st_rdev = 0;
	pKStat->st_size = file->file_size;
	pKStat->st_blksize = CLUS_SIZE(file->file_system);
	pKStat->st_blocks = ROUNDUP(file->file_size, pKStat->st_blksize);

	// 时间相关
	file_get_timestamp(file, pKStat);

	mtx_unlock_sleep(&mtx_file);
}

// 检查文件的用户权限，暂时忽略flags
int faccessat(Dirent *dir, char *path, int mode, int flags) {
	Dirent *file;
	int ret = getFile(dir, path, &file);
	if (ret < 0) {
		printk("faccessat: file %s not exist\n", path);
		// 文件不存在肯定不满足任何一项条件
		return ret;
	}

	// 文件默认有RWX三种权限，因为FAT32没有实现文件访问权限机制
	// 因此确认文件存在后，不继续检查，直接返回0
	file_close(file);
	return 0;
}

/**
 * @brief 同步文件系统到磁盘
 */
void fs_sync() {
	mtx_lock_sleep(&mtx_file);
	bufSync();
	mtx_unlock_sleep(&mtx_file);
}
