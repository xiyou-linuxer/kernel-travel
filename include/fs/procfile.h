#ifndef __PROCFILE_H
#define __PROCFILE_H
#include <fs/fs.h>

int procfs_read(Dirent *file, unsigned int n);
void procfs_init(FileSystem* fs);


#endif
