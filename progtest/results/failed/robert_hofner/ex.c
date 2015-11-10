#include <string.h>
#include "sg_copy.h"

#define MIN(a,b)	(a < b ? a : b)

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *s0, *s1, *start;
	char *loc, *bufend = (char *)buf + length;


	if (start = (sg_entry_t *)malloc(sizeof(sg_entry_t)))
		return (NULL);

	s0 = start;
	s0.paddr = ptr_to_phys(buf);
	s0.count = MIN(PAGE_SIZE, length);

	loc = (buf + PAGE_SIZE) & ~(PAGE_SIZE - 1);
	while (loc < bufend) {
		if (s1 = (sg_entry_t *)malloc(sizeof(sg_entry_t))) {
			sg_destroy(start);
			return (NULL);
		}
		s0.next = (sg_entry *)s1;

		/*
		 * Once an address that's aligned on PAGE_SIZE was
		 * computed, and each chunk size is PAGE_SIZE, all
		 * chunks are aligned and all of them (with the possible
		 * exception of the last one) are the same size
		 */
		s1.paddr = ptr_to_phys(loc);
		s1.count = MIN(bufend - loc, PAGESIZE);
		s0 = s1;
		loc += PAGE_SIZE;
	}

	start.next = NULL;
	return start;
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *s0 = sg_list, *s1;

	if (s0 == NULL)
		return;
	s1 = s0->next;
	free(s0);
	while (s1) {
		s0 = s1;
		s1 = s1->next;
		free(s0);
	}
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	sg_entry_t *src = src;
	sg_entry_t *d = dest;
	int cnt = 0, off = 0;

	if (s == NULL || d == NULL)
		return 0;

	while (off + src->count < src_offset && s) {
		off += s->count;
		s = s->next;
	}

	while (s && d && cnt < count) {
		int ncopy = MIN(d->count, s->count);

		memcpy(phys_to_ptr(d->paddr), phys_to_ptr(s->paddr), ncopy);
		cnt += ncopy;
		if (d->count <= s->count)
			d = d->next;
		if (d->count >= s->count)
			s = s->next;
	}
	return cnt;
}
