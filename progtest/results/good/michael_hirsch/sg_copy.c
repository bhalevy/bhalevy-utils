/*
 * sg_copy.c
 *
 * Tutorial implementation of scatter/gather with mapped memory
 *
 * Michael Hirsch
 * 2011-10-04
 */

#include <stdlib.h>		/* for malloc and free */
#include <string.h>		/* for memcpy */
#include "sg_base.h"
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
	sg_entry_t	*first = NULL;
	sg_entry_t	*last  = NULL;
	int		count  = 0;
	byte		*pBuf  = (byte *) buf;

	if ((buf == NULL) || (length <= 0)) {
		return NULL;
	}

	/* prepare first entry and first count (differ from rest) */

	/* This code only works if PAGE_SIZE is a power of 2 */
	count = PAGE_SIZE - ((unsigned long) buf & (PAGE_SIZE - 1));
	if (count > length) {
		count = length;
	}

	first = last = (sg_entry_t *) malloc(sizeof(sg_entry_t));
	if (first == NULL) {
		return NULL;
	}

	for (;;) {

		last->paddr = ptr_to_phys(pBuf);
		last->count = count;

		pBuf += count;
		length -= count;

		/* loop exit condition */
		if (length == 0) {
			/* finished */
			last->next  = NULL;
			break;
		}

		/* prepare the count and the next entry */
		count = PAGE_SIZE;
		if (count > length) {
			count = length;
		}

		last->next = (sg_entry_t *) malloc(sizeof(sg_entry_t));
		if (last->next == NULL) {
			sg_destroy(first);
			return NULL;
		}
		last = last->next;

	}

	return first;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t	*next = NULL;

	while (sg_list != NULL) {
		next = sg_list->next;
		free(sg_list);
		sg_list = next;
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
	byte	*pSrc;
	byte	*pDest;
	int	to_copy;
	int	dest_offset;
	int	copied;
	int	src_count;
	int	dest_count;

	if ((src == NULL) ||
	    (dest == NULL) ||
	    (src_offset < 0) ||
	    (count <= 0)) {
		return 0;
	}

	/*
	 * Skip entries before the start offset
	 * Before this loop, src_offset is absolute offset into entire src.
	 * After this loop, src points to the entry that contains the first
	 * live data and src_offset is relative to the start of this entry.
	 */
	while ((src != NULL) && (src_offset >= src->count)) {
		src_offset -= src->count;
		src = src->next;
	}
	if (src == NULL) {
		/* skipped more bytes than were available in src */
		return 0;
	}

	copied = 0;

	/* prepare the pointers from the physical addresses */
	pSrc = (byte *) phys_to_ptr(src->paddr);
	/* current value of src_offset is correct */

	pDest = (byte *) phys_to_ptr(dest->paddr);
	dest_offset = 0;

	for (;;) {
		/*
		 * How many bytes before:
		 * - end of data
		 * - end of source entry
		 * - end of destination entry
		 * This value will be in to_copy.
		 */

		to_copy = count;
		src_count = src->count - src_offset;
		if (to_copy > src_count) {
			to_copy = src_count;
		}
		dest_count = dest->count - dest_offset;
		if (to_copy > dest_count) {
			to_copy = dest_count;
		}
		
		/* do the copy */
		memcpy(pDest + dest_offset, pSrc + src_offset, to_copy);
		copied += to_copy;
		count -= to_copy;
		if (count == 0) {
			/* copied all the bytes we need to */
			break;
		}

		src_offset += to_copy;
		if (src_offset == src->count) {
			/* finished with this entry, prepare the next */
			src = src->next;
			if (src == NULL) {
				/* no more source bytes */
				break;
			}
			pSrc = (byte *) phys_to_ptr(src->paddr);
			src_offset = 0;
		}

		dest_offset += to_copy;
		if (dest_offset == dest->count) {
			/* filled all the bytes in this entry, prepare next */
			dest = dest->next;
			if (dest == NULL) {
				/* no more place to put bytes */
				break;
			}
			pDest = (byte *) phys_to_ptr(dest->paddr);
			dest_offset = 0;
		}

	}

	return copied;
}

