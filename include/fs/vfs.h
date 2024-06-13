#ifndef _VFS_H
#define _VFS_H

#include <fs/fs.h>
#include <fs/mount.h>
#include <xkernel/types.h>

#define MAX_PATH_LEN 512 // 路径最大长度
//该结构体用来记录搜索过的文件路径
struct path_search_record
{
    char searched_path[MAX_PATH_LEN]; // 查找过程中的父路径
    struct Dirent *parent_dir;           // 文件或目录所在的直接父目录
    enum dirent_type file_type;        // 找到的是普通文件还是目录,找不到将为未知类型(DIRENT_UNKNOWN)
};


struct Dirent; // 前向声明

struct kstat; // 前向声明

extern struct vfsmount mnt_root;	/*根挂载点*/
extern FileSystem* current_fs;		/*当前对应的文件系统*/

int get_entry_count_by_name(char* name);
int countClusters(struct Dirent* file);
int get_file_raw(struct Dirent *baseDir, char *path, Dirent **file);
int getFile(struct Dirent *baseDir, char *path, Dirent **file);
int file_read(struct Dirent *file, int user, unsigned long dst, unsigned int off, unsigned int n);
int file_write(struct Dirent *file, int user, unsigned long src, unsigned int off, unsigned int n);
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet);
void sync_dirent_rawdata_back(Dirent *dirent);
int walk_path(FileSystem *fs, char *path, Dirent *baseDir, Dirent **pdir, Dirent **pfile,char *lastelem, longEntSet *longSet);
void dget_path(Dirent *file);
void dget(Dirent *dirent);
int find_fs_of_dir(FileSystem* fs, void* data);
int dir_alloc_file(Dirent *dir, Dirent **file, char *path);
Dirent* search_file(const char *pathname, struct path_search_record *searched_record);
int createFile(struct Dirent* baseDir, char* path, Dirent** file);
int makeDirAt(Dirent* baseDir, char* path, int mode);
void file_shrink(Dirent* file, u64 newsize);
void test_fs_all(void);
void fileStat( struct Dirent* file, struct kstat* pKStat);
int mount_fs(char* special, char* dirPath);
int umount_fs(char* dirPath);
#endif
