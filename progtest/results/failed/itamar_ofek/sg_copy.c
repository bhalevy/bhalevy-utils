// BH: missing includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "sg_copy.h"

/**
 * @brief Map a memory buffer using a scatter-gather list
 *
 * @param  buf   input  Pointer to buffer
 * @param  length input   Buffer length in bytes
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
    if (length <= 0)
    {
        return (sg_entry_t*)0;
    }
    size_t page_count = length/PAGE_SIZE;
    size_t offset = length % PAGE_SIZE;
    if (offset)
    {
        ++page_count;
    }
    else
    {
        offset = PAGE_SIZE;
    }
    sg_entry_t* nodes[page_count];
    nodes[0] = malloc(sizeof(sg_entry_t));
    size_t entry_index = 0;
    for (;entry_index != page_count -1;)
    {
        nodes[entry_index]->next = malloc(sizeof(sg_entry_t));
        sg_entry_t* node = nodes[entry_index];
        nodes[++entry_index] = node->next;
    }
    nodes[entry_index]->next = 0;

    for (entry_index = 0 ; entry_index != page_count ;++entry_index)
    {
        nodes[entry_index]-> paddr = ptr_to_phys (buf);
// BH	buf = buf + offset;
// BH        nodes[entry_index]-> count = offset;
        nodes[entry_index]->count = length - ((unsigned long)buf & (PAGE_SIZE-1));
	if (nodes[entry_index]->count > PAGE_SIZE)
		nodes[entry_index]->count = PAGE_SIZE;
        offset = PAGE_SIZE;
	buf += nodes[entry_index]->count;
	length -= nodes[entry_index]->count;
    }
    return nodes[0];
}


void free_entry (sg_entry_t* entry)
{
    if (entry == 0)
    {
        return;
    }
// BH: BUG    bzero((void*)entry->paddr,entry->count);
    entry->count = 0;
    free(entry);
}

void sg_destroy(sg_entry_t *sg_list)
{
    if (sg_list == 0)
    {
        return;
    }
    sg_entry_t  **entry = &sg_list;
    sg_entry_t  *p= *entry;
    while (free_entry(p),*entry !=0)
    {
        entry = &(*entry)->next;
        p= *entry;

    } 
}



int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    int copied = 0;
    int entry_count = 0, dest_count = 0;
    sg_entry_t cached_dest = {0,0,0};
    for (; src && dest && count;)
    {
        if (src_offset > src ->count)
        {
            src_offset -= src ->count;
            src = src->next;
        }
        else
        {
            entry_count = src->count - src_offset;
            if (dest->count <= entry_count)
            {
// BH                memcpy((void*)dest->paddr,(void*)src->paddr+src_offset,dest->count);
                memcpy(phys_to_ptr(dest->paddr),phys_to_ptr(src->paddr)+src_offset,dest->count);
                src_offset +=dest->count;
                count -= dest->count;
                copied +=dest->count;
                dest = dest->next;
            }
            else /* dest->count > entry_count */
            {
// BH                memcpy((void*)dest->paddr,(void*)src->paddr + src_offset,entry_count);
                memcpy(phys_to_ptr(dest->paddr),phys_to_ptr(src->paddr)+src_offset,entry_count);
                cached_dest.paddr = dest->paddr + entry_count;
                cached_dest.count = dest->count - entry_count;
                cached_dest.next = dest->next;
                src_offset = 0;
                src =src->next;
                dest = &cached_dest;
                copied += entry_count;
                count -=entry_count;
            }
        }
    }
    return copied;
}

