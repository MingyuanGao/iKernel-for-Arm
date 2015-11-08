/* util_list.h */

#ifndef _UTIL_LIST_H_
#define _UTIL_LIST_H_

struct list_head {
    struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void
__list_add(struct list_head *new_lst, struct list_head *prev,
	   struct list_head *next)
{
    next->prev = new_lst;
    new_lst->next = next;
    new_lst->prev = prev;
    prev->next = new_lst;
}

static inline void
list_add(struct list_head *new_lst, struct list_head *head)
{
    __list_add(new_lst, head, head->next);
}

static inline void
list_add_tail(struct list_head *new_lst, struct list_head *head)
{
    __list_add(new_lst, head->prev, head);
}

static inline void
__list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline void
list_remove_chain(struct list_head *ch, struct list_head *ct)
{
    ch->prev->next = ct->next;
    ct->next->prev = ch->prev;
}

static inline void
list_add_chain(struct list_head *ch,
	       struct list_head *ct, struct list_head *head)
{
    ch->prev = head;
    ct->next = head->next;
    head->next->prev = ct;
    head->next = ch;
}

static inline void
list_add_chain_tail(struct list_head *ch,
		    struct list_head *ct, struct list_head *head)
{
    ch->prev = head->prev;
    head->prev->next = ch;
    head->prev = ct;
    ct->next = head;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}


// Given a struct "TYPE", get the offset of its member "MEMBER" from its
// starting addr 
#define offsetof(TYPE, MEMBER) ((unsigned int) &((TYPE *)0)->MEMBER)

// "type" should be the struct type that contains a member of type "list_head"
// "ptr" is a pointer to struct "list_header"
// "member" is the "list_head"-type member name in struct "type"
// NOTE! The use of __mptr can force the macro do a type checking, see
// P153 for more details.
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})

// Given a pointer to the member "member" of struct "type", get the addr of struct "type" 
// For example, given a pointer to the member "list" of struct "page", get the addr of struct 
// "page" (addr of member "list" - 4*sizeof(int) )
#define list_entry(ptr, type, member) container_of(ptr, type, member)

// Travese the list
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)


#endif // _UTIL_LIST_H_
