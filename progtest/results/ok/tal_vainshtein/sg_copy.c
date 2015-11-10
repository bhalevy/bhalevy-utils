#include "sg_copy.h"
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* integer type to hold the address of the buffer for pointer arithmetics */
typedef unsigned long ulongptr_t;

/*
 * sg_get_next_aligned_ptr   Calculates the address of the next aligned pointer in the buffer
 *                           Internal function.
 *
 * @in buf                   Pointer to buffer
 *
 * @ret                      the address of the next aligned pointer in the buffer
 *
 */
static inline void *sg_get_next_aligned_ptr(void *buf)
{
	ulongptr_t baddr = (ulongptr_t)buf;

	baddr = (baddr + PAGE_SIZE) & ~(PAGE_SIZE-1);
	return (void*)baddr;
}

/*
 * sg_map_one_entry  Maps a chunk of memory buffer that fits one scatter-gather entry
 *                   Internal function.
 *
 * @in buf           Pointer to a chunk of the buffer
 * @in length        Chunk length in bytes, expects values <= PAGE_SIZE
 *
 * @ret              sg_entry element mapping 'length' bytes of the buffer
 *
 */
static sg_entry_t *sg_map_one_entry(void *buf, int length)
{
	sg_entry_t *sg_entry;

	if (!buf || length > PAGE_SIZE)
		return NULL;

	sg_entry = (sg_entry_t *) malloc(sizeof(sg_entry_t));
	if (!sg_entry) {
		return NULL;
	}

	sg_entry->paddr = ptr_to_phys(buf);
	sg_entry->count = length;
	sg_entry->next = NULL;
	return sg_entry;
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
	int chunk_length;
	void *chunk_cur, *chunk_next;
	sg_entry_t *sg_entry_first, *sg_entry_cur;

	if (!buf || !length) {
		return NULL;
	}

	if (length <= PAGE_SIZE) {
		return sg_map_one_entry(buf, length);
	}

	/* creates the first sg_entry */
	chunk_cur = buf;
	chunk_next = sg_get_next_aligned_ptr(chunk_cur);
			/* All entries except the first one must be
			   aligned on a PAGE_SIZE address */
	chunk_length = (ulongptr_t)chunk_next - (ulongptr_t)chunk_cur;
	sg_entry_first = sg_map_one_entry(chunk_cur, chunk_length);
	if (!sg_entry_first) {
		return NULL;
	}
	sg_entry_cur = sg_entry_first;
	chunk_cur = chunk_next;
	length -= chunk_length;

	/* adds additional sg_entries if needed */
	while (length > 0 && sg_entry_cur) {
		chunk_length = MIN(PAGE_SIZE, length);
		sg_entry_cur->next = sg_map_one_entry(chunk_cur, chunk_length);
		sg_entry_cur = sg_entry_cur->next;
		chunk_cur = sg_get_next_aligned_ptr(chunk_cur);
				/* All entries except the first one must be
					aligned on a PAGE_SIZE address */
		length -= chunk_length;
	};

	 /* in case of memory allocation failure exit the loop
	    and destroy all the previously allocated sg_entries */
	if (!sg_entry_cur) {
		sg_destroy(sg_entry_first);
		sg_entry_first = NULL;
	}

	return sg_entry_first;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *sg_entry_next;

	while (sg_list) {
		sg_entry_next = sg_list->next;
		free(sg_list);
		sg_list = sg_entry_next;
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

	/* ### 'dest' list is used as is and never enlarged in case it's too short,
	 * ### in case it's too long, the unneeded entries are destroyed to prevent memory leak.
	 */

	void *chunk;
	int lenght, bytes_left;

	if (!src || !dest || !count) {
		return 0;
	}

	/* finds the first sg_entry to copy from */
	while (src && src_offset >= src->count) {
		src_offset -= src->count;
		src = src->next;
	}

	if (!src) {
		return 0;
	}

	/* finds the buffer address at the needed offset */
	chunk = (void *)((ulongptr_t)phys_to_ptr(src->paddr) + src_offset);

	/* copies the chunk of the buffer */
	bytes_left = count;
	dest->paddr = ptr_to_phys(chunk);
	lenght = MIN(bytes_left, src->count - src_offset);
	dest->count = lenght;
	bytes_left -= lenght;

	/* copies next chunks */
	while(bytes_left > 0 && src->next && dest->next) {
		src = src->next;
		dest = dest->next;
		dest->paddr = src->paddr;
					/* all entries in src (except the first one)
					   are already aligned on a PAGE_SIZE address */
		lenght = MIN(bytes_left, src->count);
		dest->count = lenght;
		bytes_left -= lenght;
	}

	if (dest->next) {
		/* removes the unneeded sg_entries of the dest list */
		sg_destroy(dest->next);
		dest->next = NULL;
	}
	return count - bytes_left;
}
