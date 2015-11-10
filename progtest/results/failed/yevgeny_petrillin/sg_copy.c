#include <stdio.h>
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
sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *head, *tmp;

	/* create first SG entry */
	head = (sg_entry_t *)malloc(sizeof(sg_entry_t));
	if (head == NULL)
		return NULL;

	head->paddr = ptr_to_phys(buf);
	head->next = NULL;

	/* calculating the distance from buf to next PAGE_SIZE aligned address */
	head->count = PAGE_SIZE - ((int)buf & (PAGE_SIZE - 1));
	if (head->count > length)
		head->count = length;
	buf += head->count;
	length -= head->count;
	
	tmp = head;
	/* create entries until we create all of them */
	while (length > 0) {
		tmp->next = (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if (tmp->next == NULL) {
			sg_destroy(head);
			return NULL;
		}
		tmp = tmp->next;
		tmp->paddr = ptr_to_phys(buf);
		tmp->next = NULL;
		tmp->count = (length < PAGE_SIZE) ? length : PAGE_SIZE;
		length -= tmp->count;
		buf += tmp->count;
	}
	return head;
}


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *tmp;

	while(sg_list != NULL) {
		tmp = sg_list->next;
		free(sg_list);
		sg_list = tmp;
	}
}


/*
 * sg_lenght	Calculate length in bytes of SG list
 *
 * @in sg_list	A scatter-gather list
 *
 * @ret		length in bytes
 */
static int sg_length(sg_entry_t *sg_list)
{
	int ret = 0;

	while (sg_list != NULL) {
		ret += sg_list->count;
		sg_list = sg_list->next;
	}
	return ret;
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int src_len, dest_len, ret;
	void *src_buf, *dest_buf;

// BH: inefficient
	src_len = sg_length(src);
	src_len -= src_offset;
	dest_len = sg_length(dest);

	ret = (src_len < dest_len) ? src_len : dest_len;
	ret = (ret < count) ? ret : count;

	if (ret <= 0)
		return 0;

// BH: incorrect
	src_buf = phys_to_ptr(src->paddr);
	src_buf += src_offset;
	dest_buf = phys_to_ptr(dest->paddr);

// BH: wrong order
//	memcpy(src_buf, dest_buf, ret);
	memcpy(dest_buf, src_buf, ret);

	return ret;
}

