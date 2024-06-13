#include <fs/vfs.h>
#include <fs/fs.h>
#include <debug.h>

/*构建vfs的根文件系统*/

static struct FileSystem *ramfs_fs_type;
static struct FileSystem *rootfs_fs_type;

void init_mount_tree(void)
{
	/*根挂载点没有父目录，记为空*/
	mnt_root.mnt_parent = NULL;
	mnt_root.mnt_mountpoint = NULL;
	mnt_root.mnt_count = 1;
	/*此时将根挂载点所挂载的目录标记为内存中的虚拟文件系统rootfs,待磁盘中的文件系统被加载后再将根挂载点所挂载的文件系统修改为磁盘文件系统*/
	mnt_root.mnt_root = rootfs_fs_type->root;
}

int init_rootfs(void) {
	printk("rootfs is initing...\n");
	allocFs(&rootfs_fs_type);
	strcpy(rootfs_fs_type->name,"rootfs");
	
	//为vfs的目录树提供 / 目录
	rootfs_fs_type->root = dirent_alloc();
	strcpy(rootfs_fs_type->root->name, "/");
	rootfs_fs_type->root->file_system = rootfs_fs_type;
	ASSERT(rootfs_fs_type == NULL);
	printk("rootfs is down\n");
}