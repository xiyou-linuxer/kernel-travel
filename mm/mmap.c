#include <xkernel/mmap.h>
#include <xkernel/types.h>
#include <xkernel/memory.h>
#include <xkernel/thread.h>
#include <xkernel/rbtree.h>
#include "xkernel/stdio.h"

unsigned long sysctl_max_map_count = 1024;

struct vm_area_struct * find_vma(struct mm_struct * mm, unsigned long addr)
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
	return (unsigned long)vma;
}

unsigned long do_mmap_pgoff(struct file * file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, unsigned long pgoff)
{
	struct mm_struct *mm = running_thread()->mm;	/* 获取该进程的memory descriptor*/
	len = PAGE_ALIGN(len);
	if (!len)
		return -1;
	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
		return -1;
	if (mm->map_count > sysctl_max_map_count)
		return -1;

	printk("addr:0x%llx\n",addr);
	addr = running_thread()->mm->get_unmapped_area(file, addr, len, pgoff, flags);
	printk("addr:0x%llx\n",addr);
	return 0;
}


void* sys_mmap(void* addr, size_t len, int prot,
		int flags, int fd, off_t pgoff)
{
	char * tmp = (char*)get_page();
	tmp = "mmap content:   Hello, mmap successfully!";
	return tmp;
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