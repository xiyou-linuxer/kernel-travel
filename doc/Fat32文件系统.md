# Fat文件系统

## 簇层

因为FAT32文件系统会根据磁盘镜像的大小选取大小不同的簇。为了减少上层文件系统对扇区号的计算，在缓冲层的基础上又增添了簇层对于簇进行管理。

该层提供的主要功能是通过簇号来计算扇区号。

```c
unsigned long clusterAlloc(FileSystem *fs, unsigned long prev); //分配一个簇
void clusterFree(FileSystem *fs, unsigned long cluster, unsigned long prev);//释放一个簇
unsigned long secno = clusterSec(fs, cluster) + offset / fs->superBlock.bpb.bytes_per_sec;
void clusterWrite(FileSystem *fs, unsigned long cluster, long offset, void *src, size_t n, bool isUser);//向簇内写入
void clusterRead(FileSystem *fs, unsigned long cluster, long offset, void *dst, size_t n, bool isUser);//从簇内读取
```

## Fat32 文件系统层

Fat32 文件系统的操作对象为 Dirent 结构体，其定义如下：

```c
typedef struct Dirent {
    FAT32Directory raw_dirent; // 原生的dirent项
    char name[MAX_NAME_LEN];

    // 文件系统相关属性
    FileSystem *file_system; // 所在的文件系统
    unsigned int first_clus;// 第一个簇的簇号（如果为0，表示文件尚未分配簇）
    unsigned int file_size;// 文件大小

    /* for OS */
    // 操作系统相关的数据结构
    // 仅用于是挂载点的目录，指向该挂载点所对应的文件系统。用于区分mount目录和非mount目录
    FileSystem *head;

    DirentPointer pointer;//簇指针

    // 在上一个目录项中的内容偏移，用于写回
    unsigned int parent_dir_off;
    // 标记是文件、目录还是设备文件（仅在文件系统中出现，不出现在磁盘中）
    unsigned short type;

    unsigned short is_rm;

    // 文件的时间戳
    struct file_time time;

    // 设备结构体，可以通过该结构体完成对文件的读写
    struct FileDev *dev;

    // 子Dirent列表
    struct list child_list;
    struct list_elem dirent_tag;//链表节点，用于父目录记录
    // 用于空闲链表和父子连接中的链接，因为一个Dirent不是在空闲链表中就是在树上
    //LIST_ENTRY(Dirent) dirent_link;
    // 父亲Dirent
    struct Dirent *parent_dirent; // 即使是mount的目录，也指向其上一级目录。如果该字段为NULL，表示为总的根目录
    u32 mode;
    // 各种计数
    unsigned short linkcnt; // 链接计数
    unsigned short refcnt;  // 引用计数
}Dirent;
```

### 文件系统初始化

文件系统的初始化包括初始化超级块，初始化根目录项，与递归构建目录树这三个部分。

```c
void fat32_init(FileSystem* fs) 
{
    // 1. 以fs为单位初始化簇管理器
    printk("fat32 is initing...\n");
    strcpy(fs->name, "FAT32");
    ASSERT(partition_format(fs) == 0);
    // 2. 初始化根目录
    fs->root = dirent_alloc();

    strcpy(fs->root->name, "/");
    fs->root->file_system = fs; // 此句必须放在countCluster之前，用于设置fs
    printk("cluster Init Finished!\n");
    // 设置Dirent属性
    fs->root->first_clus = fs->superBlock.bpb.root_clus;
    printk("first clus of root is %d\n", fs->root->first_clus);
    fs->root->raw_dirent.DIR_Attr = ATTR_DIRECTORY;
    fs->root->raw_dirent.DIR_FileSize = 0; // 目录的Dirent的size都是0
    fs->root->type = DIRENT_DIR;
    fs->root->file_size = countClusters(fs->root) * CLUS_SIZE(fs);

    // 设置树状结构
    fs->root->parent_dirent = NULL; // 父节点为空，表示已经到达根节点
    list_init(&fs->root->child_list);

    fs->root->linkcnt = 1;

    /* 不需要初始化fs->root的锁，因为在分配时即初始化了 */

    printk("root directory init finished!\n");
    ASSERT(sizeof(FAT32Directory) == DIRENT_SIZE);

    // 3. 递归建立Dirent树
    build_dirent_tree(fs->root);
    printk("build dirent tree succeed!\n");
    printk("fat32 init finished!\n");
}
```

### 目录项操作

```c
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet);//查找目录项
void sync_dirent_rawdata_back(Dirent *dirent)//回写目录项
dir_alloc_file(Dirent *dir, Dirent **file, char *path);//分配目录项
void dirent_dealloc(Dirent *dirent);//释放目录项
```

### 目录操作

```c
int makeDirAt(Dirent *baseDir, char *path, int mode);//创建目录
```
