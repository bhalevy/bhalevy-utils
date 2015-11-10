
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sg_copy.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))


/*
 * sg_map_single     Map a memory buffer to a single sg_entry
 *
 * @in buf           Pointer to buffer
 * @in length        Buffer length in bytes
 *
 * @ret              A newly allocated sg_entry element mapping the input 
 *                   buffer
 *
 * @note             Mapping is done to the min between the length of the
 *                   buffer and the available bytes in the physical page.
 */
sg_entry_t *sg_map_single(void *buf, int length)
{
	sg_entry_t *sg_ptr;
	
	if (!buf || !length)
	{
		return NULL;
	}
	
	/* sllocate sg entry */
	sg_ptr = malloc(sizeof(sg_entry_t));
	if (!sg_ptr) {
		return NULL;
	}
	
	/* update physical address */
	sg_ptr->paddr = ptr_to_phys(buf);
	/* update mapped bytes count to be the min between 
	 * the length of the buffer and the remaining bytes in the page
	 */
	sg_ptr->count = MIN(length, 
						(PAGE_SIZE - (sg_ptr->paddr & (PAGE_SIZE-1))));
	sg_ptr->next = NULL;
	
	return sg_ptr;
}

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
sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg_list;
	sg_entry_t *sg_ptr;
	char* buf_ptr = buf;
	int count;
	
	if (!buf || !length)
		return NULL;
	
	/* First map the head of the list.
	 * This is required to be able to return the head of the list
	 * at the function return
	 */
	sg_list = sg_map_single(buf_ptr, length);
	if (!sg_list)
		return NULL;
	
	/* update length and buf_ptr */
	length -= sg_list->count;
	buf_ptr += sg_list->count;			
	
	sg_ptr = sg_list;
	while (length) {
		/* map next element in sg list 
		 * if mapping fails for any reason, entire SG list should be freed 
		 */
		sg_ptr->next = sg_map_single(buf_ptr, length);
		if (!sg_ptr->next) {
			sg_destroy(sg_list);
			return NULL;
		}
		
		/* move pointer to newly allocated entry 
		 * and update length and buf_ptr 
		 */
		sg_ptr = sg_ptr->next;
		length -= sg_ptr->count;
		buf_ptr += sg_ptr->count;			
	}
	
	return sg_list;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *sg_ptr;
	
	/* go over entire sg list and free all entries */
	while (sg_list) {
		sg_ptr = sg_list->next;
		free(sg_list);
		sg_list = sg_ptr;
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copy_count = 0;
	int dest_offset = 0;
	char* src_ptr;
	char* dest_ptr;
	int bytes_to_copy;
	
	/* find copy start point 
	 * search source sg list for descriptor that holds start of data to copy 
	 */
	while (src && src_offset && (src_offset >= src->count)) {
		src_offset -= src->count;
		src = src->next;
	}
	
	while (src && dest && count) {
		/* find source and destination virtual addresses (including offset) */
		src_ptr = phys_to_ptr(src->paddr);
		src_ptr+= src_offset;
		dest_ptr = phys_to_ptr(dest->paddr);
		dest_ptr += dest_offset;
		
		/* calculate copy size according to remaining bytes in source, 
		 * destination and byte count
		 */ 
		bytes_to_copy = MIN((src->count - src_offset), 
							(dest->count - dest_offset));
		bytes_to_copy = MIN(count, bytes_to_copy);
		
		memcpy(dest_ptr, src_ptr, bytes_to_copy);
		
		/* move to next source descriptor if copied all descriptor data */
		src_offset += bytes_to_copy;
		if (src_offset == src->count) {
			src_offset = 0;
			src = src->next;
		}
		
		/* move to next destination descriptor if copied all descriptor data */
		dest_offset += bytes_to_copy;
		if (dest_offset == dest->count) {
			dest_offset = 0;
			dest = dest->next;
		}
		
		/* update byte counters */
		copy_count += bytes_to_copy;
		count -= bytes_to_copy;
	}
	
	return copy_count;
}

