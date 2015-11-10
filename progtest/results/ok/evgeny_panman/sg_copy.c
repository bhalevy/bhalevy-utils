// BH: missing includes
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
// BH definitions should be before code (minor)
	if(length <= 0) {
		return NULL;
	}
	unsigned long current_offset = (unsigned long)buf;
	unsigned long max_offset = current_offset + length;
	unsigned long next_aligned_offset = (current_offset & ~(PAGE_SIZE - 1)) + PAGE_SIZE;

	sg_entry_t *head = NULL;
	sg_entry_t **current_p = &head;

	while(current_offset < max_offset) {
		*current_p= (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if(*current_p == NULL) {
			// do something with error
			return NULL;
		}
		(*current_p)->paddr = ptr_to_phys((void*)current_offset);
// BH: Odd loop termination condition (minor)
		if(max_offset <= next_aligned_offset) {
			/* this is last element in the list*/
			(*current_p)->next = NULL;
			(*current_p)->count = max_offset - current_offset;
			break;
		}

		(*current_p)->count = next_aligned_offset - current_offset;
		current_offset = next_aligned_offset;
		next_aligned_offset += PAGE_SIZE;
		current_p = &((*current_p)->next);
	}

	return head;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	while(sg_list != NULL) {
		sg_entry_t *next = sg_list->next;
		free(sg_list);
		sg_list = next;
	}
}

static inline void copy_single_chunk(physaddr_t dest, int dest_offset, physaddr_t src, int src_offset, int count)
{
	void *daddr = phys_to_ptr(dest + dest_offset);
	void *saddr = phys_to_ptr(src + src_offset);

	memcpy(daddr, saddr, count);
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
	if((src == NULL) || (dest == NULL) || (src_offset < 0) || (count <= 0))
	{
		/* nothing to do */
		return 0;
	}

	/* find place in src pointed by src_offset */
	while(src->count <= src_offset)
	{
		src_offset -= src->count;
		src = src->next;
		if(src == NULL)
		{
			/* offset outside of the list*/
			return 0;
		}
	}

	int total = 0;
	int src_count;
	int dest_count;
	int dest_offset = 0;
	int copy_count;

	while(count)
	{
		src_count = src->count - src_offset;
		dest_count = dest->count - dest_offset;
		
		copy_count = (src_count < dest_count) ? src_count : dest_count;
		if(copy_count >= count)
		{
			/* both chunk conatins enouth data to accomplish copy of last count bytes
				this is the last copy in the function*/
			copy_single_chunk(dest->paddr, dest_offset, src->paddr, src_offset, count);
			total += count;
			break;
		}

		copy_single_chunk(dest->paddr, dest_offset, src->paddr, src_offset, copy_count);
		count -= copy_count;
		total += copy_count;
		if(src_count == dest_count)
		{
			/* need to move both src and dest to next chunk*/
			src = src->next;
			dest = dest->next;
			if((src == NULL) || (dest == NULL))
			{
				/* no more data in one of the lists*/
				break;
			}
			src_offset = 0;
			dest_offset = 0;
		}
		else if(src_count > dest_count)
		{
			/* move dest to next chunk*/
			dest = dest->next;
			if(dest == NULL)
			{
				break;
			}
			dest_offset = 0;
			src_offset +=  copy_count;
		}
		else
		{
			/* move src to next chunk */
			src = src->next;
			if(src == NULL)
			{
				break;
			}
			src_offset = 0;
			dest_offset += copy_count;
		}

	}

	return total;
}

