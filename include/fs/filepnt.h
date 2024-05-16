#ifndef _CLUSTERNO_H
#define _CLUSTERNO_H
#include <linux/types.h>

typedef struct Dirent Dirent;
typedef struct DirentPointer DirentPointer;
typedef struct FileSystem FileSystem;

void filepnt_init(Dirent *file);
//void filepnt_clear(Dirent *file);
u32 filepnt_getclusbyno(Dirent *file, int fileClusNo);
void filepnt_setval(DirentPointer* fileptr, int i, int value);
void clus_sequence_free(FileSystem *fs, int clus);

#endif
