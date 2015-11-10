/*
 * Copyright 2013 Matthew Ahrens. All rights reserved.
 *
 * Total time spent: approximately 90 minutes
 */
#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <stdio.h>
#include "sg_copy.h"

// BH: required typedefs
typedef unsigned long uintptr_t;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

sg_entry_t *
sg_map(void *buf, int length)
{
	sg_entry_t *first_entry = NULL;
	sg_entry_t *previous_entry = NULL;

//BH
if (length <= 0 || !buf) return NULL;

	while (length > 0) {
		sg_entry_t *entry = malloc(sizeof(*entry));

		/*
		 * If this is the first entry, remember it, because we
		 * need to return the head of the list.
		 */
		if (first_entry == NULL)
			first_entry = entry;

		entry->paddr = ptr_to_phys(buf);

		/*
		 * This entry represents the amount of space left in
		 * this page, but no more than the remaining length.
		 */
		entry->count = MIN(length,
		    PAGE_SIZE - ((uintptr_t)buf & (PAGE_SIZE - 1)));

		entry->next = NULL;

		/*
		 * Append this entry to the end of the list.
		 */
		if (previous_entry != NULL)
			previous_entry->next = entry;
		previous_entry = entry;

		/* Advance "buf" and "length" by this entry's count. */
		buf = (void *)((uintptr_t)buf + entry->count);
		length -= entry->count;
	}
	return (first_entry);
}

void
sg_destroy(sg_entry_t *sg_list)
{
	while (sg_list != NULL) {
		sg_entry_t *next = sg_list->next;
		free(sg_list);
		sg_list = next;
	}
}

int
sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int dest_offset = 0;
	int initial_count = count;

//BH
if (!src || !dest || src_offset < 0 || count < 0) return 0;

	while (src != NULL && src_offset >= src->count) {
		/* Skip this entire entry. */
		src_offset -= src->count;
		src = src->next;
	}

	while (count > 0 && src != NULL && dest != NULL) {
		int to_copy = MIN(count,
		    MIN(src->count - src_offset, dest->count - dest_offset));

		/*
		 * Note: in a real system, phys_to_ptr() would probably
		 * be more expensive, so we would want to save the
		 * virtual addresses if they will be in the same page
		 * next time through the loop.  Furthermore, in a real
		 * system we would typically need to call a routine to
		 * dispose of our virtual address mapping.
		 */

		/*
		 * Note that we can do arithmetic on physaddr_t's as
		 * long as we do not cross page boundaries, and entries
		 * do not cross page boundaries.
		 */
		assert(src_offset < src->count);
		void *src_addr = phys_to_ptr(src->paddr + src_offset);
		assert(dest_offset < dest->count);
		void *dst_addr = phys_to_ptr(dest->paddr + dest_offset);

		bcopy(src_addr, dst_addr, to_copy);

		if (src_offset + to_copy == src->count) {
			/*
			 * In a real system, we would dispose of src's
			 * virtual address in this case.
			 */
			src = src->next;
			src_offset = 0;
		} else {
			/*
			 * In a real system, we would save the result of
			 * phys_to_ptr(src->paddr) for the next
			 * invocation of the loop.
			 */
			src_offset += to_copy;
			assert(src_offset < src->count);
		}

		if (dest_offset + to_copy == dest->count) {
			/*
			 * In a real system, we would dispose of dest's
			 * virtual address in this case.
			 */
			dest = dest->next;
			dest_offset = 0;
		} else {
			/*
			 * In a real system, we would save the result of
			 * phys_to_ptr(dest->paddr) for the next
			 * invocation of the loop.
			 */
			dest_offset += to_copy;
			assert(dest_offset < dest->count);
		}

		count -= to_copy;
	}
	/*
	 * In a real system, we would need to dispose of the saved
	 * virtual address mappings here.
	 */

	/*
	 * We may not have copied the entire requested length, due to
	 * reaching the end of the src or dest lists.  Return the size
	 * actually copied.
	 */
	return (initial_count - count);
}

#ifdef TEST
static void
sg_print(sg_entry_t *entry)
{
	while (entry != NULL) {
		printf("  paddr=%lx count=%u\n", entry->paddr, entry->count);
		entry = entry->next;
	}
}

static void
test_map(void *ptr, int count)
{
	sg_entry_t *sg = sg_map(ptr, count);
	printf("sg_map(%p, %u):\n", ptr, count);
	sg_print(sg);
	sg_destroy(sg);
}

static void
test_copy(void *src, int src_len, void *dest, int dest_len,
    int src_offset, int count)
{
//BH
int i;
	char *src_char = src;

	for (i = 0; i < src_len; i++)
		src_char[i] = i;
	memset(dest, 0xba, dest_len);

	printf("\ntest_copy(src_len=%u dest_len=%u src_offset=%u count=%u)\n",
	    src_len, dest_len, src_offset, count);

	sg_entry_t *src_sg = sg_map(src, src_len);
	printf("src_sg:\n");
	sg_print(src_sg);

	sg_entry_t *dest_sg = sg_map(dest, dest_len);
	printf("dest_sg:\n");
	sg_print(dest_sg);

	int copied = sg_copy(src_sg, dest_sg, src_offset, count);
	printf("copied %u bytes\n", copied);

	assert(copied == MIN(count,
	    MIN(MAX(0, src_len - src_offset), dest_len)));
	assert(bcmp((char *)src + src_offset, dest, copied) == 0);

	sg_destroy(src_sg);
	sg_destroy(dest_sg);
}

#define MAXBUFLEN (PAGE_SIZE * 100)

int
main(int argc, char *argv[])
{
	char *src_buf = malloc(MAXBUFLEN);
	char *dest_buf = malloc(MAXBUFLEN);

	char *src_aligned = src_buf +
	    PAGE_SIZE - ((uintptr_t)src_buf & (PAGE_SIZE - 1));
	char *src_unaligned = src_aligned + PAGE_SIZE / 3;
	char *dest_aligned = dest_buf +
	    PAGE_SIZE - ((uintptr_t)dest_buf & (PAGE_SIZE - 1));
	char *dest_unaligned = dest_aligned + PAGE_SIZE / 3;

	test_map(src_aligned, PAGE_SIZE);
	test_map(src_aligned, PAGE_SIZE * 3);
	test_map(src_aligned, PAGE_SIZE / 5);
	test_map(src_aligned, PAGE_SIZE * 2 + PAGE_SIZE / 5);

	test_map(src_unaligned, PAGE_SIZE / 2);
	test_map(src_unaligned, PAGE_SIZE * 3 + PAGE_SIZE / 2);
	test_map(src_unaligned, PAGE_SIZE / 5);
	test_map(src_unaligned, PAGE_SIZE * 2 + PAGE_SIZE / 5);

	test_copy(src_aligned, PAGE_SIZE, dest_aligned, PAGE_SIZE,
	    0, PAGE_SIZE);
	test_copy(src_aligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 2);
	test_copy(src_aligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 1000);

	test_copy(src_unaligned, PAGE_SIZE, dest_aligned, PAGE_SIZE,
	    0, PAGE_SIZE);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 2);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 1000);

	test_copy(src_unaligned, PAGE_SIZE, dest_unaligned, PAGE_SIZE,
	    0, PAGE_SIZE);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_unaligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 2);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_unaligned, PAGE_SIZE * 2,
	    0, PAGE_SIZE * 1000);

	test_copy(src_aligned, PAGE_SIZE, dest_aligned, PAGE_SIZE,
	    PAGE_SIZE, PAGE_SIZE * 2);
	test_copy(src_aligned, PAGE_SIZE, dest_aligned, PAGE_SIZE,
	    PAGE_SIZE * 2, PAGE_SIZE * 2);
	test_copy(src_aligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    PAGE_SIZE, PAGE_SIZE * 2);
	test_copy(src_aligned, PAGE_SIZE * 2, dest_aligned, PAGE_SIZE * 2,
	    PAGE_SIZE / 2, PAGE_SIZE * 1000);

	test_copy(src_unaligned, PAGE_SIZE, dest_unaligned, PAGE_SIZE,
	    PAGE_SIZE, PAGE_SIZE * 2);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_unaligned, PAGE_SIZE * 2,
	    PAGE_SIZE, PAGE_SIZE * 2);
	test_copy(src_unaligned, PAGE_SIZE * 2, dest_unaligned, PAGE_SIZE * 2,
	    PAGE_SIZE / 2, PAGE_SIZE * 1000);

	free(src_buf);
	free(dest_buf);

	return (0);
}
#endif
