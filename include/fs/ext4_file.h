#ifndef _EXT4_FILE_H_
#define _EXT4_FILE_H_

#include <xkernel/types.h>
#include <fs/fs.h>

int ext4_fread(Dirent *file, unsigned long dst, unsigned int off, unsigned int n);
int ext4_fwrite(Dirent *file, unsigned long src, unsigned int off, unsigned int n);
int ext4_dir_creat(Dirent* baseDir, char* path, int mode);
int ext4_file_creat(struct Dirent* baseDir, char* path, Dirent** file);
int ext4_fsthaw(void);
#endif