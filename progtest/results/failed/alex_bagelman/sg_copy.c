
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

#define MEM_ALLOC_FAILURE -1

/****************************************************************************************/
/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
extern void sg_destroy(sg_entry_t *sg_list)
// COMMENT: why use extern?
{
	sg_entry_t *item_ptr = sg_list;

	while(sg_list -> next != NULL){		// ERROR - >
		sg_list = sg_list -> next;	// ERROR - >
		free(item_ptr);			// ERROR delete
		item_ptr = sg_list;
	}
	free(item_ptr);				// ERROR delete
// BENNY: cumbersome algorithm but works
}

/****************************************************************************************/
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
	sg_entry_t *sg_list = NULL;
	sg_entry_t *item_ptr;
	int n;

	if(length <= 0){
		return NULL;
	}

	item_ptr = malloc(sizeof(sg_entry_t));	// ERROR new
	while(1){				// BENNY: tend to use infinite loops, what's the iterator, end condition?
		if(item_ptr != NULL){

			if(sg_list == NULL){
				sg_list = item_ptr;
			}

//			item_ptr -> count = length & (PAGE_SIZE - 1);	// BUG: does not take into account ptr alignment
n = PAGE_SIZE - ((unsigned long)buf & (PAGE_SIZE - 1));
if (n > length)
	n = length;
item_ptr->count = n;
			if(item_ptr -> count == 0){
				item_ptr -> count = PAGE_SIZE;
			}

			item_ptr -> paddr = ptr_to_phys(buf);
			buf = (char *)buf + item_ptr -> count;
			length -= item_ptr -> count;

			if(length > 0){
				item_ptr -> next = malloc(sizeof(sg_entry_t));	// ERROR new
				item_ptr = item_ptr -> next;
			}else{
				item_ptr -> next = NULL;
				break;
			}

		}else{

			if(sg_list != NULL){
				sg_destroy(sg_list);
			}
			sg_list = (sg_entry_t *)MEM_ALLOC_FAILURE;  /* No specific parameter provided, */
														/* so I use the defined above return */
														/* code for an error indication */
			break;
		}
	}
	return sg_list;
}
/****************************************************************************************/
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
 *               starting from "src_offset" into "dest".
 *               The scatter gather list can be of arbitrary length so it is
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 */
extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{

// COMMENT: sloppy indentation
sg_entry_t *trg_page_ptr = dest;
sg_entry_t *src_page_ptr = src;		// ERROR: typo
char       *src_buf_ptr;
char       *trg_buf_ptr;
int         src_page_off;
int         trg_page_off = 0;
int         cur_off = 0;
int         len;
int         total_copied = 0;

 if((src == NULL) || (count == 0) || (dest == NULL)){
	return 0;
 }

 /* find the starting src page */
 while(1){			// COMMENT: infinite loop again
 	if(cur_off + src_page_ptr -> count >= src_offset ){ /* src_page_ptr points on the starting page */
 		src_page_off = src_offset - cur_off;            /* src_page_off gives the starting point on page */
		break;
	}
	if(src_page_ptr -> next == NULL){
		return 0;
	}
	cur_off += src_page_ptr -> count;
	src_page_ptr = src_page_ptr -> next;
 }
 /* copying */
 while(1){			// COMMENT: infinite loop again
	len = src_page_ptr -> count - src_page_off;
	if(len > count){
		 len = count;
	}
	if( len > trg_page_ptr -> count - trg_page_off ){
		len = trg_page_ptr -> count - trg_page_off;
	}
 	src_buf_ptr = (char*)phys_to_ptr(src_page_ptr -> paddr) + src_page_off;
 	trg_buf_ptr = (char*)phys_to_ptr(trg_page_ptr -> paddr) + trg_page_off;
 	memcpy(trg_buf_ptr, src_buf_ptr, len);
 	total_copied += len;
 	count -= len;
 	if(count == 0){
		break; /* done */
	}
	if(src_page_off + len == src_page_ptr -> count){ /* nothing to take from the page */
		if(src_page_ptr -> next == NULL){
			break; /* end of src list */
		}
		src_page_ptr = src_page_ptr -> next;
		src_page_off = 0;
	}else{
		src_page_off += len;
	}
	if(trg_page_off + len == trg_page_ptr -> count){ /* trg page is full */
		if(trg_page_ptr -> next == NULL){
			break; /* end of trg list */
		}
		trg_page_ptr = trg_page_ptr -> next;
		trg_page_off = 0;
	}else{
		trg_page_off += len;
	}
 }

 return total_copied;
}
