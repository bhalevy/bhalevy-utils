/*
 * Copyright (C) 2011, Tonian, Inc. All rights reserved.
 *
 * This software source code is the sole property of Tonian, Inc. and is
 * proprietary and confidential.
 */

#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elementss mapping the input buffer
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */
sg_entry_t *
sg_map(void *buf, int length)
{
	int count;
	const char *p = buf;
	sg_entry_t *sg, *sg_list, **next;

	if (length <= 0 || !buf)
		return NULL;

	sg = sg_list = malloc(sizeof(sg_entry_t));
	if (!sg_list)
		return NULL;
	sg->paddr = ptr_to_phys(p);
	count = PAGE_SIZE - (sg->paddr & (PAGE_SIZE-1));
	sg->count = count;
	sg->next = NULL;
	if (count > length) {
		sg->count = length;
		return sg_list;
	}
	p += count;
	length -= count;

	for (next = &sg->next;
	     length;
	     p += count, length -= count, next = &(*next)->next) {
		sg = *next = malloc(sizeof(sg_entry_t));
		if (!sg) {
			sg_destroy(sg_list);
			return NULL;
		}
		sg->paddr = ptr_to_phys(p);
		sg->count = count = (length < PAGE_SIZE ? length : PAGE_SIZE);
		sg->next = NULL;
	}

	return sg_list;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void
sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *next;

	for (; sg_list; sg_list = next) {
		next = sg_list->next;
		free(sg_list);
	}
}

/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list
 * @in offset    Offset into source
 * @in count     Number of bytes to copy
 *
 * @ret          Actual number of bytes copied
 *
 * @note         The function copies "count" bytes from "src",
 *               starting from "offset" into "dest".
 *               The scatter gather lists can be of arbitrary length so it
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 */
int
sg_copy(sg_entry_t *src, sg_entry_t *dest, int offset, int count)
{
	void *p;
	int n, copied = 0, soff, doff = 0;

	if (!src || !dest || offset < 0 || count < 0)
		return 0;

	for (soff = offset; src && src->count <= soff; src = src->next)
		soff -= src->count;

	for (; src && dest && count; count -= n) {
		n = count;
		if (soff + n > src->count)
			n = src->count - soff;

		if (doff + n > dest->count)
			n = dest->count - doff;

		memcpy((char *)phys_to_ptr(dest->paddr) + doff,
		       (char *)phys_to_ptr(src->paddr) + soff,
		       n);
		copied += n;

		soff += n;
		if (soff >= src->count) {
			soff = 0;
			src = src->next;
		}

		doff += n;
		if (doff >= dest->count) {
			doff = 0;
			dest = dest->next;
		}
	}

	return copied;
}
