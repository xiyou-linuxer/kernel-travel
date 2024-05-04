#ifndef _SYSFS_H
#define _SYSFS_H

#include <fs/fs.h>
#include <types.h>

int countClusters(struct Dirent *file);
int get_entry_count_by_name(char *name);

// 文件系统层接口函数

int get_file_raw(struct Dirent *baseDir, char *path, Dirent **file);
int getFile(struct Dirent *baseDir, char *path, Dirent **file);
int createFile(struct Dirent *baseDir, char *path, Dirent **file);
int create_file_and_close(char *path);
int file_read(struct Dirent *file, int user, u64 dst, uint off, uint n);
int file_write(struct Dirent *file, int user, u64 src, uint off, uint n);
void file_shrink(Dirent *file, u64 newsize);
void file_extend(struct Dirent *file, int newSize);
void file_close(Dirent *file);
void dget(Dirent *dirent);
void dput(Dirent *dirent);

int linkat(struct Dirent *oldDir, char *oldPath, struct Dirent *newDir, char *newPath);
int unlinkat(struct Dirent *dir, char *path);
int rm_unused_file(struct Dirent *file);
int renameat2(Dirent *oldDir, char *oldPath, Dirent *newDir, char *newPath, u32 flags);

int makeDirAt(Dirent *baseDir, char *path, int mode);
void fileStat(struct Dirent *file, struct kstat *pKStat);
int faccessat(Dirent *dir, char *path, int mode, int flags);
void fs_sync();

int find_fs_of_dir(FileSystem *fs, void *data);
void sync_dirent_rawdata_back(Dirent *dirent);
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset,
		   longEntSet *longSet);
int dir_alloc_file(Dirent *dir, Dirent **file, char *name);
void dirent_get_path(Dirent *dirent, char *path);
void init_root_fs();
int walk_path(FileSystem *fs, char *path, Dirent *baseDir, Dirent **pdir, Dirent **pfile,
	      char *lastelem, longEntSet *longSet);

void dput_path(Dirent *file);
void dget_path(Dirent *file);

void fat32_init(FileSystem *fs);
void init_files();
int mount_fs(char *special, Dirent *baseDir, char *dirPath);
int umount_fs(char *dirPath, Dirent *baseDir);

#endif