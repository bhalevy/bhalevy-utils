#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sg_copy.h"

// BH
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

static inline PAGE_ALIGN_DOWN(unsigned long p)
{
	return (void *)((p + PAGE_SIZE) & ~(PAGE_SIZE-1));
}

static inline void print_sge(struct sg_entry_s *sge)
{
	printf("phys 0x%lx count %d next %p\n",
	       sge->paddr, sge->count, sge->next);
}

static inline sg_entry_t *alloc_new_sge(unsigned long vaddr, int len)
{
	/* allocate the memory */
	sg_entry_t *sge = calloc(1, sizeof(*sge));
	if (!sge) {
		printf("Failed to allocate an SGE\n");
		return NULL;
	}

	sge->count = len;
	sge->paddr = ptr_to_phys((void *)vaddr);

//	printf("vaddr 0x%lx ", vaddr);
//	print_sge(sge);

	return sge;
}


sg_entry_t *sg_map(void *buf, int length)
{
	unsigned long vbuf = (unsigned long)buf;
	unsigned long next_vpage = PAGE_ALIGN_DOWN(vbuf);
	unsigned int i, full_pages, first_sge_len, last_sge_len;
	sg_entry_t *sg_head, *sge, *sg_tail;


// BH: missing edge conditions check
if (length <= 0 || !buf) return NULL;

	/* Allocate the head SG entry */
	first_sge_len = (unsigned long)(next_vpage - vbuf);
// BH: BUG:
if (first_sge_len > length) first_sge_len = length;
	sg_head = alloc_new_sge(vbuf, first_sge_len);
	if (!sg_head) {
		printf("Failed to allocate an SGE\n");
		return NULL;
	}

	/* Add full pages */
	full_pages = (length - first_sge_len) / PAGE_SIZE;
	sg_tail = sg_head;

//printf("full_pages=%d length=%d first_sge_len=%d\n", full_pages, length, first_sge_len); exit(0);
	for (i = 0; i < full_pages; i++, next_vpage += PAGE_SIZE) {
		/* Allocate the next SG entry */
		sge = alloc_new_sge(next_vpage, PAGE_SIZE);
		if (!sge) {
			printf("Failed to allocate an SGE\n");
			goto error;
		}

		sg_tail->next = sge;
		sg_tail = sge;
	}

	/* Add the last SGE if needed */
	last_sge_len = length - first_sge_len - full_pages * PAGE_SIZE;
	if (last_sge_len) {
		sge = alloc_new_sge(next_vpage, last_sge_len);
		if (!sge) {
			printf("Failed to allocate an SGE\n");
			goto error;
		}

		sg_tail->next = sge;
	}

	return sg_head;
error:
	sg_destroy(sg_head);
	return NULL;
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *next;

//	printf("Destroying the SG:\n");

	while (sg_list) {
		next = sg_list->next;
//		print_sge(sg_list);
		free(sg_list);
		sg_list = next;
	}
}

/**
 * sg_copy_one	Copy bytes from a single SGE into a SG list
 *
 * @src		SGE to copy from
 * @destp	Pointer to destination SG sub-list
 * @src_offset	offset in the source SGE
 * @dst_offsetp pointer to the offset of the next available byte in the *destp
 * @count	number of bytes to copy
 *
 * @ret		Actual number of copied bytes
 *
 * @note	Function will also update the destp according to the number of
 *		used destination SGEs.
 *		Function assumes that count is less or equal to src->count.
 */
static int sge_copy(sg_entry_t *src, sg_entry_t **destp,
			     int src_offset, int *dst_offsetp, int count)
{
	void *vaddr = phys_to_ptr(src->paddr) + src_offset;
	int nbytes = count, dst_offset = *dst_offsetp;
	sg_entry_t *dest = *destp;

	while (dest && count) {
		void *dest_vaddr = phys_to_ptr(dest->paddr) + dst_offset;
		int bytes_to_copy = MIN(count, dest->count - dst_offset);

		memcpy(dest_vaddr, vaddr, bytes_to_copy);

//		printf("Copied %d bytes %p->%p\n",
//		       bytes_to_copy, vaddr, dest_vaddr);
		/*
		 * If no more bytes in the destination SGE - move to the next
		 * one.
		 */
		if (dest->count - dst_offset - bytes_to_copy == 0) {
			dest = dest->next;
			dst_offset = 0;
		} else
			dst_offset += bytes_to_copy;

		vaddr += bytes_to_copy;
		count -= bytes_to_copy;

	}

	*destp = dest;
	*dst_offsetp = dst_offset;

	return nbytes - count;
}

/*
 * This function is same as sg_copy but copying starts from the first SGE from
 * src and src_offset should not be greater than src->count.
 */
static inline int __sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset,
			    int count)
{
	int nbytes = count, dst_offset = 0;
	sg_entry_t *cur_dest = dest;

	/* Copy the first one */
	count -= sge_copy(src, &cur_dest, src_offset, &dst_offset,
			    MIN(src->count - src_offset, count));
	src = src->next;

	while (count && src && cur_dest) {
		count -= sge_copy(src, &cur_dest, 0, &dst_offset,
				    MIN(src->count, count));
		src = src->next;
	}

	return nbytes - count;
}

/**
 * __sge_by_offset	Find the SGE containing the byte with the given offset
 *
 * @src			SG list head
 * @src_offset		offset in the SG list
 * @start_sge		output buffer for a pointer to an SGE we are looking for
 * @offset		pointer to the searched byte inside the returned SGE
 */
static inline int __sge_by_offset(sg_entry_t *src, int src_offset,
				sg_entry_t **start_sge, int *offset)
{
	while (src) {
		if (src_offset < src->count) {
			*offset = src_offset;
			*start_sge = src;
			return 0;
		}

		src_offset -= src->count;
		src = src->next;
	}

	return 1;
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	sg_entry_t *start_sge;
	int rc, offset;

	/* Source, count or destimation is zero - return zero */
	if (!(src && dest && count))
		return 0;

	/* Get the src SGE containing the byte at src_offset */
	rc = __sge_by_offset(src, src_offset, &start_sge, &offset);
	if (rc) {
//		printf("src_offset is outside the src SG list\n");
		return 0;
	}

	return __sg_copy(start_sge, dest, offset, count);
}

#if 0
#define NUM_IN_LINE 10
static void print_arr(char *arr, int size)
{
	int i, j;
	for (i = 0; i < size; i += NUM_IN_LINE) {
		printf("%04d: ", i);
		for (j = 0; j < NUM_IN_LINE; j++) {
			if (i + j >= size)
				break;
			printf("%02x ", arr[j + i]);
		}

		printf("\n");
	}
}

#define MAPPING_SIZE (3 * PAGE_SIZE)

int main()
{
	char buf1[MAPPING_SIZE + 32], buf2[MAPPING_SIZE + 32];
	char *buf1_align = (char *)PAGE_ALIGN_DOWN((unsigned long)&buf1[0]);
	char *buf2_align = (char *)PAGE_ALIGN_DOWN((unsigned long)&buf2[0]);
	int nbytes;

	memset(buf1, 1, sizeof(buf1));
	memset(buf2, 2, sizeof(buf2));
	memset(buf1_align - 3, 3, MAPPING_SIZE);
	memset(buf2_align - 5, 4, MAPPING_SIZE);


	sg_entry_t *sg1 = sg_map(buf1_align - 3, MAPPING_SIZE);
	sg_entry_t *sg2 = sg_map(buf2_align - 5, MAPPING_SIZE);

	nbytes = sg_copy(sg1, sg2, 1, MAPPING_SIZE + 17);

	printf("Copied %d bytes\n", nbytes);

	/* The bytes from the base buffers will have values 01 and 02 for the
	 * first and the second buffer correspondingly.
	 *
	 * The bytes from the mappings will have values 03 and 04 for the first
	 * and the second mapping correspondingly.
	 *
	 * See the bytes values after the copying in the second array.
	 * I print it with some pad from the original buffer to emphasize where
	 * copying begins and ends.
	 */
	printf("First mapping bytes with some pad at the beginning and the end\n");
	print_arr(buf1_align - 10, MAPPING_SIZE + 10);
	printf("Second mapping bytes with some pad at the beginning and the end\n");
	print_arr(buf2_align - 10, MAPPING_SIZE + 10);

	sg_destroy(sg1);
	sg_destroy(sg2);
	return 0;
}
#endif
