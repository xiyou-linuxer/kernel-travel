#include <asm/boot_param.h>
#include <asm/bootinfo.h>
#include <asm/fw.h>
#include <asm/loongarch.h>
#include <asm/page.h>
#include <xkernel/atomic.h>
#include <xkernel/init.h>
#include <xkernel/kernel.h>
#include <xkernel/printk.h>
#include <xkernel/string.h>
#include <xkernel/types.h>

#define EPERM 1
#define PAGE_MASK (~(PAGE_SIZE - 1))
#ifndef PFN_ALIGN
#    define PFN_ALIGN(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#endif

struct loongsonlist_mem_map         *loongson_mem_map;
struct boot_params                  *efi_bp;
struct loongsonlist_vbios           *pvbios;
struct loongson_system_configuration loongson_sysconf;

void       show_memory_map(void);
static int parse_mem(struct _extention_list_hdr *);

static int get_bpi_version(void *signature)
{
    char data[8];
    int  r, version = 0;

    memset(data, 0, 8);
    memcpy(data, signature + 4, 4);
    r = kstrtoint(data, 10, &version);

    if(r < 0 || version < BPI_VERSION_V1)
        {
            /**
     * TODO: 实现panic
     */
            printk("Fatal error, invalid BPI version: %d\n", version);
            while(1)
                ;
    }

    /**
   * TODO: 解析flags
   * if (version >= BPI_VERSION_V2)
   * 	parse_flags(efi_bp->flags);
   */

    return version;
}

static int parse_extlist(struct boot_params *bp)
{
    struct _extention_list_hdr *fhead;

    if(loongson_sysconf.bpi_ver >= BPI_VERSION_V3)
        fhead = (struct _extention_list_hdr *)((char *)bp + bp->extlist_offset);
    else
        fhead = (struct _extention_list_hdr *)bp->extlist_offset;
    parse_mem(fhead);

    return 0;
}

void __init fw_init_environ(void)
{
    efi_bp                   = (struct boot_params *)_fw_envp;
    loongson_sysconf.bpi_ver = get_bpi_version(&efi_bp->signature);

    pr_info("BPI%d with boot flags %llx.\n", loongson_sysconf.bpi_ver,
            efi_bp->flags);

    printk("@@@@@: efi_bp = %p\n", efi_bp);
    printk("@@@@@: efi_bp->signature = %llx\n", efi_bp->signature);
    printk("@@@@@: flags = %llx\n", efi_bp->flags);

    if(parse_extlist(efi_bp))
        printk("失败!Scan bootparm failed\n");
}

static void __init register_addrs_set(u64 *registers, const u64 addr, int num)
{
    u64 i;

    for(i = 0; i < num; i++)
        {
            *registers = (i << 44) | addr;
            registers++;
        }
}

static u8 ext_listhdr_checksum(u8 *buffer, u32 length)
{
    u8  sum = 0;
    u8 *end = buffer + length;

    while(buffer < end)
        {
            sum = (u8)(sum + *(buffer++));
        }

    return (sum);
}

// 解析内存映射数据
static int parse_mem(struct _extention_list_hdr *head)
{
    struct loongsonlist_mem_map_legacy *ptr;
    static struct loongsonlist_mem_map  mem_map;
    int                                 i;

    loongson_mem_map = (struct loongsonlist_mem_map *)head;

    if(ext_listhdr_checksum((u8 *)loongson_mem_map, head->length))
        {
            printk("mem checksum error\n");
            return -EPERM;
    }

    if(loongson_sysconf.bpi_ver < BPI_VERSION_V3)
        {
            ptr = (struct loongsonlist_mem_map_legacy *)head;

            pr_info("convert legacy mem map to new mem map.\n");
            memcpy(&mem_map, ptr, sizeof(mem_map.header));
            mem_map.map_count = ptr->map_count;
            for(i = 0; i < ptr->map_count; i++)
                {
                    mem_map.map[i].mem_type  = ptr->map[i].mem_type;
                    mem_map.map[i].mem_start = ptr->map[i].mem_start;
                    mem_map.map[i].mem_size  = ptr->map[i].mem_size;
                }
            loongson_mem_map = &mem_map;
    }

    show_memory_map();

    return 0;
}

void show_memory_map(void)
{
    if(!loongson_mem_map || loongson_mem_map->map_count == 0)
        {
            pr_info("物理内存映射为空\n");
            return;
    }

    pr_info("物理内存布局(%d块):\n", loongson_mem_map->map_count);
    for(int i = 0; i < loongson_mem_map->map_count; i++)
        {
            u32 mem_type  = loongson_mem_map->map[i].mem_type;
            u64 mem_start = loongson_mem_map->map[i].mem_start;
            u64 mem_size  = loongson_mem_map->map[i].mem_size;
            u64 mem_end   = mem_start + mem_size;

            if(mem_size == 0)
                continue; // 跳过无效区域
            mem_start = PFN_ALIGN(mem_start);
            mem_end   = PFN_ALIGN(mem_end);
            pr_info("%d %u\n", i, mem_type);

            switch(mem_type)
                {
                case ADDRESS_TYPE_SYSRAM:
                    pr_info("区域 %d: 起始地址: %llx, 大小: %llx, 类型: "
                            "%u,主内存\n",
                            i, mem_start, mem_size, mem_type);
                    // memblock_set_node(mem_start, mem_size, &memblock.memory, 0);
                    break;
                case ADDRESS_TYPE_ACPI:
                    // memblock_add(mem_start, mem_size);
                    pr_info(
                        "区域 %d: 起始地址: %llx, 大小: %llx, 类型: %u,加入\n",
                        i, mem_start, mem_size, mem_type);
                    // memblock_set_node(mem_start, mem_size, &memblock.memory, 0);
                    break;
                case ADDRESS_TYPE_RESERVED:
                    // memblock_reserve(mem_start, mem_size);
                    pr_info(
                        "区域 %d: 起始地址: %llx, 大小: %llx, 类型: %u,保留\n",
                        i, mem_start, mem_size, mem_type);
                    break;
                default:
                    pr_info(
                        "区域 %d: 起始地址: %llx, 大小: %llx, 类型: %u,0000\n",
                        i, mem_start, mem_size, mem_type);
                    continue;
                }
        }
    return;
}