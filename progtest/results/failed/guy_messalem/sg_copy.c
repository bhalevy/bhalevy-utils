#include <stdio.h>
#include "sg_copy.h"

// BH: missing includes
#include <stdlib.h>


/************************************/

sg_entry_t *sg_map(void *buf, int length) {

    physaddr_t paddr = 0;
    sg_entry_t *sg_list = NULL;

    if (buf && length>0) {
        paddr = ptr_to_phys(buf);
        sg_list = (sg_entry_t *) malloc(sizeof(sg_entry_t));
        sg_list->paddr = paddr;
//        int alignment_delta = ((paddr+PAGE_SIZE)&~(PAGE_SIZE-1)) - paddr;
        int alignment_delta = (((unsigned long)buf+PAGE_SIZE)&~(PAGE_SIZE-1)) - (unsigned long)buf;
        int count = length>=alignment_delta ? alignment_delta : length;
        sg_list->count = count;
        sg_list->next = NULL;
        length -= count;
// BH WRONG
//	paddr += count;
buf += count;

        sg_entry_t *sg_list_end = sg_list;

        while(length) {
            sg_entry_t *tmp_entry = (sg_entry_t *) malloc(sizeof(sg_entry_t));
            tmp_entry->paddr = ptr_to_phys(buf);
            int count = length>PAGE_SIZE ? PAGE_SIZE : length;
            tmp_entry->count = count;
            tmp_entry->next = NULL;
            length -= count;
// BH: WRONG
//	    paddr += count;
buf += count;
            sg_list_end->next = tmp_entry;
            sg_list_end = sg_list_end->next;
        }
        return sg_list;
    } else {
        return NULL;
    }
}

/************************************/

void sg_destroy(sg_entry_t *sg_list) {
    while(sg_list) {
        sg_entry_t *tmp_entry = sg_list;
        sg_list = sg_list->next;
        free(tmp_entry);
    }
}

/************************************/

// BH: changed the signature
//int sg_copy(sg_entry_t *src, sg_entry_t **dest, int src_offset, int count) {
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) {
    int bytes_copied = 0;

    if (count) {
        sg_entry_t *my_dest = NULL;
        //find offset within src
        while (src && src_offset >= src->count) {
            src_offset -= src->count;
            src = src->next;
        }
        if (src) {
            //copy 1st entry
            my_dest = (sg_entry_t *) malloc(sizeof(sg_entry_t));
            my_dest->paddr = src->paddr + src_offset;
            int tmp_count = (count >= src->count - src_offset) ?
                            src->count - src_offset : count;
            my_dest->count = tmp_count;
            my_dest->next = NULL;
            bytes_copied += tmp_count;
            count -= tmp_count;
            src = src->next;
            sg_entry_t *dest_end = my_dest;
            //copy the rest of the list
            while(src && count) {
                sg_entry_t *tmp_entry = (sg_entry_t *) malloc(sizeof(sg_entry_t));
                tmp_entry->paddr = src->paddr;
                int tmp_count = count >= src->count ? src->count : count;
                tmp_entry->count = tmp_count;
                tmp_entry->next = NULL;
                dest_end->next = tmp_entry;
                dest_end = dest_end->next;
                bytes_copied += tmp_count;
                count -= tmp_count;
                src = src->next;
            }
            *dest = my_dest;
        }
    }
    return bytes_copied;

}
