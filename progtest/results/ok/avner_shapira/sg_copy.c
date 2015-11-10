/* to simplify my test compilation, I used user-space headers and functions.
 * however, since "physical" addresses are involved, I assume my code should be
 * compiled in kernel. then translate malloc(x) to kmalloc(x, GFP_KERNEL) and
 * free to kfree */
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *first = NULL, **ent = &first;

// BH: validate input
if (!buf || length <= 0) return NULL;

	while(length)
	{
		physaddr_t pa = ptr_to_phys(buf);
		int c = PAGE_SIZE - pa % PAGE_SIZE;
		if (length < c)
			c = length;

		*ent = malloc(c);
		(*ent)->paddr = pa;
		(*ent)->count = c;
		(*ent)->next = NULL;

		buf += c;
		length -= c;
		ent = &(*ent)->next;
	}
	return first;
}

void sg_destroy(sg_entry_t *sg_list)
{
	while(sg_list)
	{
		sg_entry_t *tmp = sg_list->next;
		free(sg_list);
		sg_list = tmp;
	}
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	/* assumptions:
	 * - src, dest were not necessarily created using sg_map(). so they may
	 *   have non-first nodes that don't align to PAGE_SIZE. also they may
	 *   map non-continuous memory areas. otherwise they are valid as
	 *   described in the h file.
	 * - dest already exists and maps a given mem area, which should not be
	 *   exceeded.
	 */
	int dest_offset = 0;
	int ret = 0;

// BH
if (count <= 0 || src_offset < 0) return 0;

	/* skip to the desired offset */
	while(src && src_offset > src->count)
	{
// BH: BUG: reversed order
//		src = src->next;
//		src_offset -= src->count;
src_offset -= src->count;
src = src->next;
	}

	while(ret < count && src && dest)
	{
		int c = count - ret;
		if (c > src->count - src_offset)
			c = src->count - src_offset;
		if (c > dest->count - dest_offset)
			c = dest->count - dest_offset;

		memcpy(phys_to_ptr(dest->paddr + dest_offset),
				phys_to_ptr(src->paddr + src_offset), c);

		dest_offset += c;
		src_offset += c;
		ret += c;
		if (dest_offset == dest->count)
		{
			dest = dest->next;
			dest_offset = 0;
		}
		if (src_offset == src->count)
		{
			src = src->next;
			src_offset = 0;
		}
	}

	return ret;
}
