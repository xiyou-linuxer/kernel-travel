#ifndef _FS_DIRENT_H
#define _FS_DIRENT_H

#include <fs/fat32.h>

#define CLUS_SIZE(fs) ((fs)->superBlock.bytes_per_clus)

typedef struct Dirent Dirent;
int is_directory(FAT32Directory* f);
void dirent_init();
Dirent *dirent_alloc();
void dirent_dealloc(Dirent *dirent);

#endif
