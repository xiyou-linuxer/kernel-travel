#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <xkernel/types.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
#include <xkernel/console.h>
#include <xkernel/string.h>
#include <fs/buf.h>
#include <fs/cluster.h>
#include <fs/dirent.h>
#include <fs/dirent.h>
#include <fs/vfs.h>
#include <fs/filepnt.h>
#include <fs/fs.h>
#include <fs/fd.h>
#include <debug.h>
/*本文件用于实现文件的syscall*/
/*将路径转化为绝对路径，只支持 . 与 .. 开头的路径*/
static void path_resolution(const char *pathname)
{
	char buf[MAX_NAME_LEN];
	struct task_struct *pthread = running_thread();
	if (pathname[0] == '/')//如果已经是绝对路径则直接返回
	{
		return ;
	}else if (pathname[0] == '.')
	{
		strcpy(buf,pthread->cwd);//将当前工作路径复制到buf中
		if (strcmp(buf,"/")==0)
		{
			strcat(buf,strchr(pathname,'/')+1);
		}else
		{
			strcat(buf,strchr(pathname,'/'));//再将除.之后的
		}
	}else if (pathname[0] == '..')
	{
		strcpy(buf,pthread->cwd);
		memset(strrchr(buf,"/"),0,MAX_NAME_LEN);//将最后一个/与他后面的内容清除
		strcat(buf,strchr(pathname,'/'));//再将传入路径除去 .. 后其他内容与buf拼接
	}else
	{
		//如果非以上三种符号开头则默认是目录/文件名，直接与当前工作目录拼接
		strcpy(buf,pthread->cwd);
		if (strcmp(buf,"/")!=0)
		{
			strcat(buf,"/");//当前工作目录不是根目录则先在路径上加上/再与传入的路径拼接
		}
		strcat(buf,pathname);
	}
	strcpy(pathname,buf);
}

int sys_open(const char *pathname, int flags, mode_t mode)
{
	path_resolution(pathname);
	//printk("%s\n",pathname);
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
		printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
		return -1;
	}
	
	/* 若是在最后一个路径上没找到,并且并不是要创建文件,直接返回-1 */
	if ((file == NULL) && !(flags & O_CREATE))
	{
		printk("in path %s, file %s is`t exist\n",searched_record.searched_path,(strrchr(searched_record.searched_path, '/') + 1));
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
		//printk("sys_open");
		fd = file_open(file, flags ,mode);
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
	if (_fd == STDOUT)
	{
		/* 标准输出有可能被重定向为管道缓冲区, 因此要判断 */
		/*if (is_pipe(fd))
		{
			return pipe_write(fd, buf, count);
		}
		else*/
		//{
			char tmp_buf[1024] = {0};
			memcpy(tmp_buf, buf, count);
			console_put_str(tmp_buf);
			return count;
		//}
	}
	/*else if (is_pipe(fd))
	{ // 若是管道就调用管道的方法
		return pipe_write(fd, buf, count);
	}*/
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
			console_put_str("sys_write: not allowed to write file without flag O_RDWR or O_WRONLY\n");
			return -1;
		}
	}
}

/* 从文件描述符fd指向的文件中读取count个字节到buf,若成功则返回读出的字节数,到文件尾则返回-1 */
int sys_read(int fd, void *buf, unsigned int count)
{
	//ASSERT(buf != NULL);
	int32_t ret = -1;
	uint32_t global_fd = 0;
	if (fd < 0 || fd == STDOUT || fd == STDERR)
	{
		printk("sys_read: fd error\n");
	}
	else if (fd == STDIN)
	{
		/* 标准输入有可能被重定向为管道缓冲区, 因此要判断 */
		/*if (is_pipe(fd))
		{
			ret = pipe_read(fd, buf, count);
		}
        else*/
        //{
            /*char *buffer = buf;
            uint32_t bytes_read = 0;
            while (bytes_read < count)
            {
                *buffer = ioq_getchar(&kbd_buf);
                bytes_read++;
                buffer++;
            }
            ret = (bytes_read == 0 ? -1 : (int32_t)bytes_read);
        //}
    }
    else if (is_pipe(fd))
    { // 若是管道就调用管道的方法 
        ret = pipe_read(fd, buf, count);*/
	}
	else
	{
		global_fd = fd_local2global(fd);
		//printk("off:%d\n",file_table[global_fd].offset);
		filepnt_init(file_table[global_fd].dirent);
		ret = file_read(file_table[global_fd].dirent, 0, buf,file_table[global_fd].offset, count);
		file_table[global_fd].offset += ret;
	}
	return ret;
}

/* 成功关闭文件返回0,失败返回-1 */
int sys_close(int fd)
{
	int32_t ret = -1; // 返回值默认为-1,即失败
	if (fd > 2)
	{
		uint32_t global_fd = fd_local2global(fd);
		/*if (is_pipe(fd))
		{
			// 如果此管道上的描述符都被关闭,释放管道的环形缓冲区 
			if (--file_table[global_fd].fd_pos == 0)
			{
                mfree_page(PF_KERNEL, file_table[global_fd].fd_inode, 1);
                file_table[global_fd].fd_inode = NULL;
            }
            ret = 0;
        }
        else
        {*/
			ret=file_close(&file_table[global_fd]);
        //}
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
		printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
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
	return sys_open(filename, flags, mode);
}

int sys_mkdirat(int dirfd, const char *path, mode_t mode)
{
	return sys_mkdir(path, mode);
}