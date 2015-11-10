
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sg_copy.h"

#define byte char

/**************************************************** Functions Deceleration ****************************************************/

/*
 * _allocate_sg_elem    Allocate element of a scatter-gather list
 *
 * @ret                Pointer to new sg_entry_t element or NULL in failure
 */
static sg_entry_t* _allocate_sg_elem(void);

/*************************************************** Functions Implementation ***************************************************/

sg_entry_t *sg_map(void *buf, int length) {

	unsigned int num_of_elem_in_list = 0, created_elem_counter = 0, i;
	sg_entry_t* ret_list = NULL, *current_elem = NULL;
	byte first_elem_size = 0;
	void* next_free_space = buf;

	// input validation
	if (!buf || (length <= 0)) {
		return NULL;
	}

	// calculate the list size
// BH: BUG
//	num_of_elem_in_list = length / PAGE_SIZE;
//	first_elem_size = length % PAGE_SIZE;
//	if (first_elem_size > 0) {
//		++num_of_elem_in_list;
//	}
first_elem_size = (unsigned long)buf % PAGE_SIZE;
if (first_elem_size) first_elem_size = PAGE_SIZE - first_elem_size;
num_of_elem_in_list = (length - first_elem_size + PAGE_SIZE - 1) / PAGE_SIZE;

	// create the first element in list
	ret_list = _allocate_sg_elem();
	if (!ret_list) {
		printf("Failed to allocate memory\n");
		return NULL;
	}

	created_elem_counter++;

	if (first_elem_size > 0) {
		ret_list->count = first_elem_size;
		ret_list->paddr = ptr_to_phys(next_free_space);
		next_free_space = ((byte*)next_free_space) + first_elem_size;
	} else {
		ret_list->count = PAGE_SIZE;
// BH: BUG: need to cap at length
if (ret_list->count > length) ret_list->count = length;
		ret_list->paddr = ptr_to_phys(next_free_space);
		next_free_space = ((byte*)next_free_space) + PAGE_SIZE;
	}

	current_elem = ret_list;

	// create the rest of elements in list
	for (i = created_elem_counter; i < num_of_elem_in_list; i++) {
// BH: setup length
length -= current_elem->count;
		current_elem->next = _allocate_sg_elem();
		current_elem = current_elem->next;
		current_elem->count = PAGE_SIZE;
// BH: BUG: need to cap at length
if (current_elem->count > length) current_elem->count = length;
		current_elem->paddr = ptr_to_phys(next_free_space);
		next_free_space = ((byte*)next_free_space) + PAGE_SIZE;
	}

	return ret_list;
}


void sg_destroy(sg_entry_t *sg_list) {
	sg_entry_t* current = sg_list, *next = NULL;

	if (sg_list) {
		while (current) {
			next = current->next;
			free(current);
			current = next;
		}
	}
}


int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) {
	int number_of_copied = 0, bytes_left_to_copy = 0, i;

	// input validation
	if ((!src) || (!dest) || (count <= 0) || (src_offset < 0)) {
		return number_of_copied;
	}

	// check if we don't exceed page size
// BH: WRONG
//	if (src_offset >= src->count) {
//		return number_of_copied;
//	}

while (src_offset >= src->count && src) {
	src_offset -= src->count;
	src = src->next;
}
if (!src)
	return 0;

	bytes_left_to_copy = src->count - src_offset;

	// calculates how many bytes left to be copied after the offset
	number_of_copied =  (count >= bytes_left_to_copy) ? bytes_left_to_copy : count;

	// check if we have enough space at destination page.
	number_of_copied = (number_of_copied > dest->count) ? dest->count : number_of_copied;

	// if we assuming that we don't copy the same page to each other (even with offset). remove the if 0
#if 0
	if (src == dest) {
		return number_of_copied;
	}
#endif
	for (i = 0; i < number_of_copied ; i++) {
		*((byte*)(phys_to_ptr(dest->paddr) + i)) = *( i + src_offset + (byte*)phys_to_ptr(src->paddr));
	}

	return number_of_copied;
}


static sg_entry_t* _allocate_sg_elem(void) {
	sg_entry_t* new_elem = NULL;

	new_elem = (sg_entry_t*)malloc(sizeof(sg_entry_t));
	if (new_elem) {
		memset(new_elem, 0, sizeof(sg_entry_t));
	}

	return new_elem;
}

/******************************************************* debug functions *******************************************************/

void buffer_dump(void* buf, int length) {
	unsigned int i;

	if ((!buf) || (length <= 0)) {
		printf("buffer is empty\n");
		return;
	}

	for (i = 0; i < length; i++) {
		printf("value %c at address %p\n", *(((byte*)buf) + i), buf + i);
	}

}

void sg_list_dump(sg_entry_t* list) {
	sg_entry_t* current = NULL;

	// input validation
	if (!list) {
		printf("sg list is empty\n");
		return;
	}

	current = list;
	while (current) {
		printf("=============================\n");
		printf("address = %p\n", phys_to_ptr(current->paddr));
		printf("page size = %d\n", current->count);
		buffer_dump(phys_to_ptr(current->paddr), current->count);
		current = current->next;
	}
}
