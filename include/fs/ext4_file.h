#ifndef _EXT4_FILE_H_
#define _EXT4_FILE_H_

#include <xkernel/types.h>
#include <fs/fs.h>

int ext4_fread(Dirent *file, unsigned long dst, unsigned int off, unsigned int n);
int ext4_fwrite(Dirent *file, unsigned long src, unsigned int off, unsigned int n);
#endif