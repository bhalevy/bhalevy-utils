// BH: required system includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

// BH: required MIN
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

sg_entry_t *sg_map(void *buf, int length)
{
    char* curr_ptr = buf; //the length is in bytes so I am using char*
    int remain_count = length;
    sg_entry_t *first_sg = NULL;
    sg_entry_t *cur_sg = NULL;
    sg_entry_t *prev_sg = NULL;
    
    while(remain_count)
    {
        int offset_to_end_page = PAGE_SIZE - ((unsigned long)curr_ptr & (PAGE_SIZE - 1)); //the offset from buf until the end of the page (for the first sg elemnt)
        int cur_length = MIN(remain_count,offset_to_end_page);
        cur_sg = (sg_entry_t *) malloc(sizeof(sg_entry_t));
        if(!cur_sg) // allocation failed
        {
            sg_destroy(first_sg); //my implemantaion of sg_destroy can handle NULL
            return NULL;
        }        
        
        cur_sg->paddr = ptr_to_phys(curr_ptr);
        cur_sg->count = cur_length;
        cur_sg->next = NULL;
        curr_ptr += cur_length; //update the current pointer
        remain_count -= cur_length; //update the remain count
        
        if(!first_sg) //save the first element
        {
            first_sg = cur_sg;            
        }
        else
        {
            prev_sg->next = cur_sg; //update the next of the previous elemnt            
        }
        prev_sg = cur_sg;
    }
    return first_sg;
}

void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t *cur_sg = sg_list;
    sg_entry_t *next_sg = NULL;
    while(cur_sg)
    {
        next_sg = cur_sg->next;
        free(cur_sg);
        cur_sg = next_sg;
    }    
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    sg_entry_t *curr_src = src;
    sg_entry_t *curr_dest = dest;
    int remain_offset = src_offset;
    int remain_count = count;
    int offset_in_dest = 0;
    int offset_in_src = 0;
    int total_copied = 0;
    
    while(curr_src && curr_dest && remain_count)
    {
        int bytes_to_copy = 0;
    
        if(curr_src->count <= remain_offset)
        {
            remain_offset -= curr_src->count;
            curr_src = curr_src->next;
            continue;
        }
        
        if(remain_offset) // && curr_src->count > remain_offset
        {
            offset_in_src = remain_offset;
            remain_offset = 0;
            continue;
        }
        
        bytes_to_copy = MIN(curr_src->count - offset_in_src, curr_dest->count - offset_in_dest);
        bytes_to_copy = MIN(bytes_to_copy, remain_count);
        memcpy(phys_to_ptr(curr_dest->paddr) + offset_in_dest,
               phys_to_ptr(curr_src->paddr) + offset_in_src,
               bytes_to_copy);        
        total_copied += bytes_to_copy;
        remain_count -= bytes_to_copy;
        
        if((offset_in_src + bytes_to_copy) < curr_src->count)
        {
            offset_in_src += bytes_to_copy;
        }
        else
        {
            curr_src = curr_src->next;
            offset_in_src = 0;
        }
        
        if((offset_in_dest + bytes_to_copy) < curr_dest->count)
        {
            offset_in_dest += bytes_to_copy;
        }
        else
        {
            curr_dest = curr_dest->next;
            offset_in_dest = 0;
        }
    }
    
    return total_copied;
}

