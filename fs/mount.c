#include <fs/dirent.h>
#include <fs/fat32.h>
#include <fs/fs.h>
#include <fs/vfs.h>
#include <fs/fd.h>
#include <fs/path.h>
#include <fs/mount.h>
#include <xkernel/stdio.h>
#include <xkernel/string.h>
#include <sync.h>
#include <xkernel/errno.h>
#include <xkernel/thread.h>
#include <xkernel/list.h>
extern struct lock mtx_file;

struct vfsmount mount[MNT_NUM];//挂载点结构体

static struct vfsmount* alloc_vfsmount(void)
{
	for (int i = 0; i < MNT_NUM; i++)
	{
		if (mount[i].mnt_expiry_mark == 0)
		{
			mount[i].mnt_count++;
			mount[i].mnt_expiry_mark = 1;
			return mount;
		}
		
	}
	return NULL;
}

static int free_vfsmount(struct vfsmount* mnt)
{
	if (mnt == NULL)
	{
		return -1;
	}
	mnt->mnt_count--;
	if (mnt->mnt_count != 0)//如果--后当前挂载点的引用计数
	{
		return -1;
	}
	mnt->mnt_devname = NULL;
	mnt->mnt_rootdir = NULL;
	mnt->mnt_expiry_mark = 0;
	list_remove(&mnt->mnt_mounts);//将挂载点从父目录上取下
	return 0;
}

// mount之后，目录中原有的文件将被暂时取代为挂载的文件系统内的内容，umount时会重新出现
int mount_fs(char *special, char *dirPath, const char *fstype, unsigned long flags) {
	// 1. 寻找mount的目录
	Dirent *dir;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)dirPath);

	/* 先检查是否将全部的路径遍历 */
	dir = search_file(dirPath,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		// 说明并没有访问到全部的路径,某个中间目录是不存在的
		//printk("cannot access %s: Not a directory, subpath %s is`t exist\n",pathname, searched_record.searched_path);
		return -1;
	}

	// 检查dir是否是目录
	if (!is_directory(&dir->raw_dirent)) {
		printk("dir %s is not a directory!\n", dirPath);
		return -ENOTDIR;
	}

	//分配挂载点结构体
	dir->head = alloc_vfsmount();
	dir->head->mnt_mountpoint = dir;
	dir->head->mnt_parent = dir->parent_dirent->head;//挂载点的父挂载点应为所挂载目录项的父目录项对应的挂载点
	list_append(&dir->head->mnt_parent->mnt_child,&dir->head->mnt_mounts);

	// 寻找mount的文件
	// 特判是否是设备（deprecated）
	if (strncmp(special, "/dev/vda2", 10) == 0) {
		//如果vfsmount指向的root为空则说明挂载的是个设备而不是文件系统
		dir->head->mnt_rootdir = NULL;
		dir->head->mnt_devname = special;
		return 0;
	}

	// 3. 初始化mount的文件系统
	FileSystem *fs;
	allocFs(&fs);
	fs->op->fs_init_ptr(fs);
	fs->deviceNumber = 0;
	//初始化挂载点结构体vfsmount
	dir->head->mnt_rootdir = fs->root;
	return 0;
}

int umount_fs(char *dirPath) {
	// 1. 寻找mount的目录
	Dirent *dir;
	int fd = -1;
	struct path_search_record searched_record;
	memset(&searched_record, 0, sizeof(struct path_search_record));

	/* 记录目录深度.帮助判断中间某个目录不存在的情况 */
	unsigned int pathname_depth = path_depth_cnt((char *)dirPath);

	/* 先检查是否将全部的路径遍历 */
	dir = search_file(dirPath,&searched_record);
	unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
	if (pathname_depth != path_searched_depth)
	{ 
		return -1;
	}
	//如果挂载的是文件系统，如果是设备则直接释放
	if (dir->head->mnt_rootdir != NULL)
	{
		FileSystem *fs = elem2entry(FileSystem ,root, dir->head->mnt_rootdir);//通过所挂载文件系统的root项找到fs结构体
		deAllocFs(fs);//释放fs结构体
	}

	int ret = free_vfsmount(dir->head);
	dir->head = dir->parent_dirent->head;//卸载后与父目录所记录的挂载点保持一致
	return ret;
}