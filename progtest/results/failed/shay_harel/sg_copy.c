// BH: missing includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

sg_entry_t *sg_map(void *buf, int length)
{
	int			chunk = 0;
	int			user_buf_length = length;
	int			byte_count = 0;
	sg_entry_t *sg_entry = NULL, *next_sg_entry = NULL;
	char *		user_buf = (char *)buf;
	sg_entry_t *head = NULL;
	int			first_entry = 1, last_entry;

	/*sanity check*/
	if (buf == NULL || length <= 0){
		return NULL;
	}

	/*we must have at least one entry so let's allocate it and then get into a lop*/
	sg_entry = (sg_entry_t *)malloc (sizeof(sg_entry_t));
	if (sg_entry == NULL){
		return NULL;
	}

	for (;user_buf_length > 0; user_buf_length -= PAGE_SIZE){
		/*let's see how many bytes we do, usually PAGE_SIZE, until we get to the last memory chunk*/
		byte_count = ((user_buf_length >= PAGE_SIZE) ? PAGE_SIZE : user_buf_length);

		/*some house keeping*/
		last_entry = ((user_buf_length >= PAGE_SIZE) ? 0 : 1);

		/*populate the data*/
		sg_entry->paddr = ptr_to_phys((void *)user_buf);/*sg will go to HW so let's map the memory to where it really is*/
		sg_entry->count = byte_count;

		/*if it's the first one, let's remember where the head is so we can return it*/
		if (first_entry){
			head = sg_entry;
			first_entry = 0;
		}

		/*on the last entry, we just NULL the nesxt pointer and return*/
		if (last_entry){
			sg_entry->next = NULL;
			return head;
		}

		/*allocate for next entry*/
		next_sg_entry = (sg_entry_t *)malloc (sizeof(sg_entry_t));
		if (next_sg_entry == NULL){
			/*we should really clean here what we allocated so far*/
			return NULL;
		}

		/*point to the one we just allocated and then move on*/
		sg_entry->next = next_sg_entry;
		sg_entry = next_sg_entry;
		
		/*point ot next virtual memory location*/
		user_buf += PAGE_SIZE;
	}
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *next_entry;

	/*simply start destroying from the start, but beofre we do, just remember what to destory next*/
	while(sg_list->next != NULL){
		next_entry = sg_list->next;
		/*depends on the requirements, we could also free the virtual memory this entry points to*/
		free(sg_list);
		sg_list = next_entry;/*and point to next one*/
	}

	free(next_entry);/*don't forget the last one*/

}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	char *	virt_addr_src = NULL;
	char *	virt_addr_dest = NULL;
	int		bytes_to_copy = 0;

// BH: test input
if (!src || !dest || count <= 0) return 0;

	/*make sure we don't go beyond the size of src->count*/
	if ((src->count - src_offset) < count){
		bytes_to_copy = (src->count - src_offset);
	}else {
		bytes_to_copy = count;
	}

	/*get the virtual memory, since this is what we susually operate on*/
	virt_addr_src = (char *)phys_to_ptr(src->paddr);

	/*move to the offset*/
	virt_addr_src += src_offset;

	/*get the virtual memory of teh source*/
	virt_addr_dest = (char *)phys_to_ptr(dest->paddr);

	/*and copy*/
	memcpy(virt_addr_dest, virt_addr_src, bytes_to_copy);

	/*tell the user how much we did*/
	return bytes_to_copy;
}
