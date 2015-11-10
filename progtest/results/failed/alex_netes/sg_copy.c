#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg_new_p, *sg_last_p;
	sg_entry_t *sg_head_p = NULL;

	if (!buf)
		return NULL;

	while (length > 0) {
		sg_new_p = (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if (!sg_new_p) {
// BH: why not use sg_destroy?
			while (sg_head_p) {
				sg_new_p = sg_head_p;
				sg_head_p = sg_head_p->next;
				free(sg_new_p);
			}
			return NULL;
		}
		sg_new_p->paddr = ptr_to_phys(buf);
// BH: BUG: first page alignment
//		sg_new_p->count = (length < PAGE_SIZE) ? length : PAGE_SIZE;
sg_new_p->count = PAGE_SIZE - (sg_new_p->paddr & (PAGE_SIZE-1));
if (sg_new_p->count > length) sg_new_p->count = length;
		sg_new_p->next = NULL;
// BH: could have user **
		if (!sg_head_p)
			sg_head_p = sg_new_p;
		else
			sg_last_p->next = sg_new_p;

		sg_last_p = sg_new_p;
		buf += sg_new_p->count;
		length -= sg_new_p->count;
	}
	return sg_head_p;
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *sg_p;

	while(sg_list) {
		sg_p = sg_list->next;
		free(sg_list);
		sg_list = sg_p;
	}
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{

	int dst_offset = 0;
	int cpy_size = 0;
	int size;

// BH: signed count
	if (!src || !dest || !count)
		return 0;
	
// BH: BUG	while (src_offset > PAGE_SIZE && src) {
	while (src_offset > src->count && src) {
// BH: mine
src_offset -= src->count;
		src = src->next;
// BH: BUG		src_offset -= PAGE_SIZE;
	}

// BH: redundant
	if (!src)
		return 0;

	while (count > 0 && dest && src) {
// BH: this is odd
#if 0
		if (src_offset > dst_offset)
			size = (count < PAGE_SIZE - src_offset) ? count : 
			       PAGE_SIZE - src_offset;
		else
			size = (count < PAGE_SIZE - dst_offset) ? count :
			       PAGE_SIZE - dst_offset;
#endif
size = src->count - src_offset;
if (size > dest->count - dst_offset) size = dest->count - dst_offset;
if (size > count) size = count;

		memcpy(phys_to_ptr(dest->paddr) + dst_offset,
		       phys_to_ptr(src->paddr) + src_offset, size);

		cpy_size += size;
		dst_offset += size;
		src_offset += size;
// BU: BUG		if (dst_offset >= PAGE_SIZE) {
		if (dst_offset >= dest->count) {
			dst_offset = 0;
			dest = dest->next;
		}

// BH: BUG		if (src_offset >= PAGE_SIZE) {
		if (src_offset >= src->count) {
			src_offset = 0;
			src = src->next;
		}
// BH: BUG, should use size, not cpy_size
//		count -= cpy_size;
		count -= size;
	}
	return cpy_size;
}
