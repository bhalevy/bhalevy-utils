#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

extern sg_entry_t *sg_map(void *buf, int length)
{
    sg_entry_t *sg_list = NULL;
    if (length > 0)
    {
// BH: BUG first page alignment
#if 0
        int count = length % PAGE_SIZE;
        if (count == 0)
        {
            count = PAGE_SIZE;
        }
#endif
int count = PAGE_SIZE - ((unsigned)buf & (PAGE_SIZE-1));
if (count > length) count = length;
        sg_list = (sg_entry_t *)malloc(sizeof(sg_entry_t));
        sg_list->count = count;
        sg_list->next = sg_map((buf + count), (length - count));
        sg_list->paddr = ptr_to_phys(buf);
    }
    return(sg_list);
}

extern void sg_destroy(sg_entry_t *sg_list)
{
    if (sg_list->next != NULL)
    {
        sg_destroy(sg_list->next);
    }
    free(sg_list);
}

// BH: recursion may not be optimal in this case
#if 0
int sg_copy_recurse(sg_entry_t *src, void *dest, int src_offset, int count)
{
    int copied_bytes = 0;
//    if (src != NULL)
    if (src != NULL && dest != NULL)
    {
        int copy_bytes = count;
        if (count > (src->count - src_offset))
        {
            copy_bytes = (src->count - src_offset);
// BH: BUG: must take into account dest fragmentation
            copied_bytes = sg_copy_recurse(src->next, dest + copy_bytes, 0, count - copy_bytes);
        }
//        memcpy(dest, phys_to_ptr(src->paddr) + src_offset, copy_bytes);
        memcpy(phys_to_ptr(dest) + dest_offset, phys_to_ptr(src->paddr) + src_offset, copy_bytes);
        copied_bytes = copied_bytes + copy_bytes;
    }
    return(copied_bytes);
}
#endif

int sg_copy_recurse(sg_entry_t *src, sg_entry_t *dest, int src_offset, int dest_offset, int count)
{
	int copy_bytes;

fprintf(stderr, "%s: src=%p dest=%p src_offset=%d dest_offset=%d count=%d\n", __func__, src, dest, src_offset, dest_offset, count);
	if (src == NULL || dest == NULL || count == 0)
		return 0;
	
	copy_bytes = count;
	if (copy_bytes > src->count - src_offset)
		copy_bytes = src->count - src_offset;
	if (copy_bytes > dest->count - dest_offset)
		copy_bytes = dest->count - dest_offset;
        memcpy(phys_to_ptr(dest->paddr) + dest_offset, phys_to_ptr(src->paddr) + src_offset, copy_bytes);
	src_offset += copy_bytes;
	if (src_offset >= src->count) {
		src = src->next;
		src_offset = 0;
	}
	dest_offset += copy_bytes;
	if (dest_offset >= dest->count) {
		dest = dest->next;
		dest_offset = 0;
	}
	return copy_bytes + sg_copy_recurse(src, dest, src_offset, dest_offset, count - copy_bytes);
}

/*
 * ASSUMPTIONS:
 * 1. *dest is a valid pointer. (and so are all it's ->next entries)
 * dest->paddr can be mapped to pointers which are allowed to write to.
 *
 * I can't dynamically allocate virt. memory and then map it to sg_entry_t
 * because this can cause memory leaks as I don't have a way to know that
 * I should free that memory when running sg_destroy() over this sg_entry_t
 *
 * 2. the pointer we get from phys_to_ptr(dest->paddr) is pointing on a buffer with size >= count
 *    that is: *dest must have been initialized by sg_map() where length >= count
 */
extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    if (dest == NULL)
    {
        return(0);
    }
    if (src == NULL)
    {
        return(0);
    }
    if (count == 0)
    {
        return(0);
    }
    if (src_offset > src->count)
    {
        return(sg_copy(src->next, dest, (src_offset - src->count), count));
    }
    else
    {
// BH        return(sg_copy_recurse(src, phys_to_ptr(dest->paddr), src_offset, count));
        return(sg_copy_recurse(src, dest, src_offset, 0, count));
    }
}
