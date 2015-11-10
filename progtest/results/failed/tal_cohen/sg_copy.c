#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

// BH: missing definition for "min"
#define min(a, b) ((a) <= (b) ? (a) : (b))

//static function to create list element
// BH: typo static inline struct sg_entry_t* create_element(const void* buf, const void* p_buf_end) {
static inline sg_entry_t* create_element(const void* buf, const void* p_buf_end) {

	sg_entry_t* p = NULL;

	p        = (sg_entry_t *)malloc(sizeof(sg_entry_t));
	p->paddr = ptr_to_phys(buf);


// BH: typo	p->count = min(PAGE_SIZE - (p->paddr % PAGE_SIZE), p_buf_end - p_buf);
	p->count = min(PAGE_SIZE - (p->paddr % PAGE_SIZE), p_buf_end - buf);
	p->next = NULL;

	return p;
}

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
extern sg_entry_t *sg_map(void *buf, int length) {

	sg_entry_t* head          = NULL;
	sg_entry_t* p_new_element = NULL;
	sg_entry_t* p             = NULL;

	void*  p_buf              = NULL;
	void*  p_buf_end          = NULL;

	int element_count;


// BH: negative length
	if(length == 0 || buf == NULL) return NULL;

	for(p_buf = buf, p_buf_end = buf + length ; p_buf < p_buf_end ; p_buf += element_count) {

		//crerate a new list element
		p_new_element = create_element(p_buf, p_buf_end);

		//check if success to create a new element
		if(p_new_element == NULL) return NULL;

		element_count = p_new_element->count;

		//in case first
		if(head == NULL) {
			head = p_new_element;
			p    = head;
		}
		else {
			p->next = p_new_element;
			p       = p->next;
		}

	}

	//return head of the list
	return head;

}



/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
 extern void sg_destroy(sg_entry_t *sg_list) {


	 sg_entry_t* p_destroy = NULL;
	 sg_entry_t* p         = sg_list;

	 while(p != NULL) {

		 p_destroy = p;
		 p         = p->next;
		 free(p_destroy);

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
 extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) {

	 int sum_offset     = 0;
	 int rc_total_bytes = 0;

	 int node_offset;
	 int node_bytes;
	 
	 sg_entry_t* p_src  = src;
	 sg_entry_t* p_dest = dest;

	 //check for empty dest
	 if(dest == NULL || count == 0) return 0;

	 //step over until offset
	 while(p_src != NULL && (sum_offset + src->count) < src_offset) {

		 sum_offset += p_src->count;
		 p_src       = p_src->next;
	 }

	 if(p_src == NULL) return 0;
	 
	 //if we here, p_src points to the actual area
	 node_offset = (sum_offset + p_src->count) - src_offset;
	 node_bytes  = p_src->count - node_offset;
// BH: BUG: dest may be too small
	 memcpy(phys_to_ptr(p_dest->paddr), phys_to_ptr(p_src->paddr + node_offset), node_bytes);

	 rc_total_bytes += node_bytes;
	 count          -= node_bytes;   //remove from 'count' copied bytes

	 p_src  = p_src->next;
	 p_dest = p_dest->next;

	 while(p_src != NULL && p_dest != NULL && count > 0) {
	 
		node_bytes = min(p_src->count, count);   //the min is for last element
		memcpy(phys_to_ptr(p_dest->paddr), phys_to_ptr(p_src->paddr), node_bytes);
		
		rc_total_bytes += node_bytes;
		count          -= node_bytes;

		p_src  = p_src->next;
		p_dest = p_dest->next;
	 }

	 return rc_total_bytes;
}
