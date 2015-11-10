#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#if 0
sg_copy.c: In function ‘sg_map’:
sg_copy.c:12:28: warning: incompatible implicit declaration of built-in function ‘malloc’ [enabled by default]
sg_copy.c:16:4: error: request for member ‘paddr’ in something not a structure or union
sg_copy.c:17:4: error: request for member ‘count’ in something not a structure or union
sg_copy.c:19:26: error: invalid operands to binary & (have ‘void *’ and ‘int’)
sg_copy.c:25:5: error: request for member ‘next’ in something not a structure or union
sg_copy.c:25:14: error: ‘sg_entry’ undeclared (first use in this function)
sg_copy.c:25:14: note: each undeclared identifier is reported only once for each function it appears in
sg_copy.c:25:24: error: expected expression before ‘)’ token
sg_copy.c:33:5: error: request for member ‘paddr’ in something not a structure or union
sg_copy.c:34:5: error: request for member ‘count’ in something not a structure or union
sg_copy.c:34:14: error: ‘PAGESIZE’ undeclared (first use in this function)
sg_copy.c:39:7: error: request for member ‘next’ in something not a structure or union
sg_copy.c: In function ‘sg_destroy’:
sg_copy.c:50:2: warning: incompatible implicit declaration of built-in function ‘free’ [enabled by default]
sg_copy.c: In function ‘sg_copy’:
sg_copy.c:60:14: error: ‘src’ redeclared as different kind of symbol
sg_copy.c:58:25: note: previous definition of ‘src’ was here
sg_copy.c:64:6: error: ‘s’ undeclared (first use in this function)
make: *** [sg_copy.o] Error 1
[bhalevy@lt ~/home/src/progtest/robert_hofner] 1011$ mute kw sg_copy.c &
[1] 8551
[bhalevy@lt ~/home/src/progtest/robert_hofner] 1012$ make
cc    -c -o sg_copy.o sg_copy.c
sg_copy.c: In function ‘sg_map’:
sg_copy.c:17:4: error: request for member ‘paddr’ in something not a structure or union
sg_copy.c:18:4: error: request for member ‘count’ in something not a structure or union
sg_copy.c:20:26: error: invalid operands to binary & (have ‘void *’ and ‘int’)
sg_copy.c:26:5: error: request for member ‘next’ in something not a structure or union
sg_copy.c:26:14: error: ‘sg_entry’ undeclared (first use in this function)
sg_copy.c:26:14: note: each undeclared identifier is reported only once for each function it appears in
sg_copy.c:26:24: error: expected expression before ‘)’ token
sg_copy.c:34:5: error: request for member ‘paddr’ in something not a structure or union
sg_copy.c:35:5: error: request for member ‘count’ in something not a structure or union
sg_copy.c:35:14: error: ‘PAGESIZE’ undeclared (first use in this function)
sg_copy.c:40:7: error: request for member ‘next’ in something not a structure or union
sg_copy.c: In function ‘sg_copy’:
sg_copy.c:61:14: error: ‘src’ redeclared as different kind of symbol
sg_copy.c:59:25: note: previous definition of ‘src’ was here
sg_copy.c:65:6: error: ‘s’ undeclared (first use in this function)
make: *** [sg_copy.o] Error 1
#endif

#define MIN(a,b)	(a < b ? a : b)

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *s0, *s1, *start;
	char *loc, *bufend = (char *)buf + length;

// BH: BUG reverse test
	if ((start = (sg_entry_t *)malloc(sizeof(sg_entry_t))) == NULL)
		return (NULL);

	s0 = start;
// BH: s/./->/
	s0->paddr = ptr_to_phys(buf);
// BH: BUG alignment
//	s0->count = MIN(PAGE_SIZE, length);
	s0->count = MIN(PAGE_SIZE - ((unsigned long)buf & (PAGE_SIZE-1)), length);
// BH: added by me
	s0->next = NULL;

// BH: missing typecasts (unsigned long)buf
//	loc = (char *)(((unsigned long)buf + PAGE_SIZE) & ~(PAGE_SIZE - 1));
// BH: BUG must use s0->count
	loc = (char *)buf + s0->count;
	while (loc < bufend) {
// BH: BUG reverse test
		if ((s1 = (sg_entry_t *)malloc(sizeof(sg_entry_t))) == NULL) {
			sg_destroy(start);
			return (NULL);
		}
// BH: s/./->/
// BH: s/sg_entry/sg_entry_t/
		s0->next = (sg_entry_t *)s1;
// BH: added by me
		s1->next = NULL;

		/*
		 * Once an address that's aligned on PAGE_SIZE was
		 * computed, and each chunk size is PAGE_SIZE, all
		 * chunks are aligned and all of them (with the possible
		 * exception of the last one) are the same size
		 */
// BH: s/./->/
		s1->paddr = ptr_to_phys(loc);
// BH: s/PAGESIZE/PAGE_SIZE/
		s1->count = MIN(bufend - loc, PAGE_SIZE);
		s0 = s1;
		loc += PAGE_SIZE;
	}

// BH: s/./->/
// BH: BUG: plain wrong to set start->next = NULL, should do that for last entry not first
//	start->next = NULL;
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
// BH: s/src/s/
	sg_entry_t *s = src;
	sg_entry_t *d = dest;
	int cnt = 0, off = 0, soff, doff;

	if (s == NULL || d == NULL)
		return 0;

// BH: BUG: should be <=
	while (off + src->count <= src_offset && s) {
		off += s->count;
		s = s->next;
	}

// BH: BUG: missing offsets into entry
	soff = src_offset - off;
	doff = 0;
	while (s && d && cnt < count) {
// BH: BUG: missing offset into entry
//		int ncopy = MIN(d->count, s->count);
		int ncopy = MIN(d->count - doff, s->count - soff);
// BH: BUG: missing MIN against remaining count
		if (cnt + ncopy > count)
			ncopy = count - cnt;

		memcpy(phys_to_ptr(d->paddr) + doff, phys_to_ptr(s->paddr) + soff, ncopy);
		cnt += ncopy;
// BH: BUG: use entry offsets
//		if (d->count <= s->count)
//			d = d->next;
//		if (d->count >= s->count)
//			s = s->next;
		doff += ncopy;
		if (doff >= d->count) {
			d = d->next;
			doff = 0;
		}

		soff += ncopy;
		if (soff >= s->count) {
			s = s->next;
			soff = 0;
		}
	}
	return cnt;
}
