#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "sg_copy.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t* first = (sg_entry_t*)malloc(sizeof(sg_entry_t));
	sg_entry_t* cur = first;
	int remain = length;

// BH input validation
if (!buf || length <= 0) return NULL;

	do {
		cur->paddr = ptr_to_phys(buf);
// BH: alignement 		cur->count = MIN(PAGE_SIZE, remain);
		cur->count = MIN(PAGE_SIZE - (cur->paddr & (PAGE_SIZE-1)), remain);
		remain -= cur->count;
		buf += cur->count;
		cur->next = remain ? (sg_entry_t*)malloc(sizeof(sg_entry_t)) : NULL;
		cur = cur->next;
	} while (remain);

	return first;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t* next;
	sg_entry_t* cur = sg_list;
	
	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
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
int sg_copy(sg_entry_t *src, sg_entry_t *dst, int src_offset, int count)
{
	int offset = 0;
	int copied = 0;
	int to_copy;
	void* src_ptr, *dst_ptr;
	int src_len, dst_len;

	/* iterate to the first chunk to be copied */
	while (src && src_offset > src->count) {
		src_offset -= src->count;
		src = src->next;		
	}
// BH input validation	if (!src)
	if (!src || !dst || count <= 0)
		return 0;
	src_len = src->count - src_offset;
	src_ptr = phys_to_ptr(src->paddr) + src_offset;
	dst_len = dst->count;
	dst_ptr = phys_to_ptr(dst->paddr);

	while (count) {
		to_copy = MIN(src_len, dst_len);
		to_copy = MIN(to_copy, count);

		memcpy(dst_ptr, src_ptr, to_copy);
		
		copied += to_copy;
		count -= to_copy;
		src_len -= to_copy;
		src_ptr += to_copy;
		dst_len -= to_copy;
		dst_ptr += to_copy;

		if (!src_len) {
			src = src->next;
			if (!src)
				break;
			src_len = src->count;
// BH: BUG missing update of src_ptr
src_ptr = phys_to_ptr(src->paddr);
		}

		if (!dst_len) {
                        dst = dst->next;
                        if (!dst)
                                break;
                        dst_len = dst->count;
dst_ptr = phys_to_ptr(dst->paddr);
                }

	}
	return copied;
}

static void sg_print(sg_entry_t* s, int c)
{
	int i = 0, cnt =  0;
	while (s) {
		for (i = 0; i < s->count, cnt <=c; i++, cnt++) {
			printf("%c", *(char*)phys_to_ptr(s->paddr) + i);
		}
		s = s->next;
	}
	printf("\n");
	
}

#if 0
int main()
{
	int i, j;
	char buf[500];
 	char buf2[500];
	sg_entry_t* l1, *l2;

	memset(buf, 0, 500);
	for (i = 0; i < 500; i++)
		buf[i] =  i;
	

	l1 = sg_map(buf, 452);
 	l2 = sg_map(buf2, 10);
	printf("%p, phy %p, virt %p char size %d\n", buf, (void*)l1->paddr, (void*)phys_to_ptr(l1->paddr), sizeof(char));

	j = sg_copy(l1, l2, 151, 70);
	printf("copied %d\n", j);
	sg_print(l2, j);
	sg_destroy(l1);
	sg_destroy(l2);
	return 0;
}
#endif
