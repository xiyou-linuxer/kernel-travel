#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include <linux/types.h>

#define offset(struct_type,member) (long)(&((struct_type*)0)->member)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type*)((long)elem_ptr - offset(struct_type, struct_member_name))

struct list_elem {
   struct list_elem* prev;
   struct list_elem* next;
};

struct list {
   struct list_elem head;
   struct list_elem tail;
};

struct list_head {
	struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head* list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}
/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}


/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

typedef int (function) (struct list_elem* ,void* arg);

#define list_elem_init(elm) list_insert_before(elm,elm)

void list_init(struct list*);
void list_insert_before(struct list_elem* before,struct list_elem* elm);
void list_push(struct list* plist,struct list_elem* elm);
void list_append(struct list* plist,struct list_elem* elm);
void list_remove(struct list_elem* elm);
struct list_elem* list_pop(struct list* plist);
struct list_elem* list_tail(struct list* plist);
bool list_empty(struct list* plist);
uint32_t list_len(struct list* plist);
struct list_elem* list_traversal(struct list* plist,function func,void * arg);
struct list_elem* list_reverse(struct list* plist,function func,void * arg);
bool elem_find(struct list* plist,struct list_elem* obj_elem);


#endif
