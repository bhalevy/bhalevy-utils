/*
 * Scatter/gather list - Implementation
 *
 * January 30, 2012 Dave Stav <dave@stav.org.il>
 *
 * Tonian Systems Inc.
 */

#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#define MIN(a, b)	((a < b) ? a : b)
#define MIN3(a, b, c)	((a < b) ? MIN(a, c) : MIN(b, c))

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg;		/* sg_list result */

	if (!buf || length <= 0)
		return NULL;

	if (!(sg = malloc(sizeof(*sg))))
		return NULL;

	sg->paddr = ptr_to_phys(buf);
	sg->count = PAGE_SIZE - sg->paddr % PAGE_SIZE;
	if (sg->count >= length) {
		sg->count = length;
		sg->next = NULL;
	} else {
		sg->next = sg_map(buf + sg->count, length - sg->count);
	}

	return sg;
}

void sg_destroy(sg_entry_t *sg_list)
{
	if (!sg_list)
		return;
	sg_destroy(sg_list->next);
	free(sg_list);
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int len;		/* bytes to copy */
	int copied = 0;		/* total bytes copied so far */
	int dest_offset = 0;	/* offset into destination sg_entry */

	if (src_offset < 0)
		return 0;

	while (src && dest && count > 0) {
		if (src_offset >= src->count) {
			src_offset -= src->count;
			src = src->next;
		} else if (dest_offset >= dest->count) {
			dest_offset -= dest->count;
			dest = dest->next;
		} else {
			len = MIN3(count, src->count - src_offset,
					dest->count - dest_offset);
			memcpy(phys_to_ptr(dest->paddr) + dest_offset,
					phys_to_ptr(src->paddr) + src_offset,
					len);
			copied += len;
			src_offset += len;
			dest_offset += len;
			count -= len;
		}
	}

	return copied;
}
