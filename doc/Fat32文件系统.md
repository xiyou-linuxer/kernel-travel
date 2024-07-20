# Fat文件系统

## 对于簇的管理

因FAT32文件系统以簇为单位对磁盘进行读写，且会根据磁盘镜像的大小选取大小不同的簇。而缓冲层则是以物理扇区为单位对磁盘进行读写。所以我们增加了对簇层的管理，由簇层调用缓冲层的接口对磁盘进行读写。

```c
unsigned long clusterAlloc(FileSystem *fs, unsigned long prev); //分配一个簇
void clusterFree(FileSystem *fs, unsigned long cluster, unsigned long prev);//释放一个簇
unsigned long secno = clusterSec(fs, cluster) + offset / fs->superBlock.bpb.bytes_per_sec;
void clusterWrite(FileSystem *fs, unsigned long cluster, long offset, void *src, size_t n, bool isUser);//向簇内写入
void clusterRead(FileSystem *fs, unsigned long cluster, long offset, void *dst, size_t n, bool isUser);//从簇内读取
```

## Fat32 文件系统层

### Fat32 中的结构体

`FAT32BootParamBloc`中记录了Fat32文件系统的元信息。

```c
typedef struct FAT32BootParamBlock {
    // 通用的引导扇区属性
    u8 BS_jmpBoot[3];
    u8 BS_OEMName[8];
    u16 BPB_BytsPerSec;     // 扇区大小（字节）
    u8 BPB_SecPerClus;      // 簇大小（扇区）
    u16 BPB_RsvdSecCnt;     // 保留扇区数
    u8 BPB_NumFATs;         // FAT 数量
    u16 BPB_RootEntCnt;     // 根目录条目数量
    u16 BPB_TotSec16;       // 逻辑分区中的扇区数量
    u8 BPB_Media;           // 硬盘介质类型
    u16 BPB_FATSz16;        // FAT 的大小（扇区），只有FAT12/16使用
    u16 BPB_SecPerTrk;      // 一个磁道的扇区数
    u16 BPB_NumHeads;       // 磁头的数量
    u32 BPB_HiddSec;        // 隐藏扇区数量
    u32 BPB_TotSec32;       // 逻辑分区中的扇区数量，FAT32使用
    // FAT32 引导扇区属性
    u32 BPB_FATSz32;        // FAT的大小（扇区），FAT32使用
    u16 BPB_ExtFlags;       // 扩展标志
    u16 BPB_FSVer;          // 高字节是主版本号，低字节是次版本号
    u32 BPB_RootClus;       // 根目录簇号
    u16 BPB_FSInfo;         // 文件系统信息结构扇区号
    u16 BPB_BkBootSec;      // 备份引导扇区号
、  u8 BPB_Reserved[12];    // 保留
    u8 BS_DrvNum;           // 物理驱动器号，硬盘通常是 0x80 开始的数字
    u8 BS_Reserved1;        // 为 Windows NT 保留的标志位
    u8 BS_BootSig;          // 固定的数字签名，必须是 0x28 或 0x29 
    u32 BS_VolID;           // 分区的序列号，可以忽略   
    u8 BS_VolLab[11];       // 卷标 
    u8 BS_FilSysType[8];    // 文件系统 ID，通常是"FAT32"
    u8 BS_CodeReserved[420];// 引导代码
    u8 BS_Signature[2];     // 可引导分区签名，0xAA55
} __attribute__((packed)) FAT32BootParamBlock;
```

该结构被记录在 VFS 层 SuperBlock 中的匿名联合体中。

`FAT32Directory` 为Fat32文件原生的目录项结构，其中记录了文件的元信息。

```c
typedef struct FAT32Directory {
    u8 DIR_Name[11];          // 文件名（8.3格式），包含文件名和扩展名，共11字节
    u8 DIR_Attr;              // 文件属性（例如，目录、只读、隐藏、系统文件等）
    u8 DIR_NTRes;             // 保留给NT的字节，通常设置为0
    u8 DIR_CrtTimeTenth;      // 创建时间的毫秒部分（0-199范围内的值）
    u16 DIR_CrtTime;          // 文件创建时间（小时、分钟和两秒钟间隔）
    u16 DIR_CrtDate;          // 文件创建日期（年、月、日）
    u16 DIR_LstAccDate;       // 上次访问日期（年、月、日）
    u16 DIR_FstClusHI;        // 文件起始簇号的高16位
    u16 DIR_WrtTime;          // 文件最后写入时间（小时、分钟和两秒钟间隔）
    u16 DIR_WrtDate;          // 文件最后写入日期（年、月、日）
    u16 DIR_FstClusLO;        // 文件起始簇号的低16位
    u32 DIR_FileSize;         // 文件大小（以字节为单位）
} __attribute__((packed)) FAT32Directory;  // __attribute__((packed)) 确保结构体按紧凑布局，不进行任何内存对齐
```

该结构被记录在 VFS 层中的 Dirent 结构的匿名联合体中。

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

在初赛阶段，Fat32文件系统被挂载在 / 目录下。

### 操作函数

Fat32文件系统中包含了针对目录文件和普通文件的操作函数

```c
void filepnt_init(Dirent *file);//初始化文件指针
int Fatfile_read(struct Dirent *file, unsigned long dst, unsigned int off, unsigned int n);//读文件
int Fatfile_write(struct Dirent *file, unsigned long src, unsigned int off, unsigned int n);//写文件
int createFile(struct Dirent *baseDir, char *path, Dirent **file);//创建文件
int rmfile(Dirent *file);//删除文件或目录
int makeDirAt(Dirent *baseDir, char *path, int mode);//创建目录
int dirGetDentFrom(Dirent *dir, u64 offset, struct Dirent **file, int *next_offset, longEntSet *longSet)//从文件中获取目录项
```

* 文件的初始化

在 Fat32 格式的磁盘镜像中，文件的数据索引被记录在Fat表中，为了减少文件系统对数据索引频繁的进行计算，方便后续使用。在文件第一次被访问时，会使用 filepnt_init 函数将文件数据块的索引解析出来并存入 DirentPointer 结构中。

```c
   typedef struct DirentPointer {
    // 一级指针
    // u32 first[10];
    // 简化：只使用二级指针和三级指针
    struct TwicePointer *second[NDIRENT_SECPOINTER];
    struct ThirdPointer *third;
    u16 valid;
    } DirentPointer;
```

Fat 文件系统的文件数据索引是链式结构。每个簇在 Fat 表中对应一个条目，而这些条目存储了下一个簇的编号，从而形成一个链式结构来跟踪文件数据。所以我们根据记录在目录项中的 first_clus 就能获取存储文件数据的第一个簇的位置。并将剩余的簇解析出来。

```c
void filepnt_init(Dirent *file) {
    if (!file->pointer.valid) {
        // 首次打开，更新pointer
        int clus = file->first_clus;
        if (clus == 0) {
            // 文件的大小为0，返回
            return;
        }
        DirentPointer *fileptr = &file->pointer;
        for (int i = 0; FAT32_NOT_END_CLUSTER(clus); i++) {
            // 更新pointer值
            filepnt_setval(fileptr, i, clus);

            // 查询下一级簇号
            clus = fatRead(file->file_system, clus);
        }
        file->pointer.valid = 1;
    }
}
```

* 目录与文件的创建

目录与文件的创建底层调用的均为一个函数

```c
static int createItemAt(struct Dirent *baseDir, char *path, Dirent **file, int isDir) {
    lock_acquire(&mtx_file);

    char lastElem[MAX_NAME_LEN];
    Dirent *dir = NULL, *f = NULL;
    int r;
    longEntSet longSet;
    FileSystem *fs;
    extern FileSystem *fatFs;

    if (baseDir) {
        fs = baseDir->file_system;
    } else {
        fs = fatFs;
    }
    struct path_search_record searched_record;
    memset(&searched_record, 0, sizeof(struct path_search_record));
    /* 记录目录深度，帮助判断中间某个目录不存在的情况 */
    unsigned int pathname_depth = path_depth_cnt((char *)path);
    /* 先检查是否将全部的路径遍历 */
    dir = search_file(path, &searched_record);
    unsigned int path_searched_depth = path_depth_cnt(searched_record.searched_path);
    if (pathname_depth != path_searched_depth) { 
        // 说明并没有访问到全部的路径，某个中间目录是不存在的
        lock_release(&mtx_file);
        return -1;
    }
    /* 如果已经存在该文件则不能重复建立 */
    if (dir != NULL) {
        if (isDir == 1 && dir->type == DIRENT_DIR) {
            printk("directory exists: %s\n", path);
            lock_release(&mtx_file);
        } else if (isDir == 0 && dir->type == DIRENT_FILE) {
            printk("file exists: %s\n", path);
            lock_release(&mtx_file);
        }
        return -1;
    }
    // 3. 分配Dirent，并获取新创建文件的引用
    if ((r = dir_alloc_file(searched_record.parent_dir, &f, path)) < 0) {
        lock_release(&mtx_file);
        return r;
    }
    // 4. 填写Dirent的各项信息
    f->parent_dirent = searched_record.parent_dir;                 // 设置父亲节点，以安排写回
    f->file_system = searched_record.parent_dir->file_system;
    f->head = f->parent_dirent->head;                              // 子目录的对应
    if (isDir == 1) {
        f->type = DIRENT_DIR;
    } else {
        f->type = DIRENT_FILE;
    }
    // 5. 目录应当以其分配了的大小为其文件大小
    if (isDir) {
        // 目录至少分配一个簇
        int clusSize = CLUS_SIZE(searched_record.parent_dir->file_system);
        f->first_clus = clusterAlloc(searched_record.parent_dir->file_system, 0); // 在Alloc时即将first_clus清空为全0
        f->file_size = clusSize;
        f->raw_dirent.DIR_Attr = ATTR_DIRECTORY;
    } else {
        // 空文件不分配簇
        f->file_size = 0;
        f->first_clus = 0;
    }
    filepnt_init(f);
    // 4. 将dirent加入到上级目录的子Dirent列表
    list_append(&searched_record.parent_dir->child_list, &f->dirent_tag);
    // 5. 回写dirent信息
    sync_dirent_rawdata_back(f);
    if (file) {
        *file = f;
    }
    lock_release(&mtx_file);
    return 0;
}
```

* 目录与文件的删除

#### 注册操作函数

```c
static const struct fs_operation fat32_op = {
    .fs_init_ptr = fat32_init,
    .file_init = filepnt_init,
    .file_create = createFile,
    .file_read = Fatfile_read,
    .file_write = Fatfile_write,
    .file_remove = rmfile,
    .makedir = makeDirAt
};
```
