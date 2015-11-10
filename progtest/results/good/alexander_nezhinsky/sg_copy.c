#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

// BH: missing definition
#define min(a, b) ((a) <= (b) ? (a) : (b))

static inline physaddr_t phys_page_off(physaddr_t paddr)
{
	return paddr & (PAGE_SIZE-1);
}

static inline physaddr_t ptr_page_off(void *p)
{
	return phys_page_off((physaddr_t)p);
}

/*
	This implementation assumes that sg lists are created
	and destroyed exclusively though sg_map() and sg_destroy()
	
	This allows allocating the list entries as consequtive
	arrays, reducing the number of malloc() and free() calls
	to just 1. 
	
	If sg lists may be allocated externally, then either 
	the external allocator follows the same policy (and this
	code remains ok), or it allocates a separate descriptor 
	per entry (and a different implementation is due).
	Anyway a single clear policy should be maintained.
	
	If the list is implemented with separate descriptor per
	entry, then sg_map() will be visually simpler
	but less efficient.
*/
sg_entry_t *sg_map(void *buf, int length)
{
	int first_off, first_len;
	int nchunks, last_len;
	sg_entry_t *sg_list;
	int this_count;
	int i = 0;
	
	if (!buf)
		return NULL;
	if (length <= 0)
		return NULL;

	/* If the first chunk is unaligned, first_len is nonzero */
	first_off = ptr_page_off(buf);
	first_len = first_off ? PAGE_SIZE - first_off : 0; /* from buf to page's end */
	if (first_len < length) {
		nchunks = (length - first_len) / PAGE_SIZE;
		if (first_len)
			nchunks ++;
		last_len = (length - first_len) % PAGE_SIZE;
		if (last_len)
			nchunks ++;
	}
	else { /* first unaligned chunk fits within a page */
		first_len = length;
		nchunks = 1;
	}
	sg_list = calloc(nchunks, sizeof(*sg_list));
	if (!sg_list)
		return NULL;
	
	this_count = first_len ? first_len : length;
	do {
		sg_list[i].paddr = ptr_to_phys(buf);
		this_count = this_count > PAGE_SIZE ? PAGE_SIZE : this_count;
		sg_list[i].count = this_count;
		sg_list[i].next = i < nchunks-1 ? &sg_list[i+1] : NULL;

		i++;
		buf += this_count;
		length -= this_count;
		this_count = length;
	} while (length);
	
	return sg_list;
}

void sg_destroy(sg_entry_t *sg_list)
{
	if (sg_list)
		free(sg_list);
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	sg_entry_t *src_list = src, *dst_list = dest;
	int dst_offset = 0, total_copy = 0, copy_cnt;

	if (!src || !dest)
		return 0;
	if (src_offset < 0 || count < 0)
		return 0;
// BH		return -1;
	if (!count)
		return 0;

	/* find the list entry that "hosts" the given offset
	   and update the offset to be relative to that entry's buffer */
	while (src_list && src_offset >= src_list->count) {
		src_offset -= src_list->count;
		src_list = src_list->next;
	}
	
	/* here src_list and dst_list track advancement thru the lists
	   and src_offset and dst_offset are always contained within
	   the corresponding current list entry's buffer */
	while (src_list && dst_list && count) {
		copy_cnt = min(src_list->count - src_offset,
			       dst_list->count - dst_offset);

		// BH - need to truncate to count
		if (copy_cnt > count)
			copy_cnt = count;

		memmove(phys_to_ptr(dst_list->paddr) + dst_offset,
			phys_to_ptr(src_list->paddr) + src_offset,
			copy_cnt);

		src_offset += copy_cnt;
		if (src_offset == src_list->count) {
			src_offset = 0;
			src_list = src_list->next;
		}
		dst_offset += copy_cnt;
		if (dst_offset == dst_list->count) {
			dst_offset = 0;
			dst_list = dst_list->next;
		}
		total_copy += copy_cnt;
		count -= copy_cnt;
	}

	return total_copy;
}


