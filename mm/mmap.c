#include <xkernel/mmap.h>
#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <xkernel/rbtree.h>
#include <xkernel/stdio.h>
#include <asm-generic/errno.h>
#include <asm/page.h>
#include <sync.h>
#include <fs/file.h>
#include "fs/fd.h"

unsigned long sysctl_max_map_count = 1024;

struct vm_area_struct * 
find_vma(struct mm_struct * mm, unsigned long addr)
{
	struct vm_area_struct *vma = NULL;

	if (mm) {
		/*保证合法区域*/
		vma = mm->mmap_cache;
		if (!(vma && vma->vm_end > addr && vma->vm_start <= addr)) {
			struct rb_node * rb_node;

			rb_node = mm->mm_rb.rb_node;
			vma = NULL;

			while (rb_node) {
				struct vm_area_struct * vma_tmp;

				vma_tmp = rb_entry(rb_node,
						struct vm_area_struct, vm_rb);

				if (vma_tmp->vm_end > addr) {
					vma = vma_tmp;
					/*会使用红黑树维护虚拟内存的一致性，不会出现VMA之间有交集*/
					if (vma_tmp->vm_start <= addr)
						break;
					rb_node = rb_node->rb_left;
				} else
					rb_node = rb_node->rb_right;
			}
			if (vma)
				mm->mmap_cache = vma;
		}
	}
	return vma;
}

unsigned long
get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct mm_struct *mm = running_thread()->mm;
	struct vm_area_struct *vma;
	unsigned long start_addr;

	/*越界处理*/
	if (len > MAX_ADDRESS_SPACE_SIZE)
		return -1;

	/*在给定的地址处分配，如果成功返回*/
	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (MAX_ADDRESS_SPACE_SIZE - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			return addr;
	}
	/*从 free_area_cache 开始搜索*/
	start_addr = addr = mm->free_area_cache;

full_search:
	/*遍历内存区域链表*/
	for (vma = find_vma(mm, addr); ; vma = vma->vm_next) {
		/*如果地址超出 TASK_SIZE，重新从 TASK_UNMAPPED_BASE 开始搜索。*/
		if (MAX_ADDRESS_SPACE_SIZE - len < addr) {
			if (start_addr != TASK_UNMAPPED_BASE) {
				start_addr = addr = TASK_UNMAPPED_BASE;
				goto full_search;
			}
			return -1;
		}
		/*找到了足够大的未映射区域，则返回这个地址，并更新 free_area_cache*/
		if (!vma || addr + len <= vma->vm_start) {
			mm->free_area_cache = addr + len;
			return addr;
		}
		addr = vma->vm_end;
	}
	return (unsigned long)addr;
}

static void
__vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
	struct vm_area_struct *prev, struct rb_node **rb_link,
	struct rb_node *rb_parent)
{
	/*VMA 链表初始化*/
	if (prev) {
		vma->vm_next = prev->vm_next;
		prev->vm_next = vma;
	} else {
		/*第一个初始化*/
		mm->mmap = vma;
		if (rb_parent)
			vma->vm_next = rb_entry(rb_parent,
					struct vm_area_struct, vm_rb);
		else
			vma->vm_next = NULL;
	}
	/*更新管理VMA的红黑树*/
	rb_link_node(&vma->vm_rb, rb_parent, rb_link);
	rb_insert_color(&vma->vm_rb, &mm->mm_rb);
}

static void vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
			struct vm_area_struct *prev, struct rb_node **rb_link,
			struct rb_node *rb_parent)
{
	__vma_link(mm, vma, prev, rb_link, rb_parent);
	// __vma_link_file(vma);

	mm->map_count++;
}

static struct vm_area_struct *
find_vma_prepare(struct mm_struct *mm, unsigned long addr,
		struct vm_area_struct **prev_vma, struct rb_node ***rb_link,
		struct rb_node ** rb_parent)
{
	struct vm_area_struct * vma;
	struct rb_node ** __rb_link, * __rb_parent, * rb_prev;

	__rb_link = &mm->mm_rb.rb_node;
	rb_prev = __rb_parent = NULL;
	vma = NULL;

	while (*__rb_link) {
		struct vm_area_struct *vma_tmp;
		/*保存迭代前的节点*/
		__rb_parent = *__rb_link;
		vma_tmp = rb_entry(__rb_parent, struct vm_area_struct, vm_rb);

		if (vma_tmp->vm_end > addr) {
			vma = vma_tmp;
			/*当前VMA包含该 addr*/
			if (vma_tmp->vm_start <= addr)
				return vma;
			/*当前 VMA 过大*/
			__rb_link = &__rb_parent->rb_left;
		} else {
			/*当前 VMA 过小*/
			rb_prev = __rb_parent;
			__rb_link = &__rb_parent->rb_right;
		}
	}

	*prev_vma = NULL;
	if (rb_prev)
		*prev_vma = rb_entry(rb_prev, struct vm_area_struct, vm_rb);
	*rb_link = __rb_link;
	*rb_parent = __rb_parent;
	/*未找到，返回最接近的VMA*/
	return vma;
}

static inline 
bool file_mmap_ok(struct file *file, unsigned long pgoff, 
				unsigned long len)
{
	/*这里目前只有普通文件*/
	u64 maxsize = MAX_LFS_FILESIZE;

	if (maxsize && len > maxsize)
		return false;
	maxsize -= len;
	/*给定的页面偏移量超出了剩余可用的映射范围*/
	if (pgoff > maxsize >> PAGE_SHIFT)
		return false;
	return true;
}

/*检查映射的虚拟内存是否超过内核限制*/
static 
bool check_vaddr_limit(void)
{
	return true;
}

static void find_vma_links(void)
{
	return;
}

/* 当前用不到，直接返回NULL */
struct vm_area_struct *vma_merge(struct mm_struct *mm,
		struct vm_area_struct *prev, unsigned long addr,
		unsigned long end, unsigned long vm_flags,
		struct anon_vma *anon_vma, struct file *file,
		pgoff_t pgoff)
{
	return NULL;
}

unsigned long do_mmap_pgoff(struct file * file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, unsigned long pgoff)
{
	struct mm_struct *mm = running_thread()->mm;	/* 获取该进程的memory descriptor*/
	struct vm_area_struct *vma, *prev;
	struct rb_node ** rb_link, * rb_parent;
	unsigned long *v_addr = (unsigned long *)VADDR_FOR_FD_MAPP;
	u64 offset = 0;

	len = PAGE_ALIGN(len);
	if (!len)
		return -1;
	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
		return -1;
	/*如果超过最大 map 数量*/
	if (mm->map_count > sysctl_max_map_count)
		return -1;
	/*查找没被分配的虚拟地址*/
	addr = running_thread()->mm->get_unmapped_area(file, addr + TASK_UNMAPPED_BASE, len, pgoff, flags);

	/*根据 file 和 flages 设置最终的 vm_flags*/

	/*用于后续扩展*/
	if (file) {
		unsigned long flags_mask;

		if (!file_mmap_ok(file, pgoff, len))
			return -EOVERFLOW;

		flags_mask = LEGACY_MAP_MASK;

		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			break;
		case MAP_PRIVATE:
			break;
		default:
			return -EINVAL;
		}
	} else {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			break;
		case MAP_PRIVATE:
			break;
		default:
			return -EINVAL;
		}
	}

	/*检查是否超过地址空间虚拟地址用量的限制
	* 如果超过报异常*/
	check_vaddr_limit();

	/*检查是否有重叠映射：
	* 如果有就解除旧的映射*/
	// find_vma_links();
		/*do_munmap 解除映射*/

	/*获取 addr 对应 VMA*/
	vma = find_vma_prepare(mm, addr, &prev, &rb_link, &rb_parent);


	/*尝试与相邻的VMA进行合并
	* 可以合并：返回*/
	// vma = vma_merge(mm, prev, addr, addr + len, flags,
	// 	NULL, file, pgoff);
 
	/*初始化 VMA 结构
	* 文件映射： call_mmap
	* 共享匿名映射: shmem_zero_setup
	* 私有匿名映射: vma_set_anonyumous*/

	/*VMA 分配物理内存并初始化*/
	// printk("0x%llx\n",running_thread());
	// malloc_usrpage(running_thread()->pgdir, (unsigned long)vma);
	vma = (struct vm_area_struct *)get_page();
	memset(vma, 0, sizeof(*vma));
	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_flags = flags;
	vma->vm_pgoff = pgoff;


	if(file) {
		// call_mmap();	FAT32 的处理函数...
		fd_mapping(file->fd, pgoff, pgoff + (len >> 12), v_addr);
		int i = len >> 12;
		offset = *v_addr & (~PAGE_MASK);
		while (i--) {
			page_table_add(running_thread()->pgdir, addr,
					v_addr[i], PTE_V | PTE_PLV | PTE_D);
		}
		addr |= offset;
	} else if (flags & VM_SHARED) {
	
	} else {

	}
	/*建立VMA和红黑树，文件页等映射*/
 	vma_link(mm, vma, prev, rb_link, rb_parent);

	if (true) {
		int count = len >> PAGE_SHIFT;
		for (int i = 0; i < len >> PAGE_SHIFT; ++i)
			malloc_usrpage(running_thread()->pgdir,(unsigned long)addr+i*PAGE_SIZE);
	}

out:
	/*更新 mm_struct 的统计信息*/
	mm->total_vm += len >> PAGE_SHIFT;
	return addr;
}


void *sys_mmap(void* addr, size_t len, int prot,
		int flags, int fd, off_t offset)
{
	/*fd 获取 struct file*/
	struct file * file = NULL;
	if (fd > 2) {
		file = (struct file *)get_page();
		file->fd = fd;
	}

	return (void *)do_mmap(file, (unsigned long)addr, len, prot, flags, offset);
}

int sys_munmap(void *start, size_t len)
{
	return 0;
}

unsigned long do_mmap(struct file *file, unsigned long addr,
	unsigned long len, unsigned long prot,
	unsigned long flag, unsigned long offset)
{
	unsigned long ret = -1;
	if ((offset + PAGE_ALIGN(len)) < offset)
		goto out;
	if (!(offset & ~PAGE_MASK))
		ret = do_mmap_pgoff(file, addr, len, prot, flag, offset >> PAGE_SHIFT);
		
out:
	return ret;
}

unsigned long do_brk(unsigned long addr, unsigned long len)
{
	struct mm_struct *mm = running_thread()->mm;
	struct vm_area_struct * vma, * prev;
	unsigned long flags;
	struct rb_node ** rb_link, * rb_parent;
	pgoff_t pgoff = addr >> PAGE_SHIFT;
	int error;

	len = PAGE_ALIGN(len);
	if (!len)
		return addr;

	error = get_unmapped_area(NULL, addr, len, 0, MAP_FIXED);
	if (error & ~PAGE_MASK)
		return error;

 munmap_back:
	vma = find_vma_prepare(mm, addr, &prev, &rb_link, &rb_parent);
	if (vma && vma->vm_start < addr + len) {
		// if (do_munmap(mm, addr, len))
			// return -ENOMEM;
		// goto munmap_back;
	}

	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;

	/*是否能合并，能合并直接退出*/
	vma = vma_merge(mm, prev, addr, addr + len, flags,
					NULL, NULL, pgoff);
	if (vma)
		goto out;

	/* VMA 初始化*/
	vma = (struct vm_area_struct *)get_page();

	if (!vma)
		return -ENOMEM;

	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_pgoff = pgoff;
	vma->vm_flags = flags;
	vma_link(mm, vma, prev, rb_link, rb_parent);
out:
	mm->total_vm += len >> PAGE_SHIFT;
	return addr;
}

int sys_brk(void *addr)
{
	struct mm_struct *mm = running_thread()->mm;
	unsigned long min_brk, ret;
	unsigned long brk = (unsigned long) addr;
	min_brk = mm->start_brk;

	/*非法数据*/
	if(brk < min_brk)
		goto out;

	unsigned long newbrk = PAGE_ALIGN(brk);
	unsigned long oldbrk = PAGE_ALIGN(mm->brk);
	if (newbrk == oldbrk)
		goto set_brk;
	
	/* brk 收缩*/
	if (brk <= mm->brk) {
		/*解除数据段，成功解除映射跳至 set_brk ，否则跳转 out*/
		// do_munmap(mm, newbrk, oldbrk - newbrk)
		goto out;
	} else if (do_brk(oldbrk, newbrk - oldbrk) != oldbrk) {
		/*扩展失败*/
		goto out;
	}
set_brk:
	mm->brk = brk;
out:
	ret = mm->brk;
	return ret;
}
