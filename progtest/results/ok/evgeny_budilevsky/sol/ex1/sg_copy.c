#include "sg_copy.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

sg_entry_t *sg_map(void *buf, int length)
{
  sg_entry_t *sg_list_root = NULL;
  sg_entry_t *sg_prev = NULL;
  sg_entry_t *sg_next = NULL;

  while (length > 0) {
    sg_next = (sg_entry_t *)malloc(sizeof(sg_entry_t));
    if (sg_next == NULL) {
      goto Fail;
    }

    sg_next->paddr = ptr_to_phys(buf);
    /* All entries except the first one must be aligned on a
       PAGE_SIZE address */
    sg_next->count = min(length, PAGE_SIZE - (((unsigned long)buf) % PAGE_SIZE));
    sg_next->next = NULL;
    
    if (sg_prev != NULL) {
      sg_prev->next = sg_next;
    }
    else {
      sg_list_root = sg_next;
    }

    length -= sg_next->count;
    buf = ((char*)buf) + sg_next->count;

    sg_prev = sg_next;
  }

  goto Exit;
    
Fail:
  sg_destroy(sg_list_root);

Exit:
  return sg_list_root;
}

void sg_destroy(sg_entry_t *sg_list)
{
  sg_entry_t *sg_prev = NULL;

  while (sg_list != NULL) {
    sg_prev = sg_list;
    sg_list = sg_list->next;
    free(sg_prev);
  }
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
  int bytes_copied = 0;
  int copy_size = 0;
  int src_pos = 0;
  int dest_pos = 0;

  while (src != NULL && dest != NULL && bytes_copied < count) {
    if (src_pos == src->count) {
      src_pos = 0;
      src = src->next;
      continue;
    }

    if (dest_pos == dest->count) {
      dest_pos = 0;
      dest = dest->next;
      continue;
    }

    if (src_offset > 0) {
      copy_size = min(src->count, src_offset);
      src_pos += copy_size;
      src_offset -= copy_size;
      continue;
    }

    copy_size = min(src->count - src_pos, dest->count - dest_pos);
    copy_size = min(copy_size, count);
    memcpy(((char*)phys_to_ptr(dest->paddr))+ dest_pos,
           ((char*)phys_to_ptr(src->paddr)) + src_pos,
           copy_size);
    src_pos += copy_size;
    dest_pos += copy_size;
    bytes_copied += copy_size;
  }
      
  return bytes_copied;
}


