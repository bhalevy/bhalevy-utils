#include "sg_copy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OS_FREE(ptr) free(ptr)
//#define ERROR printf
#define ERROR(_s_) printf("%s\n", _s_);
//#define OS_ALIGNED_MALLOC(size, align) _aligned_malloc(size, align) // GCC has an equivalent
//#define OS_ALIGNED_FREE(ptr) _aligned_free(ptr) // GCC has an equivalent
#define OS_ALIGNED_MALLOC(size, align) malloc(size) // GCC has an equivalent
#define OS_ALIGNED_FREE(ptr) free(ptr) // GCC has an equivalent
#define OS_MEMCPY(dest, src, len) memcpy(dest, src, len)
#define MIN(a,b) ((a)<(b) ? (a) : (b))

extern sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg_map = NULL;
	sg_entry_t *iter = NULL;
	
	if ((NULL == buf) || (length <=0))
	{
//		ERROR("Invalid Params");
		return NULL;
	}

	sg_map = (sg_entry_t *)OS_ALIGNED_MALLOC(sizeof(sg_entry_t), PAGE_SIZE);
	if (NULL == sg_map)
	{
		ERROR("aligned mem allocation failed.");
		return NULL;
	}
	iter = sg_map; 
	for (; ;)
	{
// BH:		iter->count = MIN(length, PAGE_SIZE);
		iter->count = MIN(length, PAGE_SIZE - ((unsigned long)buf & (PAGE_SIZE-1)));
		iter->paddr = ptr_to_phys(buf);
// BH: ???		(char *)buf += iter->count;
		buf += iter->count;
		length -= iter->count;
		if (length<=0)
		{
			break;
		}
		
		iter->next = (sg_entry_t *)OS_ALIGNED_MALLOC(sizeof(sg_entry_t), PAGE_SIZE);
		if (NULL == iter->next)
		{
			sg_destroy(sg_map);
			ERROR("aligned mem allocation failed.");
			return NULL;
		}
		iter = iter->next;
	}
	iter->next = NULL;
	/* sanity check */
	if (length<0)
	{
		sg_destroy(sg_map);
		ERROR("BUG: memory corruption!");
		return NULL;
	}
	return sg_map;
}

extern void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *iter = sg_list;

	for (; iter!=NULL;)
	{
		sg_entry_t *temp = iter;
		iter=iter->next;
		OS_ALIGNED_FREE(temp);
	}
}

extern void sg_print(sg_entry_t *sg_list, char* buf)
{
	sg_entry_t *iter = sg_list;
	int offset = 0;

	for (; iter!=NULL; iter=iter->next)
	{
		printf("physical addr: %p buf pos: %p %s\n", phys_to_ptr(iter->paddr), buf+offset, (phys_to_ptr(iter->paddr) == buf+offset) ? "OK" : "BAD");
		offset+=iter->count;
	}
}

extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int total_copied = count;
	int dest_offset = 0;
	
	if ((NULL == src) || (NULL == dest) || (count<=0))
	{
//		ERROR("Bad Params");
//		return -1;
return 0;
	}

	/* find offset location in src list */
	for (; src!=NULL; src=src->next)
	{
		if (src_offset < src->count)
		{
			break;
		}
		
		src_offset -= src->count;
	}
	if (NULL == src)
	{
//		ERROR("src_offset out of bounds");
//		return -1;
return 0;
	}

	/* start copy */
	for (; count>0;)
	{
		char *src_buf = (char *)phys_to_ptr(src->paddr);
		char *dest_buf = (char *)phys_to_ptr(dest->paddr);
		int min_part_size = MIN(src->count-src_offset, dest->count-dest_offset);
		int bytes_to_copy = MIN(min_part_size, count);

		OS_MEMCPY(dest_buf + dest_offset, src_buf + src_offset, bytes_to_copy);

		count -= bytes_to_copy;
		dest_offset += bytes_to_copy;
		src_offset += bytes_to_copy;

		if (src->count == src_offset)
		{
			src = src->next;
			src_offset = 0;
			if (NULL == src)
			{
				break;
			}
		}
		if (dest->count == dest_offset)
		{
			dest = dest->next;
			dest_offset = 0;
			if (NULL == dest)
			{
				break;
			}
		}
	}
	/* sanity check */
	if (count<0)
	{
		ERROR("BUG: memory corruption!");
		return -1;
	}

	return total_copied - count;
}
