#ifndef _VFS_H
#define _VFS_H

#include <fs/fs.h>
#include <fs/fd.h>
#include <fs/mount.h>
#include <xkernel/types.h>

struct Dirent; // 前向声明

struct kstat; // 前向声明

extern struct vfsmount mnt_root;	/*根挂载点*/
	/*当前路径对应的文件系统*/

struct iovec {
    void  *iov_base;   /* Starting address */
    size_t iov_len;    /* Number of bytes to transfer */
};

int get_entry_count_by_name(char* name);
int countClusters(struct Dirent* file);
int get_file_raw(struct Dirent *baseDir, char *path, Dirent **file);
int getFile(struct Dirent *baseDir, char *path, Dirent **file);
//int file_read(struct Dirent *file, unsigned long dst, unsigned int off, unsigned int n);
//int file_write(struct Dirent *file, unsigned long src, unsigned int off, unsigned int n);
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet);
void sync_dirent_rawdata_back(Dirent *dirent);
int walk_path(FileSystem *fs, char *path, Dirent *baseDir, Dirent **pdir, Dirent **pfile,char *lastelem, longEntSet *longSet);
void dget_path(Dirent *file);
void dget(Dirent *dirent);
int find_fs_of_dir(FileSystem* fs, void* data);
int dir_alloc_file(Dirent *dir, Dirent **file, char *path);
int createFile(struct Dirent* baseDir, char* path, Dirent** file);
int makeDirAt(Dirent* baseDir, char* path, int mode);
//void file_shrink(Dirent* file, u64 newsize);
void test_fs_all(void);
void fileStat( struct Dirent* file, struct kstat* pKStat);
int mount_fs(char *special, char *dirPath, const char *fstype, unsigned long flags);
int umount_fs(char* dirPath);
int do_vfs_ioctl(struct fd *filp, unsigned int fd, unsigned int cmd, unsigned long arg);
#endif
