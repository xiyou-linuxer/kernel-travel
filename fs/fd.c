#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/fd.h>
#include <fs/fs.h>
#include <fs/filepnt.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/sched.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
#include <debug.h>

struct fd file_table[MAX_FILE_OPEN];//全局文件打开数组

/* 从文件表file_table中获取一个空闲位,成功返回下标,失败返回-1 */
int32_t get_free_slot_in_global(void)
{
	uint32_t fd_idx = 3;
	while (fd_idx < MAX_FILE_OPEN)
	{
		if (file_table[fd_idx].dirent == NULL)
		{
			break;
		}
		fd_idx++;
	}
	if (fd_idx == MAX_FILE_OPEN)
	{
		//printk("exceed max open files\n");
		return -1;
	}
	return fd_idx;
}

/* 将全局描述符下标安装到进程或线程自己的文件描述符数组fd_table中,
 * 成功返回下标,失败返回-1 */
int32_t pcb_fd_install(int32_t globa_fd_idx)
{
	struct task_struct *cur = running_thread();
	uint8_t local_fd_idx = 3; // 跨过stdin,stdout,stderr
	while (local_fd_idx < MAX_FILES_OPEN_PER_PROC)
	{
		if (cur->fd_table[local_fd_idx] == -1)
		{ // -1表示free_slot,可用
			cur->fd_table[local_fd_idx] = globa_fd_idx;
			file_table[globa_fd_idx].refcnt++;
			break;
		}
		local_fd_idx++;
	}
	if (local_fd_idx == MAX_FILES_OPEN_PER_PROC)
	{
		//printk("exceed max open files_per_proc\n");
		return 0;
	}
	return local_fd_idx;
}

/* 将文件描述符转化为文件表的下标 */
uint32_t fd_local2global(uint32_t local_fd)
{
    struct task_struct *cur = running_thread();
    int global_fd = cur->fd_table[local_fd];
    ASSERT(global_fd >= 0 && global_fd < MAX_FILE_OPEN);
    return (uint32_t)global_fd;
}

int file_open(Dirent* file, int flag, mode_t mode)
{
	/*若文件在打开的列表则直接将引用计数加一后返回*/
	for (int i = 0; i < MAX_FILE_OPEN; i++)
	{
		if (file_table[i].dirent == file)
		{
			file_table[i].refcnt += 1;
			return pcb_fd_install(i);
		}
	}
	int fd_idx = get_free_slot_in_global();
	if (fd_idx == -1)
	{
		printk("exceed max open files\n");
		return -1;
	}	
	file_table[fd_idx].dirent = file;
	file_table[fd_idx].dirent->refcnt += 1;
	file_table[fd_idx].offset = 0; // 每次打开文件,要将offset还原为0,即让文件内的指针指向开头
	file_table[fd_idx].flags = flag;//文件打开的标志位
	file_table[fd_idx].stat.st_mode = mode;
	file_table[fd_idx].type = dev_file;
	file_table[fd_idx].refcnt = 1;
	lock_init(&file_table[fd_idx].lock);
	return pcb_fd_install(fd_idx);
}

int file_create(struct Dirent *baseDir, char *path, int flag,mode_t mode)
{
	Dirent *file;
	createFile(baseDir, path, &file);//创建文件
	int fd_idx = get_free_slot_in_global();
	if (fd_idx == -1)
	{
		printk("exceed max open files\n");
	}
	file_table[fd_idx].dirent = file;
	file_table[fd_idx].offset =0;
	file_table[fd_idx].flags = flag;
	file_table[fd_idx].stat.st_mode = mode;
	file_table[fd_idx].type = dev_file;
	lock_init(&file_table[fd_idx].lock);//初始化文件锁
	return pcb_fd_install(fd_idx);
}

int file_close(struct fd *_fd)
{
	if (_fd->dirent == NULL)
	{
		return -1;
	}
	_fd->refcnt -= 1;
	if (_fd->refcnt == 0)//fd的引用计数为0则释放这个fd
	{
		_fd->dirent->refcnt -=1;
		_fd->dirent= NULL;
		_fd->offset = 0;
		_fd->offset = -1;
		_fd->type = -1;
	}
	return 0;
}

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
		int ret = file_write(file->parent_dirent, 0, (unsigned long)&data, file->parent_dir_off - i * DIR_SIZE, 1) < 0;
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
int rmfile(struct Dirent *file) 
{
	//若引用计数大于则报错返回
	if (file->refcnt > 1) {
		printk("File is in use\n");
		return -1;
	}
	//return 0;
	return rm_unused_file(file);
}

/**
 * @brief 需要保证传入的file->refcnt == 0
 */


/**
 * @brief 撤销链接。即删除(链接)文件
 */
int unlinkat(struct Dirent *dir, char *path) {
	lock_acquire(&mtx_file);

	Dirent *file;
	int ret;
	if ((ret = getFile(dir, path, &file)) < 0) {
		printk("file %s not found!\n", path);
		lock_release(&mtx_file);
		return ret;
	}
	ret = rmfile(file);
	lock_release(&mtx_file);
	return ret;
}

int filename2path(Dirent *file,char *newpath)
{
    Dirent *dir = file;
    char buf[MAX_PATH_LEN];
	strcpy(buf,file->name);
    while (strcmp(dir->parent_dirent->name,"/"))
    {
        strcat(buf,dir->parent_dirent->name);
        
        dir = dir->parent_dirent;
    }
    strcpy(newpath, "/");
    while (strchrs(buf,'/')!=0)
    {
        strcat(newpath,strrchr(buf,'/'));
		memset(strrchr(buf,'/'),0,MAX_NAME_LEN);
    }
    strcat(newpath,buf);
    return 1;
}

/** 
 * 返回根据文件的页号返回虚拟地址
 *  @param 文件fd
 *  @param 文件映射的起始页号
 *  @param 文件映射结束的页号
*/
void fd_mapping(int fd, int start_page, int end_page,unsigned long* v_addr)
{
	int _fd = fd_local2global(fd);
	Dirent *file = file_table[_fd].dirent;
	filepnt_init(file);
	char buf[512];
	pre_read(file,buf,file->file_size/4096+1);//将文件预读到内存中
	file_read(file, 0, (unsigned long)buf, 0, file->file_size);
	int indx = start_page;
	int count = (end_page - start_page + 1)*8;
	while (indx<=end_page)
	{
		u32 page_num = filepnt_getclusbyno(file, indx);
		unsigned long secno = clusterSec(fatFs, page_num);
		int i = 0;
		while (i<8)
		{
			u64 group = secno & BGROUP_MASK;
			struct list* list = &bufferGroups[group].list;
			struct list_elem *node = list->head.next;
			while (node != &list->tail)
			{
				Buffer* buf = elem2entry(Buffer, Buffer_node, node);
				if (buf->blockno == secno)
				{
					//printk("buf:%s num:%d\n",buf->data,indx*8+i);
					v_addr[indx*8+i] = buf->data;
					break;
					
				}
				node = node->next;
			}
			i++;
			secno++;
		}
		indx ++;
	}
	printk("aaa");
}