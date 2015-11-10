//BH: missing includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
sg_entry_t *sg_map(void *buf, int length)
{
// BH: NULL
    sg_entry_t **curr_enrtry, *scatter_buf = NULL;
    physaddr_t chunk_addr;
    int rest_len = length;
    char *rest_buf =  buf;
    curr_enrtry = &scatter_buf;
    while (rest_len > 0){
        *curr_enrtry= (sg_entry_t *)malloc(sizeof(sg_entry_t));
        (*curr_enrtry) -> next = 0;
        (*curr_enrtry)->paddr = ptr_to_phys(rest_buf);
        (*curr_enrtry)-> count =  (*curr_enrtry)->paddr & (PAGE_SIZE-1); 
        if(0 == (*curr_enrtry)-> count )
            (*curr_enrtry)-> count = PAGE_SIZE;
        rest_len -= (*curr_enrtry)-> count;
        rest_buf += (*curr_enrtry)-> count;
        curr_enrtry= &(*curr_enrtry)->next;
    }
    return scatter_buf;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list){
// BH: check inputs
if (!sg_list) return;
//    if(sg_list->next)
        sg_destroy(sg_list->next);
// BH: missing ';'
    free(sg_list);
}

/*
 * sg_buf_len       calcualte length of scatter-gather lists
 *
 * @in entry       Source sg list
 *
 * @ret           number of bytes in the buffer described by scatter-gather lists
 *
 *               The function returns the actual number of bytes copied
 */


int sg_buf_len(sg_entry_t *entry) {
    if(!entry) 
        return 0;
    return entry->count + sg_buf_len(entry->next);
// BH: missing closing brace
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count){
// BH: check inputs
if (src == NULL || dest == NULL || count <= 0) return 0;
    void* src_buf = phys_to_ptr(src->paddr + src_offset);
    void* dst_buf = phys_to_ptr(dest->paddr);

    //get src ptr and count
    int src_len = sg_buf_len(src) - src_offset;
    if(src_len <= 0) 
        return 0; // Nothing to copy
// BH: BUG src_len -> dst_len
    int dst_len = sg_buf_len(dest);
    if(dst_len <= 0) 
        return 0; // Nothing to copy
    int src_to_copy = src_len > count ? count: src_len;
    int dst_to_copy = dst_len > count ? count : dst_len;
    int copied = src_to_copy <  dst_to_copy ? src_to_copy : dst_to_copy;
    // Buffers may be overlapped - use memmove
    memmove(dst_buf, src_buf, copied);
    return copied;
}

