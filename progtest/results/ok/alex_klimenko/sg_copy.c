// BH
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define min(a, b) (((a) <= (b)) ? (a) : (b))

#include "sg_copy.h"


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)  {
   sg_entry_t *cur_entry = NULL;

   while (sg_list != NULL) {
      cur_entry = sg_list;
      sg_list = cur_entry->next;
      free(cur_entry);
   }
}

/*
 * sg_check_len    Scan a scatter-gather list and calculate its total length.
 *
 * @in *sg_list  Pointer to a scatter-gather list 
 * @in max_len   limiting length of the buffer, represented by sg_list, if <0 the list will remain unchanged
 *                 if max_len > 0 release the rest of the list. so the resulting list to have the max_len length 
 *                 if max_len == 0 list will contain first and the only element of length == 0 
 *
 * @ret the resulting length of the buffer, represented by the list
 */
int sg_check_len(sg_entry_t *sg_list, int max_len)  {
   int result = 0, more_then_max = 0;
   sg_entry_t *cur_entry = sg_list, *rest_of_list = NULL;

   while (cur_entry != NULL) {
      if (max_len >= 0 && (more_then_max = result + cur_entry->count - max_len) >= 0) {
         cur_entry->count -= more_then_max;
         rest_of_list = cur_entry->next;
         cur_entry->next = NULL;
         sg_destroy(rest_of_list);
      }
      result += cur_entry->count;
      cur_entry = cur_entry->next;
   }
   return result;
}

/*
 * sg_get_cont_len    Scan a scatter-gather list and calculate the length of continuus part of the list.
 *
 * @in out **sg_list   A pointer to a scatter-gather list 
 *
 * @ret the resulting length of the continuus portion of the list
 * *sg_list - next element after continuus portion of the original list or NULL if list is exausted
 */
int sg_get_cont_len(sg_entry_t **sg_list)  {
   int result = 0;
   sg_entry_t *cur_entry = (sg_list == NULL ? NULL : *sg_list), *rest_of_list = NULL;
   char *prev_element_addr = (cur_entry == NULL ? NULL : (char *)phys_to_ptr(cur_entry->paddr)), *cur_element_addr = NULL;

   while (cur_entry != NULL && (cur_element_addr = (char *)phys_to_ptr(cur_entry->paddr)) == prev_element_addr) {
      result += cur_entry->count;
      prev_element_addr += cur_entry->count;
      cur_entry = cur_entry->next;
   }
   *sg_list = cur_entry;
   return result;
}

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
    sg_entry_t *result = NULL, *cur_entry = NULL, *prev_entry = NULL;
    char *cur_buf = (char *)buf;
    int cur_len = length, cur_page_len = PAGE_SIZE - ((unsigned long)cur_buf & (PAGE_SIZE - 1)); //len of first element to get page alingnment
    
// BH: check input
if (!buf || length <= 0) return NULL;

// BH: limit first page to length
if (cur_page_len > length) cur_page_len = length;

    if (cur_buf != NULL) {
       while (cur_len > 0) {
           if ((cur_entry = (sg_entry_t *)malloc(sizeof(sg_entry_t))) != NULL)  {
              cur_entry->next = NULL;
              if (result == NULL) {
                 result = cur_entry;
              } else {
                 prev_entry->next = cur_entry;
              }
              prev_entry = cur_entry;
              cur_entry->paddr = ptr_to_phys(cur_buf);
              cur_entry->count = cur_page_len;
              cur_buf += cur_page_len;
              cur_len -= cur_page_len;
              cur_page_len = (cur_len >= PAGE_SIZE ? PAGE_SIZE : cur_len);
           } else {
              sg_destroy(result);
              result = NULL;
              cur_len = 0;
           }

       }
    }
    return result;

}

/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list 
 *               (I assumed:
 *       1. the routine have to make a physical copy of a data;
 *       2. the dest list has already been created and refers to a previously allocated set of buffers
 *               or mapped physical memory regions;
 *       3. each of the lists may represent arbitrary number of continuus buffers (most general case)
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)  {
// BH: check input
if (!src || !dest || src_offset < 0 || count <= 0) return 0;

   int result = 0;
   char *src_buf = (char *)phys_to_ptr(src->paddr);
   char *dest_buf = (char *)phys_to_ptr(dest->paddr);
   sg_entry_t *cur_src = src, *next_src = src, *cur_dest = dest, *next_dest = dest;
   int   cur_src_copy_size = sg_get_cont_len(&next_src), cur_dest_copy_size = sg_get_cont_len(&next_dest);
   int   cur_copy_size = 0, rest_to_copy = count,  cur_offset = src_offset;

   // first, skip offset of the sorce buffer
   while (cur_src != NULL && (cur_offset -= cur_src_copy_size) >= 0) {
      cur_src = next_src;
      cur_src_copy_size = sg_get_cont_len(&next_src);
   }

   if (cur_offset < 0) {  // if cur_offset >= 0 - nothing to copy, offset >= total src list length
      cur_offset += cur_src_copy_size;           //set up
      cur_src_copy_size -= cur_offset;              // parameters
      src_buf = (char *)phys_to_ptr(cur_src->paddr) + cur_offset;        // for main copy cycle
      // main copy cycle, which performs copy of minimal possible continuus portion of data
      while (cur_src != NULL && cur_dest != NULL && rest_to_copy > 0) {
         cur_copy_size = min(rest_to_copy, cur_src_copy_size);
         cur_copy_size = min(cur_copy_size, cur_dest_copy_size);
         if (cur_copy_size > 0) {
            memcpy(dest_buf, src_buf, cur_copy_size); // copy continuus portion (possible multi page)
            result += cur_copy_size;   
            if ((rest_to_copy -= cur_copy_size) > 0)  { // if all data copied - skip unnecessary switching
               if ((cur_src_copy_size -= cur_copy_size) <= 0) { // if source continuus portion exausted
                  if ((cur_src = next_src) == NULL) break;
                  src_buf = (char *)phys_to_ptr(cur_src->paddr);
                  cur_src_copy_size = sg_get_cont_len(&next_src);
			   } else {
				   src_buf += cur_copy_size;
			   }
               if ((cur_dest_copy_size -= cur_copy_size) <= 0) { // if dest continuus portion exausted
                  if ((cur_dest = next_dest) == NULL) break;
                  dest_buf = (char *)phys_to_ptr(cur_dest->paddr);
                  cur_dest_copy_size = sg_get_cont_len(&next_dest);
			   } else {
				   dest_buf += cur_copy_size;
			   }
            }
         } else {
            break; // if cur_copy_size == 0 - error offset too long
         }
      }
   }
   // copy finished -  fix dest list to remove unnecessary data 
   sg_check_len(cur_dest, cur_copy_size);
   return result;
}

