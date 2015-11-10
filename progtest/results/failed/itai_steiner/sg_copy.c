#include "sg_copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DEFINE/MACRO */
#define log(x) 	(printf(x))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

/* STATIC PROTOTYPES */
static void fill_entry(sg_entry_t *sg_element, void* buf, unsigned int length);
static void copy_chunk(sg_entry_t *src, unsigned int src_offset, sg_entry_t * dest, unsigned int dest_offset, unsigned int chunk_cnt);

/* FUNCTIONS DECLARATION */
 sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg_list, *sg_element;
	unsigned int alloc_cnt, chunk_size, align_offset;

	//Sanity
	if ((length < 1) || (buf == NULL))
	{
// BH		log("Invalid arguments\n");
		return NULL;
	}
	
	//Init list
	sg_list = (sg_entry_t*)malloc(sizeof(sg_entry_t));
	if (sg_list == NULL)
	{
// BH		log("Mem allocation failed\n");
		return NULL;
	}

	sg_element = sg_list;
	alloc_cnt = 0;
// BH: BUG	align_offset = length % PAGE_SIZE;
#if 0
	// Calculate first chunk size
	if ( align_offset != 0 )
	{
		chunk_size = align_offset;
	}
	else
	{
		chunk_size = PAGE_SIZE;
	}
#endif

chunk_size = MIN(PAGE_SIZE - (unsigned long)buf % PAGE_SIZE, length);

	// Map first chunk
	fill_entry(sg_element, buf, chunk_size);
	alloc_cnt = chunk_size;

	// Map remaining chunks
	while (alloc_cnt < length)
	{
		sg_element->next = malloc(sizeof(sg_entry_t));
		if (sg_element->next == NULL)
		{
// BH			log("Mem allocation failed\n");
// BH: BUG: should destroy list
			return NULL;
		}
		sg_element = sg_element->next;
// BH: truncate count to remaining length
//		fill_entry(sg_element, buf + alloc_cnt, PAGE_SIZE);
//		alloc_cnt += PAGE_SIZE;
chunk_size = MIN(PAGE_SIZE, length - alloc_cnt);
fill_entry(sg_element, buf + alloc_cnt, chunk_size);
alloc_cnt += chunk_size;
	}

	return sg_list;
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t  *sg_next_element;

	while (sg_list != NULL)
	{
		sg_next_element = sg_list->next;
		free (sg_list);
		sg_list = sg_next_element;
	}
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	unsigned int total_cnt, len_to_copy;
	unsigned int dest_offset, dest_len, src_len;

	//sanity
	if ((src == NULL) || (dest == NULL))
	{
// BH		log("Invalid arguments\n");
		return 0;
	}

	// Find first src chunk to start copying from
	while (src_offset > (src->count))
	{
		src_offset -= (src->count);
		src = src->next;
		if (src == NULL)
		{
// BH			log("src list is too short for offset\n");
			return 0;
		}
	}

	//prepare to copy
	total_cnt = 0;
	dest_offset = 0;
	dest_len = dest->count;	//length of available bytes in dest chunk for write
	src_len = (src->count) - src_offset; //length of available bytes in src chunk to read

	//copy
	while (count > 0)
	{
		if (src_len > dest_len)
		{
			//src chunk has more bytes to read than dest chunk can write
			len_to_copy = MIN(count, dest_len);
			copy_chunk(src, src_offset, dest, dest_offset, len_to_copy);
			total_cnt += len_to_copy;
			count-=len_to_copy;
			src_offset += len_to_copy;
			src_len -= len_to_copy;

			//continue to next dest chuck
			dest=dest->next;
			if (dest == NULL)
			{
				break;
			}
			dest_len = dest->count;
			dest_offset = 0;
		}
		else	//src chuck has less or equal bytes to read than than available dest chunk bytes
		{
			len_to_copy = MIN(count, src_len);
			copy_chunk(src, src_offset, dest, dest_offset, len_to_copy);
			total_cnt += len_to_copy;
			count -= len_to_copy;
			dest_offset += len_to_copy;
			dest_len -= len_to_copy;

			//switch to the next src chunk
			src=src->next;
			if ((src == NULL) || (count == 0))
			{
				break;
			}
			src_len=src->count;
			src_offset = 0;

			if (dest_len == 0)	// dest chunk had exactly the same available len as src to read
			{
				//continue to the next dest chunk
				dest = dest->next;
				if (dest == NULL)  //dest end of list
				{
					break;
				}
				dest_len = dest->count;
				dest_offset = 0;
			}
		}
	}
	return total_cnt;
}

/* STATIC FUNCTIONS DECLARATION */
static void fill_entry(sg_entry_t *sg_element, void* buf, unsigned int length)
{
	sg_element->count = length;
	sg_element->paddr = ptr_to_phys(buf);
	sg_element->next = NULL;
}

static void copy_chunk(sg_entry_t *src, unsigned int src_offset, sg_entry_t * dest, unsigned int dest_offset, unsigned int chunk_cnt)
{
	void *dest_virt_addr, *src_virt_addr;

	//get virtual address
	dest_virt_addr = phys_to_ptr(dest->paddr + dest_offset);
	src_virt_addr = phys_to_ptr(src->paddr + src_offset);

	memcpy(dest_virt_addr, src_virt_addr, chunk_cnt);
}
