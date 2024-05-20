#include <xkernel/list.h>
#include <trap/irq.h>
#include <debug.h>

void list_init(struct list* list)
{
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

void list_insert_before(struct list_elem* before,struct list_elem* elm)
{
	enum intr_status old_status = intr_disable();
	before->prev->next = elm;
	elm->prev = before->prev;
	elm->next = before;
	before->prev = elm;
	intr_set_status(old_status);
}

void list_push(struct list* plist,struct list_elem* elm)
{
	list_insert_before(plist->head.next,elm);
}

void list_append(struct list* plist,struct list_elem* elm)
{
	list_insert_before(&plist->tail,elm);
}

void list_remove(struct list_elem* elm)
{
	enum intr_status old_status = intr_disable();
	elm->prev->next = elm->next;
	elm->next->prev = elm->prev;
	intr_set_status(old_status);
}

struct list_elem* list_pop(struct list* plist)
{
	struct list_elem* ret = plist->head.next;
	list_remove(plist->head.next);
	return ret;
}

/*返回队尾的节点*/
struct list_elem* list_tail(struct list* plist)
{
	struct list_elem* ret = plist->tail.prev;
	list_remove(plist->tail.prev);
	return ret;
}

bool list_empty(struct list* plist)
{
	if (plist->head.next == &plist->tail)
		return 1;
	return 0;
}

uint32_t list_len(struct list* plist)
{
	uint32_t len = 0;
	struct list_elem* elm = plist->head.next;
	while (elm != &plist->tail)
	{
		len++;
		elm = elm->next;
	}

	return len;
}

struct list_elem* list_traversal(struct list* plist, function func, void *arg)
{
	struct list_elem* ret = plist->head.next;
	while (ret != &plist->tail)
	{
		if (func(ret,arg)){
			return ret;
		}
		ret = ret->next;
	}

	return NULL;
}

/*反向遍历*/
struct list_elem* list_reverse(struct list* plist, function func, void *arg)
{
	struct list_elem* ret = plist->tail.prev;
	while (ret != &plist->head)
	{
		if (func(ret,arg)){
			return ret;
		}
		ret = ret->prev;
	}

	return NULL;
}

bool elem_find(struct list* plist,struct list_elem* obj_elem)
{
	struct list_elem* ret = plist->head.next;
	while (ret != &plist->tail)
	{
		if (ret == obj_elem){
			return true;
		}
		ret = ret->next;
	}
	return false;
}

