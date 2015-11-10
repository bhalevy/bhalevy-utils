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
			while (sg_head_p) {
				sg_new_p = sg_head_p;
				sg_head_p = sg_head_p->next;
				free(sg_new_p);
			}
			return NULL;
		}
		sg_new_p->paddr = ptr_to_phys(buf);
		sg_new_p->count = (length < PAGE_SIZE) ? length : PAGE_SIZE;
		sg_new_p->next = NULL;
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

	if (!src || !dest || !count)
		return 0;
	
	while (src_offset > PAGE_SIZE && src) {
		src = src->next;
		src_offset -= PAGE_SIZE;
	}

	if (!src)
		return 0;

	while (count > 0 && dest && src) {
		if (src_offset > dst_offset)
			size = (count < PAGE_SIZE - src_offset) ? count : 
			       PAGE_SIZE - src_offset;
		else
			size = (count < PAGE_SIZE - dst_offset) ? count :
			       PAGE_SIZE - dst_offset;

		memcpy(phys_to_ptr(dest->paddr) + dst_offset,
		       phys_to_ptr(src->paddr) + src_offset, size);

		cpy_size += size;
		dst_offset += size;
		src_offset += size;
		if (dst_offset >= PAGE_SIZE) {
			dst_offset = 0;
			dest = dest->next;
		}

		if (src_offset >= PAGE_SIZE) {
			src_offset = 0;
			src = src->next;
		}
		count -= cpy_size;
	}
	return cpy_size;
}
