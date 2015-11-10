/*
 * A scatter-gather data type and a list of functions to implement:
 *
 * sg_map maps a memory region into a scatter gather list
 * sg_destroy destroys a scatter gather list
 * sg_copy copies 'count' bytes from src to dest starting at 'offset'
 *
 * Noam Camiel
 * Dec 27, 2011
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

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
	sg_entry_t *sg_head;	/* head of the chunk elements list */
	sg_entry_t *sg_current;	/* current chunk element */
	int buf_position;		/* current position in buffer */

// BH	if ((buf == NULL) || (length < 0)) {
	if ((buf == NULL) || (length <= 0)) {
		return NULL;
	}
	
	if (!(sg_head = (sg_entry_t *)malloc(sizeof(sg_entry_t)))) {	
		return NULL;
	}	
	sg_head->paddr = ptr_to_phys(buf);
	sg_head->next = NULL;

// BH: BUG in first entry size, single entry

//	if (length < PAGE_SIZE) {	/* entire buffer fits in a single chunk element */
//		sg_head->count = length;
//		return sg_head;
//	}

	/* alignment requirement: align entry address on PAGE_SIZE	*/
	sg_head->count = PAGE_SIZE - ((physaddr_t)buf & (PAGE_SIZE-1));
	buf_position = sg_head->count;

if (sg_head->count >= length) {	/* entire buffer fits in a single chunk element */
	sg_head->count = length;
	return sg_head;
}

	sg_current = sg_head;  
	
	/* while loop to allocate and fill the chunk elements */
	while (buf_position < length) {
		sg_entry_t *sg_next;
		if (!(sg_next = (sg_entry_t *)malloc(sizeof(sg_entry_t)))) {
			return NULL;
		}
		sg_next->paddr = ptr_to_phys((char *)buf + buf_position);
		sg_next->count = (length - buf_position) > PAGE_SIZE ? PAGE_SIZE : (length - buf_position);
		buf_position += sg_next->count;
		sg_next->next = NULL;
		
		sg_current->next = sg_next;
		sg_current = sg_next;
	}

	return sg_head;
}


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *sg_current = sg_list;
	sg_entry_t *sg_next;

	while (sg_current) {
		sg_next = sg_current->next;
		free(sg_current);
		sg_current = sg_next;
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
 *
 * impl. note	we're making no assumptions as to whether all the chunks 
 *				after the first entry are full PAGE_SIZE entries (except the last)
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) 
{
	
	sg_entry_t *sg_src_current = src;	/* current src chunk element */
	sg_entry_t *sg_dest_current = dest;	/* curretn dest chunk element */
	int src_chunk_position;				/* position within the current src chunk element */
	int dest_chunk_position = 0;		/* position within the dest chunk element */
	int total_bytes_copied = 0;			/* total bytes copied so far */
	if ((src == NULL) || (dest == NULL) || (src_offset < 0) || (count < 0)) {
		return 0;
	}

	/* first lets find the start src location */
	while (src_offset >= sg_src_current->count) {
		src_offset -= sg_src_current->count;
		sg_src_current = sg_src_current->next;
		if (sg_src_current == NULL) {
			return 0;  // src_offset exceeds src length
		}
	}		
	/* we now start from sg_src_current chunk at position within_src_position */
	src_chunk_position = src_offset; 

	/* Copy the bytes */
	while(total_bytes_copied < count) {
		/* Check we still have src bytes in our current src  chunk element */
		if (src_chunk_position >= sg_src_current->count) {  
			sg_src_current = sg_src_current->next;
			if (sg_src_current == NULL) {
				return total_bytes_copied;  // src data complete
			}
			src_chunk_position = 0;
			continue;
		}
		/* Check we still have dest bytes in our current dest chunk element */
		if (dest_chunk_position >= sg_dest_current->count) { 
			sg_dest_current = sg_dest_current->next;
			if (sg_dest_current == NULL) {
				return total_bytes_copied;  // dst room complete
			}
			dest_chunk_position = 0;
			continue;
		}

		/* do the copy */
		*((char *)phys_to_ptr(sg_dest_current->paddr) + dest_chunk_position) = *((char *)phys_to_ptr(sg_src_current->paddr)+ src_chunk_position);
		src_chunk_position++;
		dest_chunk_position++;
		total_bytes_copied++;
		
	}
	
	return total_bytes_copied;
}

