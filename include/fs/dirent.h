#ifndef _DIRENT_H
#define _DIRENT_H

#include <fs/fs.h>

typedef struct Dirent Dirent;
void dirent_init(void);
Dirent *dirent_alloc(void);
void dirent_dealloc(Dirent *dirent);
#endif
