#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"
/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elements mapping the input buffer
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */

extern sg_entry_t *sg_map(void *buf, int length)
{
// BH: signed length may be < 0
	if (length == 0) return 0;

	/* handle first segment */
	sg_entry_t *first = (sg_entry_t*)malloc(sizeof(sg_entry_t));
	if (first == NULL) {
		printf("malloc failed\n");
		return NULL;
	}
// BH: BUG, first page alignment
//	if (length <= PAGE_SIZE) {
	if (((unsigned)buf & (PAGE_SIZE-1)) + length < PAGE_SIZE) {
		first->paddr = ptr_to_phys(buf);
		first->count = length;
		first->next = NULL;
		return first;
	}

// BH: CPP style
	int first_len = 0;
	sg_entry_t *current = first;
	first->paddr = ptr_to_phys(buf);
	/* in the next line I would use bitwise & operator if I was sure PAGE_SIZE always
	   will be in power of 2, since i'm not.. I will use modulus and build on the compiler */
	if ((first_len =  (unsigned long)buf % PAGE_SIZE) != 0) first->count = first_len;
	else first->count = PAGE_SIZE;
	buf += first->count;
	length -= first->count;
	current = first;
	while (length) {
		if ((current->next = (sg_entry_t*)malloc(sizeof(sg_entry_t))) == NULL) {
			/* rollback */
			sg_destroy(first);
			printf("malloc failed\n");
			return NULL;
			break;
		}
		current = current->next;
		current->paddr = ptr_to_phys(buf);
		current->count = (length < PAGE_SIZE)? length : PAGE_SIZE;
		buf += current->count;
		length -= current->count;
	}
	current->next = NULL;
	return first;
}	


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
extern void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *next = NULL;
	while(sg_list) {
		next = sg_list->next;
		free(sg_list);
		sg_list = next;
	}
}

/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list
 * @in src_offset Offset into source
 * @in count     Number of bytes to copy
 *
 * @ret          Actual number of bytes copied
 *
 * @note         The function copies "count" bytes from "src",
 *               starting from "src_offset" into the beginning of "dest".
 *               The scatter gather list can be of arbitrary length so it is
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 */
extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int cur_real_offset = 0;
	sg_entry_t *cur_src = src;
	sg_entry_t *cur_dest = dest;
	sg_entry_t *src_start = NULL;
	int cur_src_offset = 0;
	int cur_dest_offset = 0;
	int cur_bytes_to_copy = 0;
	int left_count = count;

// BH: signed count
	if (src == NULL || dest == NULL || count == 0) return 0;

	/* find start */
	while(cur_src) {
		if (src_offset < cur_real_offset + cur_src->count) break;
		else {
			cur_real_offset += cur_src->count;
			cur_src = cur_src->next;
		}
	}
	if (cur_src == NULL) return 0;
	cur_src_offset = src_offset - cur_real_offset;

	/* copy */
	while (cur_src && cur_dest && left_count) {
		cur_bytes_to_copy = (cur_src->count - cur_src_offset < cur_dest->count - cur_dest_offset)?
				    cur_src->count - cur_src_offset : cur_dest->count - cur_dest_offset;
		cur_bytes_to_copy = (cur_bytes_to_copy < left_count) ?
				    cur_bytes_to_copy : left_count;

		memcpy((void*)(phys_to_ptr(cur_dest->paddr)+cur_dest_offset),
		       (void*)(phys_to_ptr(cur_src->paddr)+cur_src_offset),
		       cur_bytes_to_copy);

		left_count -= cur_bytes_to_copy;
		cur_src_offset += cur_bytes_to_copy;
		if (cur_src_offset == cur_src->count) {
			cur_src_offset = 0;
			cur_src = cur_src->next;
		}
		cur_dest_offset += cur_bytes_to_copy;
		if (cur_dest_offset == cur_dest->count) {
			cur_dest_offset = 0;
			cur_dest = cur_dest->next;
		}
	}
	return (count - left_count);
}
