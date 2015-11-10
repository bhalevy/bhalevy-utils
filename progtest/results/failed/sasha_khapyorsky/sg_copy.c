#include <stdlib.h>
#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
	physaddr_t paddr = ptr_to_phys(buf);
	sg_entry_t *list, *last;

	if (length <= 0)
		return NULL;

	/* put it out of loop in order to simplify loop flow (more code,
	 * but less calculations) */
	list = malloc(sizeof(*list));
	if (!list)
		return NULL;
	list->paddr = paddr;
	list->count = PAGE_SIZE - (paddr & (PAGE_SIZE - 1));
	list->next = NULL;

	if (list->count >= length) {
		list->count = length;
		return list;
	}

	length -= list->count;
// BH: BUG.  cannot depend on physaddr continguity
//	paddr += list->count;
	buf += list->count;

//	for (last = list; length > 0; length -= PAGE_SIZE, paddr += PAGE_SIZE) {
	for (last = list; length > 0; length -= PAGE_SIZE, buf += PAGE_SIZE) {
		sg_entry_t *s = malloc(sizeof(*s));
		if (!s)
			goto error;
		s->paddr = ptr_to_phys(buf);
		s->count = PAGE_SIZE;
		s->next = NULL;
		last->next = s;
		last = s;
	}

	if (length < 0)
		last->count += length;

	return list;

error:
	sg_destroy(list);

	return NULL;
}

void sg_destroy(sg_entry_t * sg_list)
{
	sg_entry_t *last = sg_list;

	while (last) {
		sg_list = last;
		last = sg_list->next;
		free(sg_list);
	}
}

/* the function implementation assumes that sg list could represent
 * non-continuos memory areas (IOW resulted by sg_copy()) */
int sg_copy(sg_entry_t * src, sg_entry_t * dest, int src_offset, int count)
{
	int total;

// BH
if (count <= 0 || src_offset < 0) return 0;

	if (!dest)
		return 0;

// BH: dec src_offset
//	for (; src && src->count <= src_offset; src = src->next)
	for (; src && src->count <= src_offset; src = src->next) {
		src_offset -= src->count;
		src = src->next;
	}

	if (!src)
		return 0;

	/* put it out of loop in order to simplify loop flow (more code,
	 * but less calculations) */
	dest->paddr = src->paddr + src_offset;
	dest->count = src->count - src_offset;
	if (count <= dest->count) {
		dest->count = count;
		return count;
	}

	count -= dest->count;
	total = dest->count;

	for (src = src->next; src && dest->next && count > 0; src = src->next) {
		dest = dest->next;
		dest->paddr = src->paddr;
		dest->count = src->count;
		count -= src->count;
		total += src->count;
	}

	if (count < 0) {
		dest->count += count;
		total += count;
	}

	return total;
}

#ifdef TEST_CASE
int main()
{
	sg_entry_t *s, *d;
	int ret;

	s = sg_map((void *)0xffffffffffff1002, 32);
	d = sg_map((void *)0xffffffffffffff00, 96);

	ret = sg_copy(s, d, 3, 51);

	sg_destroy(s);
	sg_destroy(d);

	return 0;
}
#endif
