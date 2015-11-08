/* memory.c */

#include "util_list.h"

/* -------------- buddy algorithm ---------------- */

/// Starting and end addrs of memory for paging
#define _MEM_START	0x300f0000
#define _MEM_END	0x30700000

#define PAGE_SHIFT	(12)
#define PAGE_SIZE	(1<<PAGE_SHIFT)	// Page size is 4KB
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define KERNEL_MEM_END (_MEM_END)

/// Starting and end addrs of paging-memory that are aligned by page size
#define KERNEL_PAGING_START	( (_MEM_START+(~PAGE_MASK)) & ((PAGE_MASK)) )
#define	KERNEL_PAGING_END ( ((KERNEL_MEM_END-KERNEL_PAGING_START)/(PAGE_SIZE+sizeof(struct page))) * (PAGE_SIZE) \
	+ KERNEL_PAGING_START )

// # of pages in the system
#define KERNEL_PAGE_NUM	((KERNEL_PAGING_END-KERNEL_PAGING_START)/PAGE_SIZE)

/// The starting and end addrs of page structures
#define KERNEL_PAGE_END	_MEM_END
#define KERNEL_PAGE_START	(KERNEL_PAGE_END - KERNEL_PAGE_NUM*sizeof(struct page))

/// Page flags
#define PAGE_AVAILABLE		0x00
#define PAGE_DIRTY			0x01
#define PAGE_PROTECT		0x02
#define PAGE_BUDDY_BUSY		0x04
#define PAGE_IN_CACHE		0x08

#define	NULL ((void *)0)


struct page {
    unsigned int vaddr;		// page starting addr (NOTE that page size is 4KB)
    unsigned int flags;
    int order;			// used by buddy algorithm; can be any positive numbers, 0, or -1
    unsigned int counter;	// how many times this page has been used
    struct kmem_cache *cachep;
    struct list_head list;	// Link together the buddies/pages
};


// Maximum # of buddy groups
// NOTE! 
// A fixed-value 9 means that the maximum # of buddy groups is 9, and
// their sizes are 
// 2^0, 2^1, ..., 2^8 pages, respectively. Thus, users can allocate 1MB
// memory at the largest, 
// i.e., 256*4KB=1MB. Those who need more than 1MB should change the macro 
// to reserve enough 
// space for private use.
#define MAX_BUDDY_PAGE_NUM	(9)

#define AVERAGE_PAGE_NUM_PER_BUDDY	(KERNEL_PAGE_NUM / MAX_BUDDY_PAGE_NUM)
#define PAGE_NUM_FOR_MAX_BUDDY	( (1<<MAX_BUDDY_PAGE_NUM) - 1 )

// List heads of different buddy groups
// NOTE! 
// Array index is the order of a buddy group. E.g., page_buddy[5] means
// all 
// buddies of size 2^5=32 pages
struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];

/*
 * Convert a virtual addr to a pointer to struct page 
 */
struct page *virt_to_page(unsigned int addr)
{
    unsigned int i;

    i = (addr - KERNEL_PAGING_START) >> PAGE_SHIFT;

    if (i > KERNEL_PAGE_NUM)
	return NULL;

    return (struct page *) KERNEL_PAGE_START + i;
}

/*
 * Initialize list heads of each buddy group 
 */
void init_page_buddy(void)
{
    int i;

    for (i = 0; i < MAX_BUDDY_PAGE_NUM; i++) {
	INIT_LIST_HEAD(&page_buddy[i]);
    }
}

/*
 * Initialize each buddy group NOTE! 1. After initialization, the system
 * only has two buddy groups, one with the largest buddies and the other
 * smallest buddies. 2. Each buddy consists of one or more pages, and is
 * represented by the header "page" struct. The field "order" is used to
 * distinguish the header struct "page" and the other struct "page"'s in a 
 * buddy. For each buddy, the field "order" in the header "page" struct is 
 * set to the corresponding order, and those in others are set to -1. 
 */
void init_page_map(void)
{
    int i;
    // NOTE!
    // KERNEL_PAGE_START is the starting addr of memory block for storing
    // struct "page"'s
    struct page *pg = (struct page *) KERNEL_PAGE_START;

    init_page_buddy();

    for (i = 0; i < KERNEL_PAGE_NUM; pg++, i++) {
	/// First, fill struct "page"
	pg->vaddr = KERNEL_PAGING_START + i * PAGE_SIZE;
	pg->flags = PAGE_AVAILABLE;
	pg->counter = 0;
	INIT_LIST_HEAD(&(pg->list));

	// / Make each buddy as large as possible
	if (i < (KERNEL_PAGE_NUM & (~PAGE_NUM_FOR_MAX_BUDDY))) {
	    // / Each buddy consists of one or more pages, and is
	    // represented by the header "page" struct.
	    // / The field "order" is used to distinguish the header
	    // struct "page" and the other struct
	    // / "page"'s in a buddy. For each buddy, the field "order" in 
	    // the header "page" struct is 
	    // / set to the corresponding order, and those in others are
	    // set to -1.
	    if ((i & PAGE_NUM_FOR_MAX_BUDDY) == 0) {
		pg->order = MAX_BUDDY_PAGE_NUM - 1;
	    } else {
		pg->order = -1;
	    }

	    list_add_tail(&(pg->list),
			  &page_buddy[MAX_BUDDY_PAGE_NUM - 1]);

	} else {
	    // / The remainder is not big enough to merge into a max
	    // buddy; they are 
	    // / treated as buddies of minimum size
	    pg->order = 0;
	    list_add_tail(&(pg->list), &page_buddy[0]);
	}

    }
}


// / We can do these all because the page structure that represents one
// page area is continuous
#define BUDDY_END(x,order)	((x)+(1<<(order))-1)
#define NEXT_BUDDY_START(x,order)	((x)+(1<<(order)))
#define PREV_BUDDY_START(x,order)	((x)-(1<<(order)))

/*
 * Request a buddy of 2^order pages
 * 
 * Case 1): Found an empty buddy under page_buddy[order] Case 2):
 * Otherwise search upwards for bigger buddies. If an empty buddy is
 * found, divide it into two equal buddies, one is allocated and the
 * other returned to the system 
 */
struct page *get_pages_from_list(int order)
{
    unsigned int vaddr;
    int neworder = order;
    struct page *pg, *ret;
    struct list_head *tlst, *tlst1;

    for (; neworder < MAX_BUDDY_PAGE_NUM; neworder++) {
	if (list_empty(&page_buddy[neworder])) {
	    continue;
	} else {
	    pg = list_entry(page_buddy[neworder].next, struct page, list);
	    tlst = &(BUDDY_END(pg, neworder)->list);
	    tlst->next->prev = &page_buddy[neworder];
	    page_buddy[neworder].next = tlst->next;

	    goto OUT_OK;
	}
    }

    return NULL;		// No available buddies of this size

  OUT_OK:
    // / The starting value of the loop variable is: (the order of the
    // allocated buddy - 1) 
    for (neworder--; neworder >= order; neworder--) {
	tlst1 = &(BUDDY_END(pg, neworder)->list);
	tlst = &(pg->list);
	pg = NEXT_BUDDY_START(pg, neworder);
	list_entry(tlst, struct page, list)->order = neworder;
	list_add_chain_tail(tlst, tlst1, &page_buddy[neworder]);
    }


    pg->flags |= PAGE_BUDDY_BUSY;
    pg->order = order;

    return pg;
}

/*
 * Return a buddy of 2^order pages
 * 
 * Case 1): The returning buddy under cannot be merged with adjacent buddies 
 * Case 2): Otherwise, merge and add the resultant buddy to a new
 * buddy group; repeating this process until no merging can occur 
 */
void put_pages_to_list(struct page *pg, int order)
{
    struct page *tprev, *tnext;

    if (!(pg->flags & PAGE_BUDDY_BUSY)) {
		printk("Error: realeasing a page that was not allocated at all!\n");
		return;
    }

    pg->flags &= ~(PAGE_BUDDY_BUSY);

    // / To merge with the buddy immediately before/after the releasing
    // buddy, the two buddies should
    // / have the same order, in addition to the same flags indicating
    // that they are not in use
    for (; order < MAX_BUDDY_PAGE_NUM; order++) {
	// / The struct "page" addrs of the buddies immediately before and 
	// after the releasing buddy 
	tnext = NEXT_BUDDY_START(pg, order);
	tprev = PREV_BUDDY_START(pg, order);

	if ((!(tnext->flags & PAGE_BUDDY_BUSY)) && (tnext->order == order)) {
	    pg->order++;
	    tnext->order = -1;
	    list_remove_chain(&(tnext->list),
			      &(BUDDY_END(tnext, order)->list));
	    BUDDY_END(pg, order)->list.next = &(tnext->list);
	    tnext->list.prev = &(BUDDY_END(pg, order)->list);
	    continue;

	} else if ((!(tprev->flags & PAGE_BUDDY_BUSY))
		   && (tprev->order == order)) {
	    pg->order = -1;

	    list_remove_chain(&(pg->list), &(BUDDY_END(pg, order)->list));
	    BUDDY_END(tprev, order)->list.next = &(pg->list);
	    pg->list.prev = &(BUDDY_END(tprev, order)->list);

	    pg = tprev;
	    pg->order++;
	    continue;

	} else {
	    break;
	}
    }

    list_add_chain(&(pg->list), &((tnext - 1)->list), &page_buddy[order]);
}

/*
 * Return the page addr that a struct "page" represents 
 */
void *page_address(struct page *pg)
{
    return (void *) (pg->vaddr);
}

/*
 * Request a buddy of 2^order pages, and set flags in each page to "flag" 
 * if succeeded
 * 
 * NOTE! The parameter "flag" is reserved for future use. 
 */
struct page *alloc_pages(unsigned int flag, int order)
{
    struct page *pg;
    int i;

    pg = get_pages_from_list(order);

    if (pg == NULL) {
		return NULL;
	} 

    for (i = 0; i < (1 << order); i++) {
	(pg + i)->flags |= PAGE_DIRTY;
    }

    return pg;
}


/*
 * Return a buddy of 2^order pages, and set flags in each page to xxx 
 */
void free_pages(struct page *pg, int order)
{
    int i;

    for (i = 0; i < (1 << order); i++) {
	(pg + i)->flags &= ~PAGE_DIRTY;
    }

    put_pages_to_list(pg, order);
}

/*
 * Request a buddy of 2^order pages, and set flags in each page to "flag" 
 * if succeeded
 * 
 * Return value: the page addr of the requested buddy
 * 
 * NOTE! The parameter "flag" is reserved for future use. 
 */
void *get_free_pages(unsigned int flag, int order)
{
    struct page *page;

    page = alloc_pages(flag, order);

    if (!page) { return NULL; }

    return page_address(page);
}

/*
 * Return a buddy of 2^order pages, staring from address "addr" 
 */
void put_free_pages(void *addr, int order)
{
    free_pages(virt_to_page((unsigned int) addr), order);
}


/* ----------- slab Implementation ------------- */

// NOTE that an slab cache can only contain one or more buddies of the same size
struct kmem_cache {
    unsigned int obj_size;	 // size (in bytes) of each memory block 
    unsigned int obj_num;	 // # of available memory blocks
    unsigned int page_order; // order of each buddy
    unsigned int flags;
    struct page *head_page;	 // pointer to the starting page's struct of the cache 
    struct page *end_page;	 // pointer to the end page's struct of the cache 
    void *nf_block;		     // pointer to the available memory block in the cache 
};

// Default memory block size of an slab cache is 2^0=1 page
#define KMEM_CACHE_DEFAULT_ORDER	(0)
// Maximum memory block size of an slab cache is 2^5=32 pages
#define KMEM_CACHE_MAX_ORDER		(5)	 

#define KMEM_CACHE_SAVE_RATE		(0x5a) // 0x5a=90
#define KMEM_CACHE_PERCENT			(0x64) // 0x64=100
// The maximum space of a page can be wasted 
#define KMEM_CACHE_MAX_WAST    (PAGE_SIZE - KMEM_CACHE_SAVE_RATE*PAGE_SIZE/KMEM_CACHE_PERCENT)

/* Based on the size of memory blocks in the slab cache, get the order of 
   the constituent buddies. 
*/
int find_right_order(unsigned int size)
{
    int order;
    
   	// NOTE! size/buddy_size should by less than a percentage 
	for(order=0; order<=KMEM_CACHE_MAX_ORDER; order++) {
		if(size <= (KMEM_CACHE_MAX_WAST)*(1<<order)) {
	    	return order;
		}
    }
    
	if(size > (1<<order)) { return -1; }

	return order;
}

/* Initialize the slab cache. In particular, divide the slab cache into mutiple
 * memory blocks; the starting addr of each memory block stores the starting addr 
 * of the next available memory block, with the exception of the last memory block, 
 * which stores NULL. 
 * 
 * @Parameters: "head" is pointer to the slab cache memory; "size" is memory block
 *  size; "order" is the buddy order in the slab cache.
 * 
 * @Return value: # of available memory blocks in the slab cache
*/
int kmem_cache_line_object(void *head, unsigned int size, int order)
{
    void **pl;
    char *p;
    
	pl = (void **) head;
    p = (char *) head + size;
    
	int i, s = PAGE_SIZE * (1<<order);
    
	for(i=0; s>size; i++, s-=size) {
		*pl = (void *) p;
		pl = (void **) p;
		p = p + size;
    }
    
	if(s == size) { i++; }
	
	return i;
}

/* Allocate an slab cache
 * 
 * @Parameters "size" and "flags": size and flags of a memory block in the cache  
 * 
 * NOTE
 * After the initialization, only one buddy exists (XXX?). Depending on future usage, buddies
 * are added on the fly.
*/
struct kmem_cache *kmem_cache_create(struct kmem_cache *cache, 
					unsigned int size, unsigned int flags)
{
    void **nf_block = &(cache->nf_block);
	
	// Based on the size of memory blocks, get the order of buddies of the slab cache
    int order = find_right_order(size);
    if(order == -1)
		return NULL;
  	 
	// "0" is flags value
	if((cache->head_page=alloc_pages(0,order))==NULL)
		{ return NULL; }
	*nf_block = page_address(cache->head_page);

    cache->obj_num = kmem_cache_line_object(*nf_block, size, order);
    
	cache->obj_size = size;
    cache->page_order = order;
    cache->flags = flags;
    cache->end_page = BUDDY_END(cache->head_page, order);
    cache->end_page->list.next = NULL;

    return cache;
}

/* Release an slab cache */
void kmem_cache_destroy(struct kmem_cache *cache)
{
    int order = cache->page_order;
    struct page *pg = cache->head_page;
    struct list_head *list;
    
	while(1) {
		list = BUDDY_END(pg, order)->list.next;
		
		free_pages(pg, order);
		
		if(list) {
	    	pg = list_entry(list, struct page, list);
		} else {
	    	return;
		}
    }

}

/* Allocate a memory block from the slab cache 
 
 NOTE
 When allocating a memory block, there are two cases.
 1) An empty memory block is available
 2) No available memory blocks are available
    We need to call alloc_pages to get a new buddy group.
*/
void *kmem_cache_alloc(struct kmem_cache *cache, unsigned int flag)
{
    void *p;
    struct page *pg;
    
	if(cache == NULL) return NULL;
    
	void **nf_block = &(cache->nf_block);
    unsigned int *num = &(cache->obj_num);
    int order = cache->page_order;

    if(!*num) {
		if((pg = alloc_pages(0, order)) == NULL) { 
			return NULL; 
		}	
		
		*nf_block = page_address(pg);
		cache->end_page->list.next = &pg->list;
		cache->end_page = BUDDY_END(pg, order);
		cache->end_page->list.next = NULL;
		*num += kmem_cache_line_object(*nf_block, cache->obj_size, order);
    }
	
    
	(*num)--;
    p = *nf_block;
    *nf_block = *(void **) p;
    pg = virt_to_page((unsigned int) p);
    pg->cachep = cache;		
    
	return p;
}

/* Return a memory block to the slab cache 
 * 
 * NOTE  When not having enough memory, we do not scan the cache and return 
 * the unused memory to the system, i.e., the cache memory is only returned 
 * to the system when the slab cache is destroied.
*/
void kmem_cache_free(struct kmem_cache *cache, void *objp)
{
    *(void **) objp = cache->nf_block;
    cache->nf_block = objp;
    cache->obj_num++;
}

/* ----------------- kmalloc Implementation ------------------------ */

#define KMALLOC_BIAS_SHIFT			(5)		// 32 byte minimal
#define KMALLOC_MAX_SIZE			(4096)
#define KMALLOC_MINIMAL_SIZE_BIAS	(1<<(KMALLOC_BIAS_SHIFT))
#define KMALLOC_CACHE_SIZE			(KMALLOC_MAX_SIZE/KMALLOC_MINIMAL_SIZE_BIAS)

struct kmem_cache kmalloc_cache[KMALLOC_CACHE_SIZE] = { {0, 0, 0, 0, NULL, NULL, NULL}, };

#define kmalloc_cache_size_to_index(size)	((((size))>>(KMALLOC_BIAS_SHIFT)))

/* Initialize kmalloc_cache[] */
int kmalloc_init(void)
{
    int i = 0;

    for(i=0; i<KMALLOC_CACHE_SIZE; i++) {
		if(kmem_cache_create(&kmalloc_cache[i], (i+1)*KMALLOC_MINIMAL_SIZE_BIAS, 0) == NULL) {
	    	return -1;
    	}
	}
    
	return 0;
}

/* Allocate a "size"-byte memory */
void *kmalloc(unsigned int size)
{
    int index = kmalloc_cache_size_to_index(size);
    
	if(index >= KMALLOC_CACHE_SIZE)
		return NULL;
    
	return kmem_cache_alloc(&kmalloc_cache[index], 0);
}

/* Free the memory starting from "addr"  */
void kfree(void *addr)
{
    struct page *pg;
   	/// Get the struct "page" that "addr" corresponds to, then 
	/// get the struct "kmem_cache" this struct "page" corresponds to by the
	/// member "cachep", then invoke "kmem_cache_free" to free the memory
	pg = virt_to_page((unsigned int) addr);
    kmem_cache_free(pg->cachep, addr);
}

