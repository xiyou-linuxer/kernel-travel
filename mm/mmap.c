#include <xkernel/mmap.h>
#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <xkernel/rbtree.h>
#include <xkernel/stdio.h>
#include <asm-generic/errno.h>

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
	printk("0x%llx\n",running_thread());
	malloc_usrpage(running_thread()->pgdir, (unsigned long)vma);
	// memset(vma, 0, sizeof(*vma));
	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_flags = flags;
	vma->vm_pgoff = pgoff;

	if(file) {
		// call_mmap();	FAT32 的处理函数...
	} else if (flags & VM_SHARED) {
	
	} else {

	}
	/*建立VMA和红黑树，文件页等映射*/
 	vma_link(mm, vma, prev, rb_link, rb_parent);

out:
	/*更新 mm_struct 的统计信息*/
	mm->total_vm += len >> PAGE_SHIFT;
	return addr;
}


void *sys_mmap(void* addr, size_t len, int prot,
		int flags, int fd, off_t pgoff)
{
	return (void *)do_mmap(NULL, (unsigned long)addr, len, prot, flags, pgoff);
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