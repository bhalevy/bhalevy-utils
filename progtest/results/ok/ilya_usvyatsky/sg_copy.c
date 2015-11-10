#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

/*
 * page_of      Computes page address of an address
 *
 * @in addr     An address
 */

static inline void *page_of(void *addr)
{
    return (void *)(((unsigned long)addr) & ~(PAGE_SIZE - 1));
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
    sg_entry_t  *head = NULL, *next = NULL;
    void        *cursor;
/* BH */
void *start = page_of(buf);

    /*
     * First, a trivial case:
     * zero or negative-length buffers cannot be mapped
     */
    if (length <= 0) {
        return NULL;
    }

    /*
     * Now, go backwards from the end of the buffer
     */
    for (cursor = buf + length; cursor > buf; cursor -= head->count) {
        head = malloc(sizeof(sg_entry_t));
        if (head == NULL) {
            /* No memory? */
            sg_destroy(next);
            return NULL;
        }
        /* BH: bug, what if buf is not page aligned? */
// BH        if ((cursor - buf) > PAGE_SIZE) {
        if ((cursor - start) > PAGE_SIZE) {
            /*
             * Common case: all pages but the very first
             */
            /* 
             * Here we subtract 1 to take care of cursor at a page boundary 
             */
            head->paddr = ptr_to_phys(page_of(cursor - 1));
            /* 
             * Most of the entries will be of PAGE_SIZE length,
             * but we do not know that, so compute it anyway
             */
            head->count = cursor - phys_to_ptr(head->paddr);
		/* BH: inefficient */
        }
        else {
            /*
             * Special case: very first (possibly unaligned) page
             */
            head->paddr = ptr_to_phys(buf);
            if (next) {
                head->count = phys_to_ptr(next->paddr) - buf;
            }
            else {
                head->count = length;
            }
        }
        head->next  = next;
        next        = head;
    }
    return head;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t *cursor = sg_list, *next = NULL;

    for (; cursor != NULL; cursor = next) {
        next = cursor->next;
        free(cursor);
    }
}

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

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
int sg_copy(sg_entry_t *src, sg_entry_t *dst, int src_offset, int count)
{
    int   dst_offset = 0, total_bytes_copied = 0, bytes_to_copy = 0;

    /*
     * Find start of the source
     */
    for (; src && (src_offset > src->count); 
            src_offset -= src->count, src = src->next) {
        /* Do nothing */
    }

    /*
     * Now copy as many bytes as possible at a time
     */
    while (count && dst && src) {
        if ((src->count - src_offset) < (dst->count - dst_offset)) {
            /*
             * Copy everything we have in the source, up to a count
             */
            bytes_to_copy = MIN(count, src->count - src_offset);
        }
        else {
            /*
             * Fill all the space we have in the destination up to a count
             */
            bytes_to_copy = MIN(count, dst->count - dst_offset);
        }
        /*
         * Do the copy
         */
        memcpy(phys_to_ptr(dst->paddr) + dst_offset,
                phys_to_ptr(src->paddr) + src_offset,
                bytes_to_copy);
        /*
         * Update cursors and counters
         */
        count -= bytes_to_copy;
        src_offset += bytes_to_copy;
        dst_offset += bytes_to_copy;
        total_bytes_copied += bytes_to_copy;
        /*
         * Advance cursors if we reached the end of an entry
         * in the source or in the destination (or both).
         */
        if (src->count == src_offset) {
            src = src->next;
            src_offset = 0;
        }
        if (dst->count == dst_offset) {
            dst = dst->next;
            dst_offset = 0;
        }
    }

    return total_bytes_copied;
}

#if 0
/*
 * sg_print     Print a scatter-gather list
 *
 * @in sg_list  A scatter-gather list
 */
static void sg_print(sg_entry_t *sg_list)
{
    if (sg_list == NULL) {
        printf("Empty list\n");
        return;
    }
    for (; sg_list; sg_list = sg_list->next) {
        printf("%p (0x%x) %5d (0x%x)\n", 
                phys_to_ptr(sg_list->paddr), sg_list->paddr,
                sg_list->count, sg_list->count);
    }
}

int main(int argc, char *argv[])
{
    void        *buf;
    int          length;
    sg_entry_t  *sg_list;

    if (argc != 3) {
        printf("Usage: %s buf length\n", argv[0]);
        return 1;
    }
    buf    = (void *)strtoul(argv[1], NULL, 16);
    length = strtol(argv[2], NULL, 10);

    printf("Mapping buffer %p, length %d\n", buf, length);

    sg_list = sg_map(buf, length);

    sg_print(sg_list);

    sg_destroy(sg_list);

    return 0;
}
#endif
