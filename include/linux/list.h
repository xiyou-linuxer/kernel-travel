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
