#include "sg_copy.h"
#include <stdlib.h>

int min3(int a, int b, int c)
{
  int min_ab = (a < b ? a : b);
  return (min_ab < c ? min_ab : c);
}

sg_entry_t *sg_map(void *buf, int length)
{
  // Since the only the first (head) element of the sg_map may contain
  // less then PAGE_SIZE, we will have to divide buf to chunks in reverse.
  // We want to maintain the order of buf as the order of elements
  
  // We do not allow chunks with zero length. They are treated as NULL
  
  // holds the length will still have to map in the buffer
  int length_left = length;
  // holds the current offset we are handling in buf
  // (reminder: we insert in reverse)
  void * current_buf = buf + length;
  // holds the current head of the map.
  sg_entry_t * sg_head = NULL;
  // count of current entry
  int entry_count = 0;
  // the new head we will add to the list
  sg_entry_t * new_sg_head = NULL;
  
  if (length < 0) {
    return NULL;
  }
  
  while (length_left > 0)
  {
    new_sg_head = (sg_entry_t *) malloc(sizeof(sg_entry_t));
    if (new_sg_head == NULL) {
      return NULL;
    }

    if (length_left >= PAGE_SIZE) {
      entry_count = PAGE_SIZE;
    } else {
      entry_count = length_left;
    }
    current_buf -= entry_count;
    // put data in new head
    new_sg_head->paddr = ptr_to_phys(current_buf);
    new_sg_head->count = entry_count;
    new_sg_head->next = sg_head;

    length_left -= entry_count;
    
    // set current head to be the new head
    sg_head = new_sg_head;
  }
  
  // note that for an empty buffer we will return NULL
  // (which is a valid sg)
  return sg_head;
}

void sg_destroy(sg_entry_t *sg_list)
{
  sg_entry_t * current_head = sg_list;
  sg_entry_t * next_head = NULL;
  
  while (current_head != NULL)
  {
    next_head = current_head->next;
    free(current_head);
    current_head = next_head;
  }
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
  // current entry we're on in the src list
  sg_entry_t * cur_src_entry = src;
  // current entry we're on in the dest list
  sg_entry_t * cur_dest_entry = dest;
  // used for finding the correct entry in the src list
  int src_offset_to_go = src_offset;
  // bytes we still have to copy
  int count_left = count;
  // offsets in the current entries we're copying from/to
  int offset_in_src_entry = 0;
  int offset_in_dest_entry = 0;
  // the current amount of bytes to copy in the iteration
  int bytes_to_copy = 0;
  
  if (src_offset < 0 || count < 0) {
    return -1;
  }
  
  while (cur_src_entry != NULL)
  {
    if (src_offset_to_go >= cur_src_entry->count) {
      src_offset_to_go -= cur_src_entry->count;
      cur_src_entry = cur_src_entry->next;
    } else {
      break;
    }
  }
  
  if (cur_src_entry == NULL)
  {
    return 0;
  }
  
  offset_in_src_entry = src_offset_to_go;
  while (count_left > 0 && cur_src_entry != NULL && cur_dest_entry != NULL)
  {
    bytes_to_copy =
      min3(count_left,
           cur_src_entry->count - offset_in_src_entry,
           cur_dest_entry->count - offset_in_dest_entry);
    // sanity check
    if (bytes_to_copy == 0) {
      return -1;
    }
    void * copy_from_ptr =
      phys_to_ptr(cur_src_entry->paddr) + offset_in_src_entry;
    void * copy_to_ptr =
      phys_to_ptr(cur_dest_entry->paddr) + offset_in_dest_entry;
    
    memcpy(copy_to_ptr, copy_from_ptr, bytes_to_copy);
    
    offset_in_src_entry += bytes_to_copy;
    offset_in_dest_entry += bytes_to_copy;
    count_left -= bytes_to_copy;
    
    if (offset_in_src_entry == cur_src_entry->count) {
      offset_in_src_entry = 0;
      cur_src_entry = cur_src_entry->next;
    }
    if (offset_in_dest_entry == cur_dest_entry->count) {
      offset_in_dest_entry = 0;
      cur_dest_entry = cur_dest_entry->next;
    }
  }
  
  return (count - count_left);
}
