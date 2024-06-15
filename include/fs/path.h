#ifndef _FS_PATH_H
#define _FS_PATH_H

#include <fs/fs.h>
#include <fs/mount.h>
#include <xkernel/types.h>


#define MAX_PATH_LEN 512 // 路径最大长度
//该结构体用来记录搜索过的文件路径
struct path_search_record
{
    char searched_path[MAX_PATH_LEN]; // 查找过程中的父路径
    struct Dirent *parent_dir;           // 文件或目录所在的直接父目录
    enum dirent_type file_type;        // 找到的是普通文件还是目录,找不到将为未知类型(DIRENT_UNKNOWN)
};

void path_resolution(const char* pathname);
char* path_parse(char* pathname, char* name_store);
int path_depth_cnt(char* pathname);
Dirent* search_dir_tree(Dirent* parent, char* name);
Dirent* search_file(const char *pathname, struct path_search_record *searched_record);
#endif