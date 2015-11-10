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
	unsigned char *pBuffIt = (unsigned char *)buf;
	sg_entry_t *pFirstEnt;
	/* pEntryIt initially point to the head of the list */
	sg_entry_t **pEntryIt = &pFirstEnt;
	/* from ptr_to_phys() I can tell that PAGE_SIZE is power of 2,
	   otherwise % can be used */
// BH: BUG
//	int len = (int)buf & (PAGE_SIZE - 1);
//	if (len == 0) {
//		len = PAGE_SIZE;
//	}
int len = PAGE_SIZE - ((unsigned long)buf & (PAGE_SIZE - 1));
	while (length > 0) {
		if (len > length) {
			len = length;
		}
		*pEntryIt = (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if (*pEntryIt == NULL) {
			/* *pEntryIt is NULL, it terminates the list */
			sg_destroy(pFirstEnt);
			return NULL;
		}
		(*pEntryIt)->paddr = ptr_to_phys(pBuffIt);
		pBuffIt+= len;
		(*pEntryIt)->count = len;
		pEntryIt = &((*pEntryIt)->next);
		length-= len;
		len = PAGE_SIZE;
	}
	/* add termination */
	*pEntryIt = NULL;
	return pFirstEnt;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *pToDelete;
	while (sg_list != NULL) {
		pToDelete = sg_list;
		sg_list = sg_list->next;
		free(pToDelete);
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
	int copied = 0;
	int destInEntryOffset = 0;
	int copyCount;

// BH
if (count <= 0 || src_offset < 0) return 0;

	/* iterate src until the copy start entry */
	while (src_offset > 0 && src != NULL) {
		if (src_offset < src->count) {
			break;
		}
		src_offset-= src->count;
		src = src->next;
	}
	/* from now and on src_offset is used as srcInEntryOffset */
	while (src != NULL && dest != NULL && count > 0) {
		/* calculate number of bytes that can be copied at this time */
		copyCount = dest->count - destInEntryOffset;
		if (copyCount > src->count - src_offset) {
			copyCount = src->count - src_offset;
		}
		if (copyCount > count) {
			copyCount = count;
		}
		memcpy((char *)phys_to_ptr(dest->paddr) + destInEntryOffset, (char *)phys_to_ptr(src->paddr) + src_offset, copyCount);
		/* update counters */
		copied+= copyCount;
		count-= copyCount;
		destInEntryOffset+= copyCount;
		if (dest->count == destInEntryOffset) {
			destInEntryOffset = 0;
			/* advance dest to next non-empty entry */
			do {
				dest = dest->next;
			} while(dest != NULL && dest->count == 0);
		}
		src_offset+= copyCount;
		if (src->count == src_offset) {
			src_offset = 0;
			/* advance src to next non-empty entry */
			do {
				src = src->next;
			} while (src != NULL && src->count == 0);
		}
	}
	return copied;
}
