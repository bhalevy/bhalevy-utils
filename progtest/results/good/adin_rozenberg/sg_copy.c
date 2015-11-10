#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

#define PTR_SUM(p, n) ((void *)((char *)(p) + n))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

sg_entry_t *sg_map(void *buf, int length)
{
    sg_entry_t *head = NULL, *prev_entry, *entry;
    int first = 1;
    
    while (length > 0) {
        entry = malloc(sizeof(sg_entry_t));
        if (entry == NULL) {
            sg_destroy(head);
            return NULL;
        }
        entry->paddr = ptr_to_phys(buf);
        /*
         * entry->paddr is guaranteed to be page-aligned starting from second entry,
         * since virtual and physical addresses have same alignment
         */
        if (first) {
            entry->count = MIN(length, PAGE_SIZE - entry->paddr % PAGE_SIZE);
            head = entry; /* remember first chunk's address (and eventually return it) */
            first = 0;
        } else {
            entry->count = MIN(length, PAGE_SIZE);
            prev_entry->next = entry;
        }
        entry->next = NULL;
        buf = PTR_SUM(buf, entry->count);
        length -= entry->count;
        prev_entry = entry;
    }
    return head;
}

void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t *entry = sg_list, *next_entry;
    while (entry != NULL) {
        next_entry = entry->next;
        free(entry);
        entry = next_entry;
    }
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    int copied = 0, to_copy, dest_offset = 0;
// BH
if (count <= 0 || src_offset < 0) return 0;

    /* advance src_offset bytes in src sg_list */
    while (src != NULL && src_offset >= src->count) {
        src_offset -= src->count;
        src = src->next;
    }
    while (src != NULL && dest != NULL && count > 0) {
        /* copy the minimum of: space left in src chunk, space left in dest chunk, and bytes remaining */
        to_copy = MIN(src->count - src_offset, dest->count - dest_offset);
        to_copy = MIN(to_copy, count);
        memcpy(phys_to_ptr(dest->paddr + dest_offset), phys_to_ptr(src->paddr + src_offset), to_copy);
        if (dest_offset + to_copy < dest->count) {
            /* still some room left in dest chunk */
            dest_offset += to_copy;
        } else {
            /* we've consumed the entire chunk, move to next one */
            dest_offset = 0;
            dest = dest->next;
        }
        if (src_offset + to_copy < src->count) {
            /* still some data left in src chunk */
            src_offset += to_copy;
        } else {
            /* we've consumed the entire chunk, move to next one */
            src_offset = 0;
            src = src->next;
        }
        count -= to_copy;
        copied += to_copy;
    }
    return copied;
}

