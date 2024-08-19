#include <fs/fs.h>
#include <xkernel/types.h>
#include <xkernel/stdio.h>
#include <xkernel/printk.h>
#include <xkernel/string.h>
#include <fs/vfs.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/buf.h>
#include <fs/ext4.h>
#include <fs/ext4_sb.h>
#include <fs/ext4_fs.h>
#include <fs/ext4_inode.h>
#include <fs/ext4_dir.h>
#include <fs/ext4_file.h>
#include <sync.h>
#include <debug.h>
#include <fs/procfile.h>


FileSystem *procFs;
static const struct fs_operation proc_op = {
 	.fs_init_ptr = procfs_init,
	//.file_read = procfs_read,
	.file_write = ext4_fwrite,
	.file_create = ext4_file_creat,
	//.makedir = procfs_dir_creat,
	.fsthaw = NULL,
};

int procfs_read(Dirent *file, unsigned int n)
{
	return 0;
}


void procfs_init(FileSystem* fs)
{
	printk("procfd is initing...\n");
	strcpy(procFs->name, "proc");
	//初始化根目录
	strcpy(procFs->root->name,"/");
	procFs->root->file_system = procFs;
	procFs->op = &proc_op;
	procFs->root->type = DIRENT_DIR;
	//设置目录树的树根
	procFs->root->parent_dirent = NULL; // 父节点为空，表示已经到达根节点
	list_init(&procFs->root->child_list);
	procFs->root->linkcnt = 1;
}

