#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#ifdef DEBUG
#include <stdio.h>
#endif

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
	/* sanity check - if no buffer or illegal length - return an empty list */
	if ((buf == NULL) || (length <= 0))
	{
		return (NULL);
	}
	
	sg_entry_t *map_head; 			       /* head or sg map. points to first entry in map 			  */
	sg_entry_t **map_next_ptr = &map_head; /* points to the previous entry pointer to be linked 	  */
	int count;  						   /* helper variable for calculating element's 'count' value */
	/* maintains the offset of the sg entry in PAGE_SIZE block */
	unsigned long byte_offset = (((unsigned long)buf) & (unsigned long)(PAGE_SIZE-1));
		
	while (length > 0)
	{
		/* allocate a new map entry and link it to the list*/
		*map_next_ptr = (sg_entry_t*) malloc(sizeof(sg_entry_t)); 
		if (*map_next_ptr == NULL)
		{
			/* allocation error - cleanup entire map (to avoid ambiguity on partial allocation)*/
			sg_destroy(map_head);
			return (NULL); /* no map was allocated */
		}

		/* evaluate the element's byte count */
		count = ((PAGE_SIZE - byte_offset) < length) ? (PAGE_SIZE - byte_offset) : length;
		
		/* set up the newly created sg map entry */
		(*map_next_ptr)->paddr = ptr_to_phys(buf); /* set paddr of first byte used in entry 	*/
		(*map_next_ptr)->count = count;
		map_next_ptr = &((*map_next_ptr)->next);   
		
		length -= count; 						   /* update remaining bytes to allocate 		*/
		buf += count;			   				   /* get to the next byte to allocate			*/
		byte_offset = 0; 		   				   /* PAGE_SIZE align all entries (except first)*/
		
	} /* while (length > 0) */
	
	*map_next_ptr = NULL; /* terminate the linked list (we know there's at least one entry) */
	
	return (map_head);
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *element = sg_list; /* pointer to next element to be freed */
	sg_entry_t *next;              /* temporary ptr to next element       */
	
	while (element != NULL)
	{
#ifdef DEBUG
		printf ("freeing element @0x%08x\n", element);
#endif		

		next = element->next; 	/* don't want to lose the rest of the list */
		free(element);
		element = next;		  
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
	/* find element and offset in src where start of copied area reside */
	
	while ((src != NULL) && (src_offset >= src->count))
	{
		/* we know that offset is not in this memory block */
		src_offset -= src->count; /* decrement offset by number of skipped bytes */
		src = src->next; 		  /* move on to the next segment */
	}
	
	if (src == NULL)
	{
		/* src is too short to have such src_offset */
		return (0);
	}
	
	/* now src points to the segment holding our desired offset and src_offset holds the offset within the segment */
	unsigned int dest_offset = 0; /* maintains offset within dst segment into next byte that needs to be written */
	int bytes_left_in_segment; /* helper variable - number of uncopied bytes in src segment	*/
	int src_bytes_to_copy;     /* how many bytes in src segment needs to be copied 			*/
	int bytes_in_dest;		   /* helper variable - how many bytes are in dest segment 		*/          
	int dest_bytes_to_copy;    /* how many bytes need to be copied into dest segment		*/
	int orig_count = count;    /* used to calculate number of bytes copied					*/
	
	while ((count > 0) && (src != NULL) && (dest != NULL))
	{
		/* determine how many bytes in current src segment needs to be copied */
		bytes_left_in_segment = src->count - src_offset;
		src_bytes_to_copy = ( (count > bytes_left_in_segment) ? (bytes_left_in_segment) : (count) );
		/* determine physical pointer to start copying from */
		physaddr_t copy_from_phys_addr = src->paddr + src_offset;
		
		/* copy src_bytes_to_copy bytes from current src segment to dest sg list */
		while ((src_bytes_to_copy > 0) && (dest != NULL))
		{
			bytes_in_dest = dest->count - dest_offset; 
			dest_bytes_to_copy = ( (src_bytes_to_copy > bytes_in_dest) ? bytes_in_dest : src_bytes_to_copy );
			
			/* 
				copy bytes from source to destination 
				note that I assume that memcpy uses non-physical addressing
			*/
#ifdef DEBUG
			/* use this for debug since memcpy() will crash the process */
			printf ("memcpy(0x%08x, 0x%08x, %d)\n", 
				(unsigned long)phys_to_ptr(dest->paddr + (physaddr_t)dest_offset),
				(unsigned long)phys_to_ptr(copy_from_phys_addr), 
				dest_bytes_to_copy
				);
#else			
			memcpy (
				phys_to_ptr(dest->paddr + (physaddr_t)dest_offset), 
				phys_to_ptr(copy_from_phys_addr), 
				dest_bytes_to_copy
				);
#endif				
			/* do the housekeeping */
			count -= dest_bytes_to_copy;
			src_bytes_to_copy -= dest_bytes_to_copy;
			dest_offset += dest_bytes_to_copy;
			
			/* get next dest segment if current segment is full */
			if (dest->count <= dest_offset)
			{
				dest = dest->next; /* get next segment */
				dest_offset = 0;   /* start copying to first byte in next segment */
			}
		}
				
		/* get the next source segment to process */
		src = src->next;
		src_offset = 0; /* once first segment was processed - we always start with 0 offset in the src segment */
	}
	
	/* return the number of bytes actually copied */
	return (orig_count - count);
}
