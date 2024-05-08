#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/file.h>
#include <fs/file_time.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/buf.h>
#include <linux/stdio.h>
#include <linux/string.h>
#include <debug.h>

int get_file_raw(Dirent *baseDir, char *path, Dirent **pfile) {
	//lock_acquire(&mtx_file);
	
	Dirent *file;
	longEntSet longSet;
	FileSystem *fs;

	//FileSystem *fatFs;

	/*if (baseDir) {
		fs = baseDir->file_system;
	} else {
		fs = fatFs;
	}*/

	if (path == NULL) {
		/*if (baseDir == NULL) {
			// 一般baseDir都是以类似dirFd的形式解析出来的，所以如果baseDir为NULL，表示归属的fd无效
			printk("get_file_raw: baseDir is NULL and path == NULL, may be the fd associate with it is invalid\n");
			//lock_release(&mtx_file);
			return -1;
		} else {
			// 如果path为NULL，则直接返回baseDir（即上层的dirfd解析出的Dirent）
			*pfile = baseDir;
			dget_path(baseDir); // 此时复用一次，也需要加引用
			//lock_release(&mtx_file);
			return 0;
		}*/
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
	/*if (IS_LINK(&(file->raw_dirent))) {
		char buf[MAX_NAME_LEN];
		file_readlink(file, buf, MAX_NAME_LEN);
		file_close(file);
		printk("follow link: %s -> %s\n", path, buf);
		ASSERT(buf[0] == '/');		  // 链接文件的路径必须是绝对路径
		return getFile(NULL, buf, pfile); // 递归调用
	} else {
		*pfile = file;
		return 0;
	}*/
}

int file_read(struct Dirent *file, int user, unsigned long dst, unsigned int off, unsigned int n) {
	lock_acquire(&mtx_file);

	printk("read from file %s: off = %d, n = %d\n", file->name, off, n);
	if (off >= file->file_size) {
		// 起始地址超出文件的最大范围，遇到文件结束，返回0
		lock_release(&mtx_file);
		return 0;
	} else if (off + n > file->file_size) {
		printk("read too much. shorten read length from %d to %d!\n", n,
		     file->file_size - off);
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
	clusterRead(file->file_system, clus, offset, (void *)dst, MIN(n, clusSize - offset), user);
	len += MIN(n, clusSize - offset);

	// 之后的块
	clusIndex += 1;
	printk("%d",clusIndex * clusSize);
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus = filepnt_getclusbyno(file, clusIndex);
		clusterRead(file->file_system, clus, 0, (void *)(dst + len), MIN(clusSize, n - len),
			    user);
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


int file_write(struct Dirent *file, int user, unsigned long src, unsigned int off, unsigned int n) {
	lock_acquire(&mtx_file);

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

	lock_release(&mtx_file);
	return n;
}
