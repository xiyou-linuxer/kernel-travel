#ifndef _VFS_H
#define _VFS_H

#include <fs/fs.h>
#include <linux/types.h>

int countClusters(struct Dirent *file);
int get_file_raw(struct Dirent *baseDir, char *path, Dirent **file);
int getFile(struct Dirent *baseDir, char *path, Dirent **file);
int file_read(struct Dirent *file, int user, unsigned long dst, unsigned int off, unsigned int n);
int file_write(struct Dirent *file, int user, unsigned long src, unsigned int off, unsigned int n);
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet);
void sync_dirent_rawdata_back(Dirent *dirent);
int walk_path(FileSystem *fs, char *path, Dirent *baseDir, Dirent **pdir, Dirent **pfile,char *lastelem, longEntSet *longSet);
void dget_path(Dirent *file);
int find_fs_of_dir(FileSystem* fs, void* data);
Dirent* search_file(Dirent* parent, char* name);
Dirent* search_file1(const char *pathname, struct path_search_record *searched_record);
#endif
