#include <stdlib.h>	/* malloc() */
#include <string.h>	/* memcpy() */
#include "sg_copy.h"


/* normally PAGE_SIZE would be defined more along these lines to facilitate
 * shifts-based divs/mods. in this case _PAGE_SIZE is renamed to start with
 * underscore to avoid colliding with the original PAGE_SIZE from sg_copy.h */
#define PAGE_SHIFT      5
#define _PAGE_SIZE      (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))


/* min() macro lifted from <linux/kernel.h> would of course be much safer,
 * nicer, and side effect free, but it's not ANSI-C compliant */
#define min(x, y) ((x) < (y) ? (x) : (y))


/**
 * Copies memory area like memcpy() but using physical addresses.
 *
 * @in dest     Physical address of the destination memory area.
 * @in src      Physical address of the source memory area.
 * @in count    Number of bytes of memory to copy from src to dest.
 *
 * @ret         Physical address of the destination.
 *
 * @note        Since we're "playing pretend", this function merely
 *              converts "physicall addresses" to actual pointers
 *              using phys_to_ptr(), then delegates to plain vanilla
 *              memcpy(). Presumably a real platform would either
 *              have an equivalent API entry point or would have a
 *              more effective way of copying physical memory
 *              contents. In the worst case scenario a simple loop
 *              would have done the job too.
 *
 * @warning     In this implementation all the constraints of memcpy(),
 *              including overlap-related ones, apply as well.
 */
physaddr_t physmemcpy(physaddr_t dest, physaddr_t src, int count)
{
	memcpy(phys_to_ptr(dest), phys_to_ptr(src),  count);
	return dest;
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
	sg_entry_t	*head = NULL;
	sg_entry_t	**entry = &head;
	physaddr_t	paddr;
	int		curr_size = 0;
	unsigned char	*curr_start = (unsigned char *)buf;

	if (!buf || !length)
		return NULL;

	while (length) {
		paddr = ptr_to_phys(curr_start);

		*entry = malloc(sizeof(sg_entry_t));
		if (!*entry) {
			sg_destroy(head);
			return NULL;
		}

		(*entry)->paddr = paddr;
		/* ensure up to page-sized chunks satisfying alignment: */
		if (((paddr + length) >> PAGE_SHIFT) > (paddr >> PAGE_SHIFT))
			curr_size = PAGE_SIZE - (paddr & ~PAGE_MASK);
		else
			curr_size = length;
		(*entry)->count = curr_size;
		(*entry)->next = NULL;

		curr_start += curr_size;
		length -= curr_size;
		entry = &(*entry)->next;
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
	sg_entry_t *prev;
	while (sg_list) {
		prev = sg_list;
		sg_list = sg_list->next;
		free(prev);
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
 * @warning      Actual source and destination of this operation should not
 *               overlap (much as with memcpy).
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copied = 0;
	int curr_src_offset = 0;
	int curr_dest_offset = 0;

	if (!src || !dest || !count)
		return 0;

	/* skip to entry that contains the requested offset: */
	while (src && src_offset > src->count) {
		src_offset -= src->count;
		src = src->next;
	}
	if (!src || src_offset > src->count) {
		/* out-of-bounds src_offset requested */
		return 0;
	}
	curr_src_offset = src_offset;

	/* actually copy the data: */
	while (copied < count && src && dest) {
		int curr_dest_free = dest->count - curr_dest_offset;
		int curr_src_left = min(src->count - curr_src_offset,
				count - copied);
		int curr_op_count = min(curr_src_left, curr_dest_free);

		physmemcpy(dest->paddr + curr_dest_offset,
				src->paddr + curr_src_offset, curr_op_count);
		copied += curr_op_count;
		curr_src_offset += curr_op_count;
		curr_dest_offset += curr_op_count;

		/* advance to next src/dest SG entries if necessary: */
		if (curr_src_offset == src->count) {
			src = src->next;
			curr_src_offset = 0;
		}
		if (curr_dest_offset == dest->count) {
			dest = dest->next;
			curr_dest_offset = 0;
		}
	}

	return copied;
}
