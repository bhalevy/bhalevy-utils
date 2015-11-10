#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

sg_entry_t *sg_map(void *buf, int length)
{
    byte * buffer = (byte *)buf;
    sg_entry_t * list = NULL;
    sg_entry_t ** curr = &list;
    while (length > 0) {
        *curr = malloc(sizeof(sg_entry_t));
        if (*curr == NULL) {
            fprintf(stderr, "failed to allocate new entry");
            return NULL;
        }
        (*curr)->paddr = ptr_to_phys(buffer);
// BH: BUG alignment
//        (*curr)->count = MIN(length, PAGE_SIZE);
        (*curr)->count = PAGE_SIZE - ((unsigned long)buffer & (PAGE_SIZE-1));
	if ((*curr)->count > length)
		(*curr)->count = length;
        (*curr)->next = NULL;
        buffer += (*curr)->count;
        length -= (*curr)->count;
        curr = &((*curr)->next);
    }

    return list;
}

void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t * next;
    while(sg_list) {
        next = sg_list->next;
        free(sg_list);
        sg_list = next;
    }
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    int bytes_copied = 0;
    int size;
    int dst_offset = 0;
    byte * srcbuff, * dstbuff;

    /* find source offset */
// BH: first page alignment
//    while(src && src_offset >= PAGE_SIZE) {
    while(src && src_offset >= src->count) {
        src_offset -= src->count;
        src = src->next;
    }

    /* copy */
    while (src && dest && count > 0) {

        size = MIN(PAGE_SIZE, count);
        size = MIN(size, (src->count - src_offset));
        size = MIN(size, (dest->count - dst_offset));

        srcbuff = (byte *)phys_to_ptr(src->paddr) + src_offset;
        dstbuff = (byte *)phys_to_ptr(dest->paddr) + dst_offset;
        memcpy(dstbuff, srcbuff, size);

        bytes_copied += size;
        count -= size;

// BH: BUG
//        dst_offset = (dst_offset + size) % PAGE_SIZE;
        dst_offset += size;
//        if (dst_offset == 0) {
        if (dst_offset >= dest->count) {
            dest = dest->next;
            dst_offset = 0;	// BH
        }

// BH: BUG
//        src_offset = (src_offset + size) % PAGE_SIZE;
        src_offset += size;
//        if (src_offset == 0) {
        if (src_offset >= src->count) {
            src = src->next;
            src_offset = 0;	// BH
        }
    }

    return bytes_copied;
}

/*  for testing... */

void print_buf(const byte * buff, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (i % 8 == 0 && i % 16) {
            printf("   ");
        }
        if (i % 16 == 0) {
            printf("\n%04x  ", i);
        }
        printf("%02X ", buff[i]);
    }
    printf("\n");
}

void print_list(const sg_entry_t * list)
{
    byte * buffer;
    printf("List: \n");
    while(list) {
        printf("Entry: [0x%lx, %d]\n", list->paddr, list->count);
        printf("Data: ");
        buffer = (byte *)phys_to_ptr(list->paddr);
        print_buf(buffer, list->count);
        list = list->next;
    }

    printf("\n");
}

