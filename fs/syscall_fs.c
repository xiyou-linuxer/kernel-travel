#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
#include <xkernel/console.h>
#include <xkernel/string.h>
#include <xkernel/ioqueue.h>
#include <xkernel/memory.h>
#include <fs/path.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <debug.h>
#include <asm/syscall.h>
#include <xkernel/wait.h>
/*本文件用于实现文件的syscall*/
int sys_open(const char *pathname, int flags, mode_t mode)
{
	if (strcmp(pathname,".")==0)
	{
		return file_open(fatFs->root,flags,mode);
	}
	path_resolution(pathname);
	Dirent *file;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)pathname);
	if (flags == 65)
	{
		flags = 6;
	}
	/* 先检查是否将全部的路径遍历 */
	file = search_file(pathname,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		// 说明并没有访问到全部的路径,某个中间目录是不存在的
		//printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
		return -1;
	}
	/* 若是在最后一个路径上没找到,并且并不是要创建文件,直接返回-1 */
	if ((file == NULL) && !(flags & O_CREATE))
	{
		return -1;
	}
	else if ((file != NULL) && flags & O_CREATE)
	{ // 若要创建的文件已存在
		//printk("%s has already exist!\n", pathname);
		return file_open(file, flags ,mode);
	}
	
	switch (flags & O_CREATE)
	{
	case O_CREATE:
		fd = file_create(searched_record.parent_dir, pathname, flags, mode);
		break;
	default:
		/* 其余情况均为打开已存在文件:
		 * O_RDONLY,O_WRONLY,O_RDWR */
		fd = file_open(file, flags ,mode);
		int _fd = fd_local2global(fd);
		file_table[_fd].offset = 0;
	}

	/* 此fd是指任务pcb->fd_table数组中的元素下标,
	 * 并不是指全局file_table中的下标 */
	return fd;
}

int sys_write(int fd, const void *buf, unsigned int count)
{
	int _fd = fd_local2global(fd);
	if (fd < 0)
	{
		printk("sys_write %d: fd error\n",fd);
		return -1;
	}
	if (_fd == STDOUT || fd == STDOUT)
	{
		/* 标准输出有可能被重定向为管道缓冲区, 因此要判断 */
		if (is_pipe(fd))
		{
			return pipe_write(fd, buf, count);
		}
		else
		{
			char tmp_buf[1024] = {0};
			memcpy(tmp_buf, buf, count);
			console_put_str(tmp_buf);
			return count;
		}
	}
	else if (is_pipe(fd))
	{ // 若是管道就调用管道的方法
		return pipe_write(fd, buf, count);
	}
	else
	{
		Dirent *wr_file = file_table[_fd].dirent;
		filepnt_init(wr_file);
		if (file_table[_fd].flags & O_WRONLY || file_table[_fd].flags & O_RDWR)
		{
			unsigned bytes_written = file_write(wr_file, 0, buf,file_table[_fd].offset,count);
			file_table[_fd].offset += bytes_written;
			return bytes_written;
		}
		else
		{
			/*console_put_str("sys_write: not allowed to write file without flag O_RDWR or O_WRONLY\n");*/
			return -1;
		}
	}
}

/* 从文件描述符fd指向的文件中读取count个字节到buf,若成功则返回读出的字节数,到文件尾则返回-1 */
int sys_read(int fd, void *buf, unsigned int count)
{
	int32_t ret = -1;
	uint32_t global_fd = 0;
	if (fd < 0 || fd == STDOUT || fd == STDERR)
	{
		printk("sys_read: fd error\n");
	}
	else if (fd == STDIN)
	{
		/* 标准输入有可能被重定向为管道缓冲区, 因此要判断 */
		if (is_pipe(fd))
		{
			ret = pipe_read(fd, buf, count);
		}
    }
    else if (is_pipe(fd))
    { // 若是管道就调用管道的方法 
        ret = pipe_read(fd, buf, count);
		if (ret == -1)
		{
			return 0;
		}
	}
	else
	{
		global_fd = fd_local2global(fd);
		filepnt_init(file_table[global_fd].dirent);
		ret = file_read(file_table[global_fd].dirent, 0, buf,file_table[global_fd].offset, count);
		file_table[global_fd].offset += ret;
	}
	return count;
}

/* 成功关闭文件返回0,失败返回-1 */
int sys_close(int fd)
{
	int pid = running_thread()->pid;
	//printk("fd:%d pid:%d",fd,p->pid);
	int32_t ret = -1; // 返回值默认为-1,即失败
	if (fd > 2)
	{
		uint32_t global_fd = fd_local2global(fd);
		if (is_pipe(fd))
		{
			if (pipe_table[pid][0] == fd)
			{
				pipe_table[pid][0] = 0;
			}else
			{
				pipe_table[pid][1] = 0;
			}
			if (pipe_table[pid][0]==0&&pipe_table[pid][1] == 0)
			{
				file_table[global_fd].pipe.flag = 0;
			}
			// 如果此管道上的描述符都被关闭,释放管道的环形缓冲区 
			if (--file_table[global_fd].offset == 0)
			{
				memset(&file_table[global_fd].pipe,0,sizeof(struct ioqueue));
			}
			ret = 0;
        }
        else
        {
			ret=file_close(&file_table[global_fd]);
        }
        running_thread()->fd_table[fd] = -1; // 使该文件描述符位可用
    }
    return ret;
}

int sys_mkdir(char* path, int mode) 
{
	path_resolution(path);
	struct path_search_record searched_record;
	Dirent * file = search_file(path,&searched_record);
	if (file != NULL)
	{
		return 0;
	}
	makeDirAt(NULL, path, mode);
	return 0;
}

char * sys_getcwd(char *buf, int size)
{
	void* a = running_thread()->cwd;
	memcpy(buf ,a ,size);
	return buf;
}

int sys_chdir(char* path)
{
	struct task_struct* pthread = running_thread();
	path_resolution(path);
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	Dirent *cwd_dirent = search_file(path,&searched_record);
	if (cwd_dirent == NULL)//如果没有这个路径
	{
		return -1;
	}else if (cwd_dirent->type != DIRENT_DIR)//如果这个路径不是目录类型
	{
		printk("It's file\n");
	}
	strcpy(pthread->cwd, path);
	pthread->cwd_dirent = cwd_dirent;
	return 0;
} 


int sys_unlink(char *pathname)
{
	Dirent *file;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));
	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)pathname);

	/* 先检查是否将全部的路径遍历 */
	file = search_file(pathname,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		// 说明并没有访问到全部的路径,某个中间目录是不存在的
		//printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
		return -1;
	}
	/*if (file->type == DIRENT_DIR)//不能直接删除目录
	{
		printk("It's direct\n");
		return -1;
	}*/
	int ret = rmfile(file);
	return ret;
}


int sys_fstat(int fd,struct kstat* stat)
{
	int ret = 0;
	uint32_t global_fd = fd_local2global(fd);
	fileStat(file_table[global_fd].dirent,stat);
	if (stat==NULL)
	{
		ret = -1;
	}
	return ret;
}

int sys_lseek(int fd, int offset, uint8_t whence)
{
    if (fd < 0)
    {
        printk("sys_lseek: fd error\n");
        return -1;
    }
    ASSERT(whence > 0 && whence < 4);
    uint32_t _fd = fd_local2global(fd);
    struct fd *pf = &file_table[_fd];
    int32_t new_pos = 0; // 新的偏移量必须位于文件大小之内
    int32_t file_size = (int32_t)pf->dirent->file_size;
    switch (whence)
    {
    /* SEEK_SET 新的读写位置是相对于文件开头再增加offset个位移量 */
    case SEEK_SET:
        new_pos = offset;
        break;

    /* SEEK_CUR 新的读写位置是相对于当前的位置增加offset个位移量 */
    case SEEK_CUR: // offse可正可负
        new_pos = (int32_t)pf->offset + offset;
        break;

    /* SEEK_END 新的读写位置是相对于文件尺寸再增加offset个位移量 */
    case SEEK_END: // 此情况下,offset应该为负值
        new_pos = file_size + offset;
    }
    if (new_pos < 0 || new_pos > (file_size - 1))
    {
        return -1;
    }
    pf->offset = new_pos;
    return pf->offset;
}

int sys_dup(int oldfd)
{
	struct task_struct* cur = running_thread();
	unsigned int globa_fd_idx = cur->fd_table[oldfd];
	return pcb_fd_install(globa_fd_idx);//将全局fd下载到新的局部fd当中
}

int sys_dup2(uint32_t old_local_fd, uint32_t new_local_fd)
{
	int ret = -1;
	struct task_struct *cur = running_thread();
	/* 针对恢复标准描述符 */
	if (old_local_fd < 3)
	{
		cur->fd_table[new_local_fd] = old_local_fd;
		ret = old_local_fd;
	}
	else
	{
		if (cur->fd_table[new_local_fd]!=-1)
		{
			file_close(cur->fd_table[new_local_fd]);
		}
		uint32_t new_global_fd = cur->fd_table[old_local_fd];
		cur->fd_table[new_global_fd] = old_local_fd;
		ret = old_local_fd;
	}
	return ret;
}

int sys_openat(int fd, const char *filename, int flags, mode_t mode)
{
	if (fd == AT_OPEN || filename[0] == '/' || fd == AT_FDCWD)//如果是open系统调用或者文件路径为绝对路径则直接打开
	{
		return sys_open(filename, flags, mode);	
	}else //路径为fd的路径
	{
		char buf[MAX_PATH_LEN];
		int global_fd = fd_local2global(fd);
		Dirent *file = file_table[global_fd].dirent;
		filename2path(file,buf);
		strcat(buf,"/");
		strcat(buf,filename);
		return sys_open(buf,flags,mode);
	}
}

int sys_mkdirat(int dirfd, const char *path, mode_t mode)
{
	return sys_mkdir(path, mode);
}

int sys_unlinkat(int dirfd, char *path, unsigned int flags)
{
	return sys_unlink(path);
}

int sys_mount(const char *special, const char *dir, const char *fstype, unsigned long flags, const void *data)
{
	path_resolution(dir);
	return mount_fs(special,dir);
}

int sys_umount(const char* special) 
{
	path_resolution(special);
	return umount_fs(special);
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
	//pre_read(file,buf,file->file_size/4096+1);//将文件预读到内存中
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
	return;
}
int sys_statx(int dirfd, const char *pathname, int flags, unsigned int mask, struct statx *buf)
{
	int fd;
	if (pathname[0] == '/' || dirfd == AT_FDCWD)//如果是open系统调用或者文件路径为绝对路径则直接打开
	{
		fd = sys_open(pathname, O_CREATE | O_RDWR, 066);
	}else{
		char buf[MAX_PATH_LEN];
		int global_fd = fd_local2global(dirfd);
		Dirent *file = file_table[global_fd].dirent;
		filename2path(file,buf);
		strcat(buf,"/");
		strcat(buf,pathname);
		fd = sys_open(buf,flags,660);
	}
	int ret = 0;
	struct kstat stat;
	uint32_t global_fd = fd_local2global(fd);
	fileStat(file_table[global_fd].dirent,&stat);
	buf->stx_size = stat.st_size;
	return ret;
}

/* 创建管道,成功返回0,失败返回-1 */
int32_t sys_pipe(int32_t pipefd[2])
{
    int32_t global_fd = get_free_slot_in_global();
    /* 初始化环形缓冲区 */
    ioqueue_init(&file_table[global_fd].pipe);
    /* 将fd_flag复用为管道标志 */
    file_table[global_fd].type = dev_pipe;
    /* 将fd_pos复用为管道打开数 */
    file_table[global_fd].offset = 2;
    pipefd[0] = pcb_fd_install(global_fd);
    pipefd[1] = pcb_fd_install(global_fd);
    int pip = running_thread()->pid;
    pipe_table[pip][0] = pipefd[0];
    pipe_table[pip][1] = pipefd[1];
    //printk("0:%d,1:%d\n", pipe_table[pip][0], pipe_table[pip][1]);
    return 0;
}

int sys_getdents(int fd, struct linux_dirent64 * buf, size_t len)
{
	int global_fd = fd_local2global(fd);
	Dirent *file = file_table[global_fd].dirent;
	strcpy(buf->d_name,file->name);
	buf->d_off = file_table[global_fd].offset;
	return len;
}
