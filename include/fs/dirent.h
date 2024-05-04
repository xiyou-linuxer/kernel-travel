#ifndef _FS_DIRENT_H
#define _FS_DIRENT_H

typedef struct Dirent Dirent;

void dirent_init();
Dirent *dirent_alloc();
void dirent_dealloc(Dirent *dirent);

#endif
