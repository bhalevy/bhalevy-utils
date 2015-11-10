#include <stdlib.h>
#include "sg_copy.h"

/*

assumptions:

1. ptr_to_phys and phys_to_ptr functions are given.
2. the buf passed to sg_map is on a page boundary.
3. a function (phys_copy) to copy between physical pages is assumed
   to be given. it returns the number of bytes actually copied

*/

extern int phys_copy(physaddr_t src, physaddr_t dest, int len);	// BH: why introduce this and not use memcpy over phys_to_ptr?

#define MIN(a,b) ((a < b) ? a : b) 

/*
 * map a buffer by splitting it up into page size blocks.
 * it is assumed that the buffer starts on a page boundary
 */
sg_entry_t *sg_map(void *buf, int length)
{
	void *curr = buf;
	void *end = buf + length; 
	sg_entry_t *sg_list, *entry;

	// if the buffer to map does not start on a page boundary or the
	// specified length is 0 we bail out.
	if ((unsigned long)buf % PAGE_SIZE || length == 0)	// BH: not part of the specification
		return NULL;

	sg_list = entry = malloc(sizeof(sg_entry_t));

	for (;;) {
		
		if (!entry) {
			sg_destroy(sg_list);
			return NULL;
		}
	
		entry->paddr = ptr_to_phys(curr);
		entry->count = MIN(end - curr, PAGE_SIZE);

		curr += PAGE_SIZE;
		if (curr >= end) {
			entry->next = NULL;
			break;
		}

		entry->next = malloc(sizeof(sg_entry_t));	// BH: weird iteration
		entry = entry->next;				// BH: BUG, malloc may fail
	}
	return sg_list;
}

/*
 * destroy an sg_list. free all the lists entries.
 */
void sg_destroy(sg_entry_t* sg_list)
{
	sg_entry_t* curr = sg_list;

	while (curr) {
		sg_entry_t* del_entry = curr;
		curr = curr->next;
		free(del_entry);
	}
}


/*
 * copy count bytes from from offset in the src sg list to the dest sg 
 * list (at the begining of dest). 
 * returns the number of bytes actually copied
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int offset, int count)
{
	int curr_offset = offset;
	int count_left = count;
	int copy_size;
	sg_entry_t *curr_src = src;
	sg_entry_t *curr_dest = dest;	

	// find the entry to start copying from
	for (; curr_src && curr_offset >= PAGE_SIZE; 
	     curr_src = curr_src->next, curr_offset -= PAGE_SIZE);

	if (!curr_src)
		return 0; // offset is larger than the src buffer
		
	copy_size = MIN(curr_src->count - curr_offset, count_left);
	if (copy_size <= 0) // if offset is larger or equal to the first pages count
		return 0;
	
	while (count_left && curr_src && curr_dest) {
		copy_size = MIN(curr_src->count - curr_offset, count_left);	// BH: mine
		
		// phys_copy is a function to copy data between physical pages
		// which is assumed to be implemented.
#if 0
		int actual_copy = phys_copy(curr_src->paddr + curr_offset, 
					    curr_dest->paddr, copy_size);
#endif
		int actual_copy = copy_size;
		memcpy((char *)phys_to_ptr(curr_dest->paddr),
			(char *)phys_to_ptr(curr_src->paddr) + curr_offset,
			copy_size);	// BH: mine
		count_left -= actual_copy;
		if (actual_copy != copy_size)
			break; // the physical copy failed to copy the full length so we bail.

		curr_src = curr_src->next;
		curr_dest = curr_dest->next;			// BH: bug, dest increments by copy_size
		curr_offset = 0;				// BH: mine
//		copy_size = MIN(curr_src->count, count_left);	// BH: BUG
	}
	return count - count_left;
}



