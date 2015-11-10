
#include <stdlib.h>

#include "sg_copy.h"

// BUG: BH: missing min()
#define min(a, b) ((a) <= (b) ? (a) : (b))

/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elements mapping the input buffer
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */
// BUG: BH: kept header declaration
//extern sg_entry_t *sg_map(void *buf, int length);

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *obj=NULL,
		       *list_ptr=NULL, 
			   *head=NULL;
	void *ptr;

	if (buf == NULL || length == 0)
		return NULL;

	ptr = buf;
	while (ptr < buf+length) {
//		obj = malloc(sizeof(struct sg_entry_t));	// BUG: BH: struct
		obj = malloc(sizeof(sg_entry_t));
		obj->paddr = ptr_to_phys(ptr); // After second though - its enough to call ptr_to_phys only once at the beginning and adjust the next node according to this value+length (aligned to PAGE_SIZE) 
// BH: used 32 rather than PAGE_SIZE
		obj->count = min( (32 - (obj->paddr % 32)), (buf+length - ptr) ); // The (buf+length - ptr) is for the last node that might be less than 32
		obj->next = NULL;

		if (list_ptr == NULL) {
			list_ptr = obj;
			head = list_ptr;
		}
		else {
			list_ptr->next = obj;
			list_ptr = list_ptr->next;
		}

		ptr = ptr + obj->count;
	}

	return head;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */

// As i understood from the phone call with ben - i do not need to free the virtual memory that the chunks represents - only free the sg_list. On second though - its too easy - so maybe i do miss something :-)
// BUG: BH: kept header declaration
//extern void sg_destroy(sg_entry_t *sg_list);
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *prev = sg_list;

	while (sg_list != NULL) {
		sg_list = sg_list->next;
		// If a free to the virtual address should be done - it should be done here - free(phys_to_ptr(sg_list->paddr))
		free(prev);
		prev = sg_list;
	}
}

/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list
 * @in src_offset Offset into source
 * @in count     Number of bytes to copy
 *
 * @ret          Actual number of bytes copied
 *
 * @note         The function copies "count" bytes from "src",
 *               starting from "src_offset" into the beginning of "dest".
 *               The scatter gather list can be of arbitrary length so it is
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 */

// BUG: BH: kept header declaration
//extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count);
// BUG: BH: dest => dst
int sg_copy(sg_entry_t *src, sg_entry_t *dst, int src_offset, int count)
{
	int	dst_offset = 0,
	    total_copied_bytes = 0;

	while (src!=NULL && src_offset > src->count) {
		src_offset -= src->count;
		src = src->next;
	}

	/* start point to copy is src_offset in this src chunk */

	while (src != NULL && dst != NULL && count>0) {
// BUG: BH: missing "int"
		int bytes_to_copy = min( count, min(src->count-src_offset, dst->count-dst_offset));
		memcpy (phys_to_ptr(dst->paddr+dst_offset), phys_to_ptr(src->paddr+src_offset), bytes_to_copy );
		total_copied_bytes += bytes_to_copy;
		count -= bytes_to_copy;
		
		if (bytes_to_copy == src->count-src_offset) { // copied everything from this source chunk
			src = src->next;
			src_offset = 0;
		}
		else {
			src_offset += bytes_to_copy;
		}

		if (bytes_to_copy == dst->count-dst_offset) { // copied to all space in this dest chunk
			dst = dst->next;
			dst_offset = 0;
		}
		else {
			dst_offset += bytes_to_copy;
		}
	}

	return total_copied_bytes;
	
}
