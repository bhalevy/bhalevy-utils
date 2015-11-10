#include <stdlib.h>
#include <stdio.h>

#include "sg_copy.h"

#define IS_PAGE_ALIGNED(x) (((x) & (PAGE_SIZE-1)) == 0)

#define ROUND_UP(x, y) ((((x) + (y - 1)) / y) * y)

#define min(x, y) ((x) < (y) ? (x) : (y))

/*
 * sg_map_one - map a buffer into a single sg_entry
 *
 * @buf: buffer to map
 * @size: how many bytes to map
 *
 * note: size must be in range (0, PAGE_SIZE]
 */
static sg_entry_t *sg_map_one(void *buf, int size)
{
	sg_entry_t *sg;

	sg = malloc(sizeof(*sg));
	if (sg) {
		sg->paddr = ptr_to_phys(buf);
		sg->count = size;
		sg->next = NULL;
	}

	return sg;
}

/*
 * sg_align_buf - alignes a buffer to a PAGE_SIZE address
 *
 * @buf: the buffer to align
 * @size: the buffer size
 *
 * @ret: sg mapping of range [old_buf, aligned_buf)
 *
 * note: - this function modifies both buf and size
 * 	 - buf is assumed to be not page aligned
 */
static sg_entry_t *sg_align_buf(void **buf, int *size)
{
	sg_entry_t *sg = NULL;
	void *aligned = (void *) ROUND_UP((unsigned long) *buf, PAGE_SIZE);
	int cnt = min(*size, (unsigned long) aligned - (unsigned long) *buf);

	sg = sg_map_one(buf, cnt);
	if (sg) {
		*buf += cnt;
		*size -= cnt;
	}

	return sg;
}

/*
 * The guts of sg_map
 */
static sg_entry_t *__sg_map(void *buf, int size)
{
	sg_entry_t *head = NULL;
	sg_entry_t **sg = &head;

	/* align buffer */
	if (!IS_PAGE_ALIGNED((unsigned long) buf)) {
		*sg = sg_align_buf(&buf, &size);
		if (!(*sg))
			goto no_mem;
		sg = &(*sg)->next;
	}

	/* map buffer, one page at a time */
	while (size > PAGE_SIZE) {
		*sg = sg_map_one(buf, PAGE_SIZE);
		if (!(*sg))
			goto no_mem;

		buf += PAGE_SIZE;
		size -= PAGE_SIZE;
		sg = &(*sg)->next;
	}

	/* last entry might be needed */
	if (size > 0) {
		*sg = sg_map_one(buf, size);
		if (!(*sg))
			goto no_mem;
	}

	return head;

no_mem:
	fprintf(stderr, "%s: failed to allocate %lu bytes\n", __func__, sizeof(*head));
	sg_destroy(head);
	return NULL;
}

/*
 * sg_map - map a memory buffer using a scatter-gather list
 *
 * @buf: buffer to map
 * @length: buffer length
 *
 * @ret: sg_list is successful, NULL on error.
 */
sg_entry_t *sg_map(void *buf, int length)
{
	if (!buf || length <= 0)
		return NULL;

	return __sg_map(buf, length);
}

/*
 * sg_destroy - Destroy the sg_list
 *
 * @sg_list: the list to destroy
 */
void sg_destroy(sg_entry_t *sg_list)
{
	while (sg_list) {
		sg_entry_t *sg = sg_list->next;
		free(sg_list);
		sg_list = sg;
	}
}

/*
 * sg_seek - Repositions the offset to sg_list
 *
 * @sg: the sg_list to seek on
 * @offset: number of bytes to seek
 *
 * @ret: sg_entry node for which seek stops, NULL if offset > size(sg)
 *
 * note: modifies offset by the ammount bytes skiped
 */
static sg_entry_t *sg_seek(sg_entry_t *sg, int *offset)
{
	while (sg && *offset - sg->count >= 0) {
		*offset -= sg->count;
		sg = sg->next;
	}

	return sg;
}

/*
 * sg_copy_head - copy sg_list head
 *
 * @from: the list to copy from
 * @to: the list to copy to
 * @offset: offset into from
 * @limit: max amount of data allowed to copy
 *
 * @ret: number of bytes copied
 */
static int sg_copy_head(sg_entry_t **from, sg_entry_t **to, int offset, int limit)
{
	int ret;

	if (offset == 0)
		return 0;

// BH: test input
if (!(*from) || !*to) return 0;

	(*to)->paddr = ptr_to_phys(phys_to_ptr((*from)->paddr) + offset);
	ret = (*to)->count = min(limit, PAGE_SIZE - offset);

	*to = (*to)->next;
	*from = (*from)->next;

	return ret;
}

/*
 * sg_copy_tail - copy sg_list tail
 *
 * @from: the list to copy from
 * @to: the list to copy to
 * @count: number of bytes to copy
 *
 * @ret: number of bytes copied
 *
 * note: less then count might be copied if the from list doesn't have enough bytes
 */
static int sg_copy_tail(sg_entry_t **from, sg_entry_t **to, int count)
{
	int ret;

// BH BUG	if (!(*from))
if (!(*from) || !*to)
		return 0;

	(*to)->paddr = (*from)->paddr;
	ret = (*to)->count = min((*from)->count, count);

	*to = (*to)->next;
	*from = (*from)->next;

	return ret;
}

/*
 * sg_copy_page - copy a page from one list to another
 *
 * @from: the list to copy from
 * @to: the list to copy to
 *
 * @ret: number of bytes copied
 */
static int sg_copy_page(sg_entry_t **from, sg_entry_t **to)
{
	int ret;

// BH: test input
if (!(*from) || !*to) return 0;

	(*to)->paddr = (*from)->paddr;
	ret = (*to)->count = (*from)->count;
	
	*to = (*to)->next;
	*from = (*from)->next;

	return ret;
}

/*
 * Copy bytes using scatter-gather lists
 *
 * Assumptions: 1. This function will not allocate sg_entries, so dest must be long enough.
 * 		2. This function will be free any leftover tail of the dest list - use sg_destroy if needed.
 *
 * Danit: I can add any missing pieces if needed.
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copied = 0;
	sg_entry_t **from = &src, **to = &dest;

	/* seek forward */
	*from = sg_seek(*from, &src_offset);
	if (!(*from)) {
// BH		fprintf(stderr, "%s: offset > size(src)\n", __func__);
		return 0;
	}

	copied = sg_copy_head(from, to, src_offset, count);
	count -= copied;

	/* copy pages */
	while (*from && count > PAGE_SIZE) {
		int written = sg_copy_page(from, to);
// BH: break condition
if (!written) break;
		copied += written;
		count -= written;
	}

	/* copy last block */
	if (count)
		copied += sg_copy_tail(from, to, count);

	return copied;
}

