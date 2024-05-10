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
#include <linux/memory.h>
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

int file_read(struct Dirent *file, int user, unsigned long dst, unsigned int off, unsigned int n) {
	lock_acquire(&mtx_file);

	printk("read from file %s: off = %d, n = %d\n", file->name, off, n);
	if (off >= file->file_size) {
		// 起始地址超出文件的最大范围，遇到文件结束，返回0
		lock_release(&mtx_file);
		return 0;
	} else if (off + n > file->file_size) {
		printk("read too much. shorten read length from %d to %d!\n", n,file->file_size - off);
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
	for (; end >= clusIndex * clusSize; clusIndex++) {
		clus ++;
		clusterRead(file->file_system, clus, 0, (void *)(dst + len), MIN(clusSize, n - len),user);
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

/*搜索文件的临时接口*/
Dirent* search_file(Dirent* parent,char *name)
{
	Dirent *file;
	struct list_elem* dir_node = parent->child_list.head.next;
	while (dir_node!=&parent->child_list.tail)
	{
		file = elem2entry(struct Dirent,dirent_tag,dir_node);
		if (strcmp(file->name, name) == 0)
		{
			break;
		}
		dir_node = dir_node->next;
	}
	return file;
}

char *path_parse(char *pathname, char *name_store)
{
    if (pathname[0] == '/')
    { // 根目录不需要单独解析
        /* 路径中出现1个或多个连续的字符'/',将这些'/'跳过,如"///a/b" */
        while (*(++pathname) == '/')
            ;
    }

    /* 开始一般的路径解析 */
    while (*pathname != '/' && *pathname != 0)
    {
        *name_store++ = *pathname++;
    }

    if (pathname[0] == 0)
    { // 若路径字符串为空则返回NULL
        return NULL;
    }
    return pathname;
}

/* 返回路径深度,比如/a/b/c,深度为3 */
int32_t path_depth_cnt(char *pathname)
{
    ASSERT(pathname != NULL);
    char *p = pathname;
    char name[MAX_NAME_LEN]; // 用于path_parse的参数做路径解析
    uint32_t depth = 0;

    /* 解析路径,从中拆分出各级名称 */
    p = path_parse(p, name);
    while (name[0])
    {
        depth++;
        memset(name, 0, MAX_NAME_LEN);
        if (p)
        { // 如果p不等于NULL,继续分析路径
            p = path_parse(p, name);
        }
    }
    return depth;
}

/* 搜索文件pathname,若找到则返回其inode号,否则返回-1 */
Dirent* search_file1(const char *pathname, struct path_search_record *searched_record)
{
	/* 如果待查找的是根目录,为避免下面无用的查找,直接返回已知根目录信息 */
	if (!strcmp(pathname, "/") || !strcmp(pathname, "/.") || !strcmp(pathname, "/.."))
	{
		searched_record->parent_dir = fatFs->root;
		searched_record->file_type = DIRENT_DIR;
		searched_record->searched_path[0] = 0; // 搜索路径置空
		return NULL;
	}

	uint32_t path_len = strlen(pathname);
	/* 保证pathname至少是这样的路径/x且小于最大长度 */
	ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
	char *sub_path = (char *)pathname;
	Dirent *parent_dir = fatFs->root;
	Dirent *dir_e;

	/* 记录路径解析出来的各级名称,如路径"/a/b/c",
	* 数组name每次的值分别是"a","b","c" */
	char name[MAX_NAME_LEN] = {0};

	searched_record->parent_dir = parent_dir;
	searched_record->file_type = DIRENT_UNKNOWN;

	sub_path = path_parse(sub_path, name);
	while (name[0])
	{ // 若第一个字符就是结束符,结束循环
		/* 记录查找过的路径,但不能超过searched_path的长度512字节 */
		//ASSERT(strlen(searched_record->searched_path) < 512);

		/* 记录已存在的父目录 */
		strcat(searched_record->searched_path, "/");
		strcat(searched_record->searched_path, name);
		dir_e = search_dir_tree(parent_dir, name);
		/* 在所给的目录中查找文件 */
		if (dir_e != NULL)
		{
			memset(name, 0, MAX_NAME_LEN);
			/* 若sub_path不等于NULL,也就是未结束时继续拆分路径 */
			if (sub_path)
			{
				sub_path = path_parse(sub_path, name);
            }

            if (dir_e->type == DIRENT_DIR )
            { // 如果被打开的是目录
				searched_record->parent_dir = parent_dir;
				continue;
			}
			else if (dir_e->type == DIRENT_FILE)
			{ // 若是普通文件
				searched_record->file_type = DIRENT_FILE;
				return dir_e;
			}
		}
		else
		{
			return NULL;
		}
	}

	/* 执行到此,必然是遍历了完整路径并且查找的文件或目录只有同名目录存在 */
	/* 保存被查找目录的直接父目录 */
	//searched_record->parent_dir = dir_open(cur_part, parent_inode_no);
	searched_record->file_type = DIRENT_DIR;
	return dir_e;
}
