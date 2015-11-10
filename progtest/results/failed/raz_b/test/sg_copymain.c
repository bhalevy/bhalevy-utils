
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

int verbose = 1;

static sg_entry_t *
my_sg_map(void *buf, int length)
{
	int count;
	char *p = buf;
	sg_entry_t *sg, *sg_list = NULL, **next;

	for (next = &sg_list;
	     length;
	     p += count, length -= count, next = &(*next)->next) {
		sg = *next = malloc(sizeof(sg_entry_t));
		sg->paddr = ptr_to_phys(p);
		count = length;
		if (count + (sg->paddr & (PAGE_SIZE-1)) > PAGE_SIZE) {
			count = PAGE_SIZE - (sg->paddr & (PAGE_SIZE-1));
		}
		sg->count = count;
		sg->next = NULL;
	}

	return sg_list;
}

static void
dump_map(char *buf, sg_entry_t *test, sg_entry_t *ref)
{
	do {
		fprintf(stderr, "buf=%p\n", buf);
		if (test) {
			fprintf(stderr, "test: %p paddr=%04lx count=%d next=%p\n",
				test, test->paddr, test->count, test->next);
			test = test->next;
		} else
			fprintf(stderr, "test: NULL\n");
		if (ref) {
			fprintf(stderr, " ref: %p paddr=%04lx count=%d next=%p\n",
				ref, ref->paddr, ref->count, ref->next);
			buf += ref->count;
			ref = ref->next;
		} else
			fprintf(stderr, "ref : NULL\n");
		fprintf(stderr, "\n");
	} while (test || ref);
}

static sg_entry_t *
test_sg_map(char *buf, int length)
{
	sg_entry_t *test, *test0;
	sg_entry_t *ref, *ref0;

	if (verbose)
		fprintf(stderr, "test_sg_map: buf=%p length=%d\n", buf, length);
	test = test0 = sg_map(buf, length);
	ref = ref0 = my_sg_map(buf, length);

	while (test) {
		if (!ref) {
			fprintf(stderr, "test_sg_map: test=%p ref=%p\n",  test, ref);
			dump_map(buf, test0, ref0);
			exit(1);
		}
		if (test->paddr != ref->paddr) {
			fprintf(stderr, "test_sg_map: test->paddr=%lx ref->paddr=%lx\n",  test->paddr, ref->paddr);
			dump_map(buf, test0, ref0);
			exit(1);
		}
		if (test->count != ref->count) {
			fprintf(stderr, "test_sg_map: test->count=%d ref->count=%d\n",  test->count, ref->count);
			dump_map(buf, test0, ref0);
			exit(1);
		}
		test = test->next;
		ref = ref->next;
	}
	if (ref) {
		fprintf(stderr, "test_sg_map: test=%p ref=%p\n",  test, ref);
		dump_map(buf, test0, ref0);
		exit(1);
	}

	return test0;
}

static int
test_sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int ret = sg_copy(src, dest, src_offset, count);
	if (verbose)
		printf("sg_copy: src=%p dest=%p offs=%d count=%d: ret=%d\n", src, dest, src_offset, count, ret);

	return ret;
}

static void
sg_verify(char *src, char *dest, int scount, int dcount, int offs, int len, int n)
{
	int count;

	if (verbose)
		fprintf(stderr, "sg_verify: src=%p dest=%p scount=%d dcount=%d offs=%d len=%d n=%d\n", src, dest, scount, dcount, offs, len, n);

	count = len;
	if (offs + count > scount) {
		count = scount - offs;
	}
	if (count > dcount) {
		count = dcount;
	}

	if (n != count) {
		fprintf(stderr, "sg_verify: scount=%d dcount=%d offs=%d len=%d: n=%d should have been %d\n",  scount, dcount, offs, len, n, count);
		exit(1);
	}

	if (memcmp(dest, src + offs, count)) {
		fprintf(stderr, "sg_verify: scount=%d dcount=%d offs=%d len=%d n=%d: memcmp failed\n",  scount, dcount, offs, len, n);
		exit(1);
	}  
}

int
main(int argc, char **argv)
{
	int i, n;
	int scount, dcount, offs, count;
	char *src, *dest, *sbak, *dbak;
	sg_entry_t *sg_src, *sg_dest;

	scount = 80;
	dcount = 100;

	src = calloc(scount, 1);
	sbak = calloc(scount, 1);
	dest = calloc(dcount, 1);
	dbak = calloc(dcount, 1);

	for (i = 0; i < scount; i++)
		src[i] = i;

	memcpy(sbak, src, scount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 0;
	count = 80;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 0;
	count = 70;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 10;
	count = 70;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 17;
	count = 60;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 33;
	count = 80;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 80;
	count = 1;
	n = test_sg_copy(sg_src, sg_dest, offs, count);
	sg_verify(sbak, dest, scount, dcount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	for (i = 0; i < dcount; i++)
		dest[i] = i;
	memcpy(dbak, dest, dcount);
	memset(src, 0, scount);

	sg_src = test_sg_map(src, scount);
	sg_dest = test_sg_map(dest, dcount);
	offs = 5;
	count = 90;
	n = test_sg_copy(sg_dest, sg_src, offs, count);
	sg_verify(dbak, src, dcount, scount, offs, count, n);
	sg_destroy(sg_src);
	sg_destroy(sg_dest);
	memset(dest, 0, dcount);

	printf("PASSED\n");
	return 0;
}
