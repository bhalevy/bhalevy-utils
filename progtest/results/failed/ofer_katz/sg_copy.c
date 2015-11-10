
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
sg_entry_t *sg_map(void *buf, int length)
{
	int offset;
	int sg_entry_num;
	sg_entry_t *ret_val;
	sg_entry_t *curr_entry;

// BH: input validation
if (!buf || length <= 0) return NULL;

	/* check if buf address is PAGE_SIZE aligned */
// BH: BUG	offset = PAGE_SIZE - ( (physaddr_t)buf & (PAGE_SIZE-1) );
offset = PAGE_SIZE - ( (unsigned long)buf & (PAGE_SIZE-1) );
	offset %= PAGE_SIZE;
	
// BH: BUG: cap offset
if (offset > length) offset = length;
	
	/* Calculate the number of required sg entries for the specified buffer length */
	length -= offset;
	sg_entry_num = length / PAGE_SIZE;
	
	/* If length does not divide evenly by PAGE_SIZE the list will have an additional (last) entry with the remaining bytes */
	if(length % PAGE_SIZE)
		++sg_entry_num;
	
	/* If buf address is not PAGE_SIZE aligned the first entry of the sg list shall contain the first few bytes up to the nearest PAGE_SIZE aligned address*/
	if(offset > 0)
		++sg_entry_num;

	/* Attempt to allocate all the required memory for the sg_list containers (this simplifies the function flow) */
	ret_val = (sg_entry_t*)malloc(sizeof(sg_entry_t) * sg_entry_num);
	if(ret_val == NULL)
	{
		printf("Not enough resources for creating an SG list!!!\n");
		return NULL;
	}

	curr_entry = ret_val;
	if(offset > 0)
	{
		curr_entry->count = offset;
		curr_entry->paddr = ptr_to_phys(buf);
// BH: BUG
if (!length) {
	curr_entry->next = NULL;
	return curr_entry;
}
		curr_entry->next = curr_entry + 1;
		buf = (void*)((char*)buf + offset);
		++curr_entry;
	}
	
	while(length > PAGE_SIZE)
	{
		curr_entry->count = PAGE_SIZE;
		curr_entry->paddr = ptr_to_phys(buf);
		curr_entry->next = curr_entry + 1;
		buf = (void*)((char*)buf + PAGE_SIZE);
		++curr_entry;
		length -= PAGE_SIZE;
	}
	
	/* Set the last entry of the SG list */
	curr_entry->count = length;
	curr_entry->paddr = ptr_to_phys(buf);
	curr_entry->next = NULL;
	
	return ret_val;
}


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	/* freeing the allocated SG list containers */
	free(sg_list);
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
	int byte_copied = 0;
	int dest_offset = 0;

// BH
if (count <= 0 || src_offset < 0) return 0;
	
	/* Find the requested offset in the SG list */
	while( (src != NULL) && (src->count < src_offset) )
	{
		src_offset -= src->count;
		src = src->next;
	}
	
	/* Copy the requested range or until reaching one of the lists ends */
	while( (src != NULL) && (dest != NULL) && (count > 0) )
	{
		int bytes_to_copy;
		int src_entry_bytes_left = src->count - src_offset;
		int dest_entry_bytes_left = dest->count - dest_offset;
		void *source = (void*)( (physaddr_t)phys_to_ptr(src->paddr) + src_offset );
		void *destination = (void*)( (physaddr_t)phys_to_ptr(dest->paddr) + dest_offset );
		
		if(src_entry_bytes_left > dest_entry_bytes_left)
		{
			/* Copy the data to the remaining destination buffer in the current destination SG entry */
			bytes_to_copy = dest_entry_bytes_left;
			src_offset += dest_entry_bytes_left;
			/* Reset the offset and advance to the next entry in the destination SG list */
			dest_offset = 0;
			dest = dest->next;
		}
		else if(src_entry_bytes_left < dest_entry_bytes_left)
		{
			/* Copy the data from the remaining source buffer in the current source SG entry */
			bytes_to_copy = src_entry_bytes_left;
			dest_offset += src_entry_bytes_left;
			/* Reset the offset and advance to the next entry in the source SG list */
			src_offset = 0;
			src = src->next;
		}
		else
		{
			/* Copy the remaining data in the current source SG entry to the current destination SG entry*/
			bytes_to_copy = dest_entry_bytes_left;
			/* Reset the offset and advance to the next entry in the source and destination SG list */
			src_offset = 0;
			dest_offset = 0;
			src = src->next;
			dest = dest->next;
		}
		
		/* Update the number of bytes left to copy after this iteration */
		if(count < bytes_to_copy)
		{
			bytes_to_copy = count;
			count = 0;
		}
		else
		{
			count -= bytes_to_copy;
		}
		
		/* Copy the source data to the destination */
		memcpy( destination,source, bytes_to_copy );
		byte_copied += bytes_to_copy;
	}
	
	return byte_copied;
}


