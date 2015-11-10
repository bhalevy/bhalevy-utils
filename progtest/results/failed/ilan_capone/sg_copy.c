#include "sg_copy.h"
#include <stdlib.h>
#include <assert.h>


/*
 * NOTES:
 *
 * -1-
 * I am relying on an assumption, that all included in the header file is
 * part of this API, and can be relied on.
 * Specifically - the inline functions translating between pointers and
 * physaddr_t show that page boundary checks for either are the same.
 * With this justification, I am not translating back and forth to check
 * the validity of a given entry, or to calculate the proper parameters
 * for a new or copied entry.
 *
 * -2-
 * The specification of sg_copy() implies, that the dest list is already
 * allocated ("sg_entry_t **dest_p" would imply it is an output parameter
 * carrying a pointer to data our function should allocate).
 * I assumed copy semantics, and not duplicate (not allocating new entries
 * for the dest list, and using as bounds all 3 elements: src list existing
 * entries, dest list allocated entries, and given count.
 * Were the semantics those of duplicate, I could use the given entry as
 * a head with zero count, and allocate new entries in a manner similar
 * to that done in sg_map().
 */


/******************************************************************************
 * Internal implementation, static functions
 *****************************************************************************/

static int
sg_entry_valid(sg_entry_t *entry)
{
	physaddr_t first_page = entry->paddr & ~(PAGE_SIZE-1);
	physaddr_t last_page  = (entry->paddr + entry->count - 1) & ~(PAGE_SIZE-1);
	return first_page == last_page;
}

static int
sg_chunk_length(void *chunk_buf, int chunk_max_len)
{
	physaddr_t paddr     = ptr_to_phys(chunk_buf);
	physaddr_t page      = paddr & ~(PAGE_SIZE-1);
	int        offset    = paddr & ~page;
	int        remaining = PAGE_SIZE - offset;
	if (chunk_max_len < remaining) {
		remaining = chunk_max_len;
	}
	return remaining;
}

static sg_entry_t *
sg_single_alloc(void *chunk_buf, int chunk_len)
{
	/* See the comment in sg_single_dealloc(). */

	sg_entry_t *entry = malloc(sizeof(sg_entry_t));
	/*
	 * Allocation result should be checked, but we need a well defined
	 * policy, such as: abort, let it SEGV, silently skip the operation,
	 * write whatever log, etc.
	 */

	entry->paddr = ptr_to_phys(chunk_buf);
	entry->count = chunk_len;
	entry->next  = NULL;

	return entry;
}

static void
sg_single_dealloc(sg_entry_t *entry)
{
	/*
	 * This is a naive implementation.
	 * In a more optimized implementation, we may want to:
	 *
	 * 1/ Pool the entries by pushing them to a "free pool" rather than calling
	 *    malloc/free every time, for faster allocation and deallocation, and 
	 *    to enable the idea mentioned in [2] right below.
	 *
	 * 2/ Allocate in not-too-small and not-too-big continuous groups, e.g. 4k,
	 *    for better heap utilization (less unused "pad" at the end of each 
	 *    allocated entry, less heap fragmentation, all without requiring
	 *    too big continuous allocations.
	 */

	free(entry);
}

static int
sg_single_copy(sg_entry_t *src, sg_entry_t *dest, int offset, int count)
{
	int copied = src->count - offset;
	if (copied > count) {
		copied = count;
	}

	assert(sg_entry_valid(src)); /* Input validity */

	dest->paddr = src->paddr + offset;
	dest->count = copied;

	assert(sg_entry_valid(dest)); /* Output validity */

	return copied;
}


/******************************************************************************
 * Required API implementation
 *****************************************************************************/

sg_entry_t *
sg_map(void *buf, int length)
{
	sg_entry_t *list = NULL;
	sg_entry_t **chunk_p = &list;

	/* Negative length is invalid; NULL buf is only valid with zero length */
// BH	assert((length == 0) || ((length > 0) && buf));

	while (length > 0) {
		int chunk_len = sg_chunk_length(buf, length);
		sg_entry_t *new_chunk = sg_single_alloc(buf, chunk_len);

		assert(sg_entry_valid(new_chunk)); /* Validate our output */

		*chunk_p = new_chunk;
		chunk_p = &new_chunk->next;
		buf += chunk_len;
		length -= chunk_len;
	}

	return list;
}

void
sg_destroy(sg_entry_t *sg_list)
{
	while (sg_list) {
		sg_entry_t *entry = sg_list;
		sg_list = sg_list->next;
		sg_single_dealloc(entry);
	}
}

int
sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copied = 0;

	/* Negative length or offset is disallowed */
	/* No assertion on src or dest, since NULL means empty list of us */
// BH	assert((count >= 0) && (src_offset >= 0));
	if (count <= 0 || src_offset < 0) return 0;

	/* No need to do anything if we're not going to copy anything */
	if ((count == 0) || !src || !dest) {
		return copied;
	}

	/* Find the entry where our offset begins */
	while (src && (src_offset >= src->count)) {
		assert(sg_entry_valid(src)); /* Input validity */
		src_offset -= src->count;
		src = src->next;
	}

	/* Now copy all chunks we can, within range, as long as the lists allow */
	while (src && dest && (count > 0)) {
		int copied_this_time = sg_single_copy(src, dest, src_offset, count);
		src_offset = 0; /* Only count it the first time */

		copied += copied_this_time;
		count -= copied_this_time;

		src = src->next;
		dest = dest->next;
	}

	return copied;
}

