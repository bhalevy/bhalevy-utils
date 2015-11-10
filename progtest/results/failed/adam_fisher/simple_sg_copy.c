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
sg_entry_t *sg_map(void *buf, int length){

    /* the number of nodes to be returned */	
	int num_nodes;
	/* the entry point to the sg_entry map */
	sg_entry_t *ptr;
	/* the current and next nodes */
    sg_entry_t *current_ptr, *next_ptr;
	/* the current and next chunks */
	void *chunk;
	/* memory alignment offset requirement */
	physaddr_t alignment_offset;
	
    /* assign list storage for the first node */	
	ptr = (sg_entry_t *)malloc(sizeof(sg_entry_t));
	
	/* store address for first chunk in first list node */
	ptr->paddr = ptr_to_phys(buf);
	
	/* simple case, single chunk */
	if (length <= PAGE_SIZE){
		/* set size of node's chunk */
		ptr->count = length;
		/* reset next node */
		ptr->next = NULL;

		return ptr;
	} /* simple case, single chunk */
	
	/* determine alignment offset */
	alignment_offset = PAGE_SIZE - (ptr_to_phys(chunk) % PAGE_SIZE);
	/* set useable buffer length according to alignment requirement */
	length -= alignment_offset;
	
	/* calculate the number of nodes required */
	num_nodes = (length / PAGE_SIZE);
	if ((length % PAGE_SIZE) > 0) num_nodes++;

	/* set size of first node's chunk */
	ptr->count = PAGE_SIZE;
	
	/* locate and align next chunk position */
	chunk = buf + PAGE_SIZE; /* unaligned starting position for next chunk */
	chunk += alignment_offset; /* add alignment offset */
	
	/* initialize current node */
	current_ptr = ptr;
	
	/* add the rest of chunks */
	while (--num_nodes > 0){
          
        /* assign list storage for the next node */	
    	next_ptr = (sg_entry_t *)malloc(sizeof(sg_entry_t));
        /* link to the next structure */
        current_ptr->next = next_ptr;
        /* point to the next structure */
        current_ptr = next_ptr;
        
    	/* store address for new chunk in current list node */
    	current_ptr->paddr = ptr_to_phys(chunk);
    	
    	/* the last chunk may be smaller */
    	if ((num_nodes == 1) && ((length % PAGE_SIZE) >  0)){
            /* shorter chunk */
                       
        	/* set size of node's chunk */
        	current_ptr->count = length % PAGE_SIZE;
        	
        	/* shorter chunk */
        } else {
            /* regular chunk */
               
        	/* set size of node's chunk */
        	current_ptr->count = PAGE_SIZE;
        	/* locate the next chunk position */
        	chunk += PAGE_SIZE;
        	
        } /* regular chunk */

		/* reset next node */
		current_ptr->next = NULL;
    	
    } /* add the rest of chunks */
    
	return ptr;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list){
     
	/* the current and next nodes */
    sg_entry_t *current_ptr, *next_ptr;
    
    /* free memory chunks */
    free(phys_to_ptr(sg_list->paddr));

    /* initialize current node */     
    current_ptr = sg_list;

    /* loop over list for node destruction */    
    while (current_ptr->next != NULL){
          next_ptr = current_ptr->next;
          free(current_ptr);
          current_ptr = next_ptr;
    }
    
    /* free last node */
    free(current_ptr);
    
	return;
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
extern int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count){
       
    /* pointers for traversing source and destination nodes */
    sg_entry_t *src_node = src, *dest_node = dest;
    /* pointer to first byte to be copied */
    char *src_start;
    /* number of bytes that can be copied */
    int actual, src_actual, dest_actual;
       
    /* attempt to locate the source offset */
    /* reduce offset by the length of the chunk as we progress through the nodes */
    while ((src_node->next != NULL) && (src_node->count < src_offset)){
          
          /* reduce offset */
          src_offset -= src_node->count;
          /* move to next node */
          src_node = src_node->next;
          
    } /* attempt to locate the source offset */
    
    /* we have located the source offset */
    /* transfer the number of bytes possible */
    if (src_node->count > src_offset){
                        
        /* set initial byte to be copied */
        src_start = phys_to_ptr(src_node->paddr) + src_offset;
        
        /* calculate number of bytes that can be copied from source */
        src_actual = src_node->count - src_offset;
        while (src_node->next != NULL){
              src_node = src_node->next;
              src_actual += src_node->count;
        }

        /* calculate number of bytes that can be copied to destination */
        dest_actual = 0;
        while (dest_node->next != NULL){
              dest_actual += dest_node->count;
              dest_node = dest_node->next;
        }
        dest_actual += dest_node->count;
        
        /* the actual number transferable is the minimum */
        actual = (src_actual < dest_actual) ? src_actual : dest_actual;
        
        /* copy the possible number of bytes */
        memmove(
                phys_to_ptr(dest->paddr),
                src_start,
                actual
                );
        
        /* return the number of bytes copied */
        return actual;
        
    } /* we have located the source offset */
    
    /* the offset exceeds the length of the source buffer */
    /* we copy nothing */
	return 0;
}
