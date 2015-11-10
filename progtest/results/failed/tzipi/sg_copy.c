#include "sg_copy.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

typedef int BOOL;
#define TRUE 1
#define FALSE 0

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
	sg_entry_t *sg_list = NULL;
	sg_entry_t *curr_elem = NULL;
	sg_entry_t *prev_elem = NULL;
	int size_of_first_elem = 0;
	int i;
	BOOL is_failed = FALSE;

	void *curr_buf = NULL;

	if( length <= 0 ) {
		printf( "Given length %d is illegal\n", length );
		is_failed = TRUE;
		goto cleanup;
	}

	
	curr_elem = malloc( sizeof( sg_entry_t ) );
	if( !curr_elem ) {
		printf("Failed to malloc curr_elem\n");
		is_failed = TRUE;
		goto cleanup;
	}
	bzero( curr_elem, sizeof( sg_entry_t ) );
// BH: BUG
//	size_of_first_elem = ( (length % PAGE_SIZE) > 0 )  ? length % PAGE_SIZE : PAGE_SIZE;
size_of_first_elem = PAGE_SIZE - ((unsigned)buf & (PAGE_SIZE-1));
if (size_of_first_elem > length) size_of_first_elem = length;

	curr_elem->count = size_of_first_elem;
	curr_elem->paddr = ptr_to_phys( buf );
	
	sg_list = curr_elem; /* point to first elem in list */
	prev_elem = curr_elem;
	
	curr_elem = NULL;
	curr_buf = buf;

	for( i = size_of_first_elem; i < length; i += PAGE_SIZE ) {
		curr_elem = malloc( sizeof( sg_entry_t ) );
		if( !curr_elem ) {
			printf("Failed to malloc curr_elem\n");
			is_failed = TRUE;
			goto cleanup;
		}
		bzero( curr_elem, sizeof( sg_entry_t ) );

// BH: Odd, wahhhh
//		curr_buf = curr_buf + i;
		curr_buf = buf + i;
		
		curr_elem->count = PAGE_SIZE;
// BH: BUG: end condition
if (curr_elem->count > length - i) curr_elem->count = length - i;
		curr_elem->paddr = ptr_to_phys( curr_buf );
		
		prev_elem->next = curr_elem;

		prev_elem = curr_elem;
		curr_elem = NULL;
	}
	
cleanup:

	if( is_failed ) {
		if( sg_list ) {
			sg_destroy( sg_list );
			sg_list = NULL;
		}
	}

	return sg_list;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *curr_elem = NULL;
	sg_entry_t *prev_elem = NULL;

	curr_elem = sg_list;

	while( curr_elem ) {
		prev_elem = curr_elem;
		curr_elem = curr_elem->next;
		
		free( prev_elem );
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
	void *src_ptr = NULL;
	void *dest_ptr = NULL;

	sg_entry_t *src_curr_elem = NULL;
	sg_entry_t *dest_curr_elem = NULL;

	int elem_indx = 1;
	int offset_in_elem = 0;

	int avail_from_src_elem = 0;

	int total_copied = 0;
	int copied = 0;
	int amount_to_cpy = 0;

	int i = 0;

	BOOL get_new_ptr = TRUE;

	if( !src ) {
		printf("src list is empty\n");
		return 0;
	}

	if( !dest ) {
		printf("dest list is empty\n");
		return 0;
	}

	if( count == 0 ) {
		printf("amount to copy is 0\n");
		return 0;
	}

	/* find offset */
	elem_indx = (src_offset - src->count) / PAGE_SIZE;
	offset_in_elem = (src_offset - src->count) % PAGE_SIZE;
	offset_in_elem = offset_in_elem < 0 ? 0 : offset_in_elem;

	src_curr_elem = src;
	for( i = 1; i < elem_indx; i++ ) {
		if( src_curr_elem ) {
			src_curr_elem = src_curr_elem->next;
		}
	}
	
	if( !src_curr_elem ) {
		printf("given offset=%d is out of scope of src list\n", src_offset );
		return 0;
	}
	
	src_ptr = phys_to_ptr( src_curr_elem->paddr );

	src_ptr += offset_in_elem;
	avail_from_src_elem = src_curr_elem->count - offset_in_elem;

	dest_curr_elem = dest;
	while( dest_curr_elem ) {
		copied = 0;
		dest_ptr = phys_to_ptr( dest_curr_elem->paddr );
		while( copied <= dest_curr_elem->count ) {
			if( !src_ptr ) {
				if( src_curr_elem ) {
					src_ptr = phys_to_ptr( src_curr_elem->paddr );
					avail_from_src_elem = src_curr_elem->count;
				} 
				else {
					break;
				}
			}
			if( avail_from_src_elem > dest_curr_elem->count ) {
				amount_to_cpy = dest_curr_elem->count;
				avail_from_src_elem -= dest_curr_elem->count;
				memcpy( dest_ptr, src_ptr, amount_to_cpy );
				copied += amount_to_cpy;
				total_copied += amount_to_cpy;
				src_ptr += amount_to_cpy; /* advance src_ptr with the amount of bytes already copied */
			} 
			else {
				amount_to_cpy = avail_from_src_elem;
				 
				memcpy( dest_ptr, src_ptr, amount_to_cpy);
				copied += amount_to_cpy;
				total_copied += amount_to_cpy;

				if( src_curr_elem ) {
					src_curr_elem = src_curr_elem->next;
				}
				src_ptr = NULL;
			}
		}

		if( !src_curr_elem ) {
			/* no more to copy from */
			break;
		}

		dest_curr_elem = dest_curr_elem->next;
	}


	return total_copied;

}





