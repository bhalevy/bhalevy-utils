#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *head = NULL, *cur = NULL, *prev = NULL;
	// Should really be something like u8 * or uint8 *, but this is compiler-dependent
	unsigned char *ptr = (unsigned char *)buf;

	// Sanity checks
	if ( !buf || !length)
		return NULL;

	if ( !(head = (sg_entry_t *) malloc(sizeof(sg_entry_t))) )
		return NULL;

	// First chunk can be non-PAGE_SIZE aligned, so it needs special attention
// BH: BUG: first page count
//	head->count = length < PAGE_SIZE ? length : PAGE_SIZE - length % PAGE_SIZE;
	head->count = PAGE_SIZE - (unsigned)buf % PAGE_SIZE;
	if (head->count > length)
		head->count = length;
	head->paddr = ptr_to_phys(ptr);
	// Make sure list is consistent in case we call sg_destroy()
	head->next = NULL;

	prev = head;

	while (length -= prev->count)
	{
		if ( !(cur = (sg_entry_t *) malloc(sizeof(sg_entry_t))) )
		{
			sg_destroy(head);
			return NULL;
		}

		ptr += prev->count;

		cur->count = length < PAGE_SIZE ? length : PAGE_SIZE;
		cur->paddr = ptr_to_phys(ptr);
		cur->next = NULL;
		prev->next = cur;

		prev = cur;
	}

	return head;
}

// ------------------------------------------

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *cur = sg_list, *next = NULL;

	while (cur)
	{
		next = cur->next;
		free(cur);
		cur = next;
	}
}

// ------------------------------------------

// BH: why the hack not define min(a, b) and use it for min3(a, b, c)?
static inline int min(int a, int b, int c)
{
	return a < b ? (a < c ? a : c) : (b < c ? b : c);
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	sg_entry_t *cur = NULL;
	int src_len = 0, dest_len = 0;
	int copy_size = 0;
	unsigned char *offsetted_src;

	// Sanity checks
	if (!src || !dest || !count)
		return 0;

	/* Since our scatter-gather list is a mapping of a contiguous memory buffer,
	we can use that to efficiently copy one list to another. Otherwise we'd have
	to deal with copying only parts of chunks, which is a lot messier and longer */

// BH: inefficient
	cur = src;
	while (cur)
	{
		src_len += cur->count;
		cur = cur->next;
	}

	cur = dest;
	while (cur)
	{
		dest_len += cur->count;
		cur = cur->next;
	}

// BH: BUG: didn't take src offset in to account, presumed phys_to_ptr is contiguous
//	copy_size = min(dest_len, src_len, count);
	copy_size = min(dest_len, src_len - src_offset, count);
// BH: BUG: wrong assumption
	offsetted_src = (unsigned char *) phys_to_ptr(src->paddr) + src_offset;
	memcpy(phys_to_ptr(dest->paddr), offsetted_src, copy_size);

	return copy_size;
}
