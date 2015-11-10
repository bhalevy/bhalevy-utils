#include <stdlib.h>    // for malloc
#include <strings.h>   // for bcopy
#include "sg_copy.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

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
sg_entry_t *
sg_map(void *buf, int length)
{
	int			offset = (unsigned long)buf & (PAGE_SIZE-1);
	int			sge, numsges = length/PAGE_SIZE;
    sg_entry_t	*psgl, *psge;

	// Account for partial page
	if(offset || (length & (PAGE_SIZE - 1)))
	   numsges++;

	// Do nothing if length is zero
    if(length == 0)
		return(NULL);

	// Implementation note -
	//	I am choosing to allocate the entire SGL in a single block for
	//	a few reasons.  First, it requires fewer calls to malloc (or equivalent),
	//	which is a relatively expensive routine, and second, it increases the
	//	likelihood that multiple SGEs are contained within a single cache line
	//	which means fewer CPU cache misses.  It also simplifies the destroy
	//	routine.

	// Allocate a block of memory to contain the SGL
	if((psgl = malloc(sizeof(sg_entry_t)*numsges)) == NULL) {
		// Log error?
		return(NULL);
	}
	for(sge = 0, psge = psgl; ; psge++) {
		// Convert virt addr to phys.  Assumes ptr_to_phys preserves buffer
		// offset.
		psge->paddr = ptr_to_phys(buf);
		psge->count = MIN((PAGE_SIZE-offset), length);
		sge++;
		if(sge < numsges) {
			psge->next = psge+1;
		} else {
			psge->next = NULL;
			break;
		}
		offset = 0;
		length -= psge->count;
		buf = (void *)((char *)buf + psge->count);
	}
	return(psgl);
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void
sg_destroy(sg_entry_t *sg_list)
{
	// SGLs are allocated as a contiguous block of memory in sg_map.  Just
	// free the block
	if(sg_list)
		free(sg_list);
	// else ASSERT ??.  Bad form to call with NULL..
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
int
sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int	copied = 0;
	int	dest_offset = 0;

	// Advance to the starting source entry based on the offset
	while(src_offset && src && src->count < src_offset) {
		src_offset -= src->count;
		src = src->next;
	}

	// Now copy until we have exhausted the count, or run out of src or dest SGL
	while(src && dest && count) {
		// We need to copy the smaller of either the remaining count, the
		// remaining space in the src SGE, or the remaining length in the dest
		// SGE.
		int	chunk = MIN(count, (src->count - src_offset));
		chunk = MIN(chunk, (dest->count - dest_offset));
		bcopy(phys_to_ptr(src->paddr + src_offset),
			  phys_to_ptr(dest->paddr + dest_offset),
			  chunk);
		count -= chunk;
		copied += chunk;
		if((src->count - src_offset) == chunk) {
			// Completed src SGE.  Advance
			src = src->next;
			src_offset = 0;
		} else {
			src_offset += chunk;
		}
		if((dest->count - dest_offset) == chunk) {
			// Completed dest SGE.  Advance
			dest = dest->next;
			dest_offset = 0;
		} else {
			dest_offset += chunk;
		}
	}
	return(copied);
}





