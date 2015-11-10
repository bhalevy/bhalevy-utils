#include <stdlib.h>
#include <string.h>
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
	char *curr_buf = (char *)buf;
	int curr_len;
	unsigned int unalignment_bytes;

	if (!length)
		return NULL;

	sg_entry_t *map_head = NULL;
	sg_entry_t *curr_map = NULL;
	sg_entry_t *new_map = NULL;

	unalignment_bytes = (unsigned int)curr_buf % PAGE_SIZE;
	/* first entry */
	if (unalignment_bytes) {
		map_head = (sg_entry_t *)malloc(sizeof(sg_entry_t));
		map_head->paddr = ptr_to_phys(curr_buf);
		map_head->count = (length < unalignment_bytes) ? length : unalignment_bytes;
		length -= map_head->count;
		curr_buf += map_head->count;
		curr_map = map_head;
	}

	/* other entries */
	while (length) {
		new_map = (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if (length < PAGE_SIZE)
			curr_len = length;
		else
			curr_len = PAGE_SIZE;
		new_map->paddr = ptr_to_phys(curr_buf);
		new_map->count = (length < PAGE_SIZE) ? length : PAGE_SIZE;
		new_map->next = NULL;
		if (!map_head) {
			map_head = new_map;
		} else {
			curr_map->next = new_map;
			curr_map = new_map;
		}
		curr_map = new_map;
		curr_buf += new_map->count;
		length -= new_map->count;
	}

	return map_head;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *curr_map, *next_map;

	curr_map = sg_list;
	while (curr_map) {
		next_map = curr_map->next;
		free(curr_map);
		curr_map = next_map;
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copied_bytes = 0;
	int src_bytes, dest_bytes, min_bytes;
	char *src_buffer, *dest_buffer;

	/* make sure we have both source and destination */
	if ((!src) || (!dest))
		return 0;

	/* find the start of the source to be used */
	while (src_offset > src->count) {
		src_offset -= src->count;
		src = src->next;
		if (!src)
			return 0;
	}
        src_bytes = src->count - src_offset;
	src_buffer = (char *)phys_to_ptr(src->paddr) + src_offset;

	/* set the initial destination variables */
        dest_bytes = dest->count;
        dest_buffer = (char *)phys_to_ptr(dest->paddr);

	/* continue copying while we can */
	while (count) {
		/* in each iteration copy the the minimum between the source buffer left bytes and the destinatio buffer
		   left bytes */
		min_bytes = (src_bytes < dest_bytes) ? src_bytes : dest_bytes;
		if (count < min_bytes)
			min_bytes = count;
		/* NOTE: this assume no overlap between the buffers exist (otherwise we should use memmove) */
		memcpy(dest_buffer, src_buffer, min_bytes);

		/* update the counters */
		count -= min_bytes;
		copied_bytes += min_bytes;

		/* update the iteration variables */
		src_bytes -= min_bytes;
		dest_bytes -= min_bytes;
		src_buffer += min_bytes;
		dest_buffer += min_bytes;

		/* if the source map entry is exhausted then move to the next one */
		if (src_bytes == 0) {
			src = src->next;
			if (!src)
				break;
			src_bytes = src->count;
			src_buffer = (char *)phys_to_ptr(src->paddr);
		}

		/* if the destination map entry is exhausted then move to the next one */
		if (dest_bytes == 0) {
			dest = dest->next;
			if (!dest)
				break;
                        dest_bytes = dest->count;
                        dest_buffer = (char *)phys_to_ptr(dest->paddr);
		}
	}

	return copied_bytes;
}

#if 0
int main(int argc, char *argv[])
{
	char src[1024] = "0         10        20        30        40        50        60";
	char dest[1024];

	sg_entry_t *src_map, *dest_map;

	src_map = sg_map(src, 1024);
	dest_map = sg_map(dest, 1024);
	sg_copy(src_map, dest_map, 0, 70);	
	printf("%s\n", dest);
	memset(dest, 0, 1024);

        sg_copy(src_map, dest_map, 10, 70);
        printf("%s\n", dest);
	memset(dest, 0, 1024);

	sg_destroy(dest_map);
	dest_map = sg_map(dest, 30);

    	sg_copy(src_map, dest_map, 0, 70);
        printf("%s\n", dest);
	memset(dest, 0, 1024);

	sg_copy(src_map, dest_map, 20, 70);
        printf("%s\n", dest);
	memset(dest, 0, 1024);
        
	sg_destroy(src_map);
	sg_destroy(dest_map);

	return 0;
}
#endif
