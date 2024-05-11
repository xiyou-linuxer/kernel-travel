#ifndef _DIRENT_H
#define _DIRENT_H

#include <fs/fs.h>

typedef struct Dirent Dirent;
void dirent_init(void);
Dirent *dirent_alloc(void);
void dirent_dealloc(Dirent *dirent);
Dirent* search_dir_tree(Dirent* parent, char* name);
char* path_parse(char* pathname, char* name_store);
int path_depth_cnt(char* pathname);
#endif
