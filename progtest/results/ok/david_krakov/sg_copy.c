
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

/* 
 * helper func: allocate a sg entry and set its properties.
 * may return NULL on allocation error.
 */
static sg_entry_t* make_sg_entry(void* buf, int count) {
	sg_entry_t* sg_entry;
	sg_entry = malloc(sizeof(struct sg_entry_s));
	if (sg_entry != NULL) {
		sg_entry->paddr = ptr_to_phys(buf);
		sg_entry->count = count;
	}
	return sg_entry;
}
 
sg_entry_t *sg_map(void *buf, int length) {
	int first_page_size;
	sg_entry_t *head, *tail;
	
	// fail-fast on bad parameters
	if (buf == NULL)
		return NULL;

// BH: should check length
if (length <= 0)
	return NULL;

	// page size of first chunk - so next will be aligned
	first_page_size = PAGE_SIZE - (ptr_to_phys(buf) % PAGE_SIZE);
	first_page_size = min(first_page_size, length);
	
	// create first chunk (not aligned)
	head = make_sg_entry(buf, first_page_size);
	length -= first_page_size;
	buf += first_page_size;
	
	// create middle chunks (aligned, size == PAGE_SIZE)
	tail = head;
	while (length >= PAGE_SIZE && tail != NULL) {
		tail->next = make_sg_entry(buf, PAGE_SIZE);
		tail = tail->next;
		length -= PAGE_SIZE;
		buf += PAGE_SIZE;
	}
	
	// create last chunk (aligned, size <= PAGE_SIZE)
	if (length > 0 && tail != NULL) {
		tail->next = make_sg_entry(buf, length);
		tail = tail->next;
	}
	
	// handle failed malloc() - when make_sg_entry() had returned NULL at some point
	if (tail == NULL) {
		// free allocated chunks (if any)
		if (head != NULL) sg_destroy(head);
		// returned failed status
		return NULL;
	}
	
	// tail is last chunk
	tail->next = NULL;
	
	return head;
}

void sg_destroy(sg_entry_t *sg_list) {
	sg_entry_t *to_be_freed;
	while (sg_list != NULL) {
		to_be_freed = sg_list;
		sg_list = sg_list->next;
		free(to_be_freed);
	}
}


int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) {
	int offset = 0;
	int src_chunk_offset, dest_chunk_offset, copied = 0;

// BH
if (count <= 0 || src_offset < 0) return 0;
	
	// src should point to chunk at correct offset 
	// [note src_offset may fall out scope of src sg list, so handle this]
	while (src != NULL && offset + src->count < src_offset) {
		offset += src->count;
		src = src->next;
	}
	// offset in current src chunk
	src_chunk_offset = src_offset - offset; 
	// offset in current dest chunk
	dest_chunk_offset = 0;
	
	
	// copy from src to dest
	// assuming all src and dest chunks can be of different sizes
	// this means that all copies may be misaligned from page boundaries.
	while (copied < count && dest != NULL && src != NULL) {
		// between two current chunks
		int bytes_to_copy = min(src->count - src_chunk_offset, 
								dest->count - dest_chunk_offset);
		bytes_to_copy = min(bytes_to_copy, count - copied);
		
		memcpy(phys_to_ptr(dest->paddr) + dest_chunk_offset,
			   phys_to_ptr(src->paddr) + src_chunk_offset,
			   bytes_to_copy);
		
		src_chunk_offset += bytes_to_copy;
		dest_chunk_offset += bytes_to_copy;
		copied += bytes_to_copy;
		
		if (src_chunk_offset == src->count) {
			src = src->next;
			src_chunk_offset = 0;
		}
		
		if (dest_chunk_offset == dest->count) {
			dest = dest->next;
			dest_chunk_offset = 0;
		}
	}
	
	return copied;
}

