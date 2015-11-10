
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "sg_copy.h"

// BH:
#define SG_LIST_END NULL


sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *sg_list, *sg_next, *sg_last;
	int count;
	ptrdiff_t chunck, buf_end;
	
	//calc list size
	buf_end = (ptrdiff_t)buf + length;
	count = (buf_end + PAGE_SIZE - 1)/PAGE_SIZE -  (ptrdiff_t)buf/PAGE_SIZE;
	
	//allocate
	sg_list = (sg_entry_t *) malloc(sizeof(sg_entry_t) * count);
	if(NULL == sg_list)
		return NULL;
	
	sg_last = sg_list + count - 1;
	chunck = (ptrdiff_t)buf;	
	for(sg_next=sg_list; sg_next != sg_last; sg_next++)
	{
		//paddr
		sg_next->paddr = ptr_to_phys((void *)chunck);

		//next chunck
		chunck = ((chunck + PAGE_SIZE)/PAGE_SIZE) * PAGE_SIZE;

		//count/
		sg_next->count =  chunck - (ptrdiff_t)phys_to_ptr(sg_next->paddr);
		
		//link the list
		sg_next->next = sg_next + 1;
	}

	//mark the last entry
	sg_next->next = SG_LIST_END;
	sg_next->paddr = ptr_to_phys((void *)chunck);
	sg_next->count = buf_end - (ptrdiff_t)phys_to_ptr(sg_next->paddr);

	return sg_list;	
}

void sg_destroy(sg_entry_t *sg_list)
{
	free ((void *)sg_list);
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int byte_count = 0, dest_offset = 0;
	
	while (count > 0)
	{
		int to_copy;

		//should not happen after the first byte was copied  
		if (src_offset >= src->count)
		{
			src_offset -= src->count;			
			src = src->next;
			
			if (SG_LIST_END == src)
				return byte_count;

			continue;
		}

		//copy min(src count,dest count, count)
		if(src->count - src_offset > dest->count - dest_offset)
			to_copy = dest->count - dest_offset;
		else
			to_copy = src->count - src_offset;

		if(to_copy > count)
			to_copy = count;

		memcpy((void *)((ptrdiff_t) phys_to_ptr(dest->paddr) + dest_offset),
			(void *)((ptrdiff_t) phys_to_ptr(src->paddr) + src_offset),
			to_copy);

		//update counters
		byte_count += to_copy;
		count -= to_copy;

		//update src entry
		if (to_copy == src->count - src_offset)
		{			
			src = src->next;
			src_offset = 0;
			
			if (SG_LIST_END == src)
				return byte_count;
		}
		else
			src_offset += to_copy;
		
		//update dest entry
		if (to_copy == dest->count - dest_offset)
		{			
			dest = dest->next;
			dest_offset = 0;
			
			if (SG_LIST_END == dest)
				return byte_count;
		}
		else
			dest_offset += to_copy;
	}

	return byte_count;
}

#if 0
int main()
{
	void *buf, *tmp;
	sg_entry_t *sg_list, *dest;
	int copied;

	//some lines to test the sg

	//allocate a buffer
	buf = malloc(0x27);
	memcpy (buf, "Yo Jonny what's up my man, how are you doing?",0x27);
	
	//map to sg list
	sg_list = sg_map(buf, 0x27);
	
	tmp = malloc(0x34);

	dest = sg_map(tmp, 0x34);
	
	copied = sg_copy(sg_list, dest, 0x21, 22);

	return 0;
}
#endif
