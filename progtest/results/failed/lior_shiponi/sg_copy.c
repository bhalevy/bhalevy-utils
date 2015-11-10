#include "sg_copy.h"

// BH: mine
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GFP_KERNEL 0

static inline void *
kmalloc(size_t sz, int unused)\
{
	return malloc(sz);
}

#define kfree free

#define min(a, b) ((a) >= (b) ? (a) : (b))
// BH: end of mine

#define PAGE_SHIFT 5

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
	sg_entry_t *entry, *prev;
	physaddr_t paddr;
	int count, offset, page;

// BH boundaries check
if (length <= 0 || buf == NULL) return NULL;

	/* handle first possible partial page */
	paddr = ptr_to_phys(buf);
	offset = paddr & ~(PAGE_SIZE - 1);
	count = min(PAGE_SIZE - offset, length);
	entry = (sg_entry_t *)kmalloc(sizeof(sg_entry_t), GFP_KERNEL);
	entry->paddr = paddr;
	entry->count = count;
	length -= count;
	
	page = paddr >> PAGE_SHIFT;
	prev = entry;

	while(length > 0) {
		entry = (sg_entry_t *)kmalloc(sizeof(sg_entry_t), GFP_KERNEL);
		entry->count = min(PAGE_SIZE, length);
		entry->paddr = ++page << PAGE_SHIFT;
		length -= entry->count;
		prev->next = entry;
	}

	entry->next = NULL;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *entry, *next;
	entry = sg_list;

#if 0
	while(entry->next != NULL) {
		kfree(entry);
// BH: BUG: can't deref entry after freeing it
		entry = entry->next;
	}

	kfree(entry); /* remove the last one */
#else
for (; entry; entry = next) {
	next = entry->next;
	free(entry);
}
#endif
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
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) 
{
	sg_entry_t *src_entry, *dest_entry;
	int src_entry_offset, dest_entry_offset;
	int left = count;

// BH: boundaries check
if (!src || !dest || count <= 0) return 0;

	src_entry = src;
	dest_entry = dest;

	while(src_offset > 0) {
		if (src_offset >= src_entry->count) {
			src_offset -= src_entry->count;
			src_entry = src_entry->next;
			src_entry_offset = 0;
		}
		else {
			src_entry_offset = src_entry->count - src_offset;
			src_offset = 0;
		}
	}
	
	while(left > 0) {
		int max_copy = min(src_entry->count - src_offset, dest_entry->count - dest_entry_offset);

		memcpy(phys_to_ptr(dest_entry->paddr + dest_entry_offset), 
			   phys_to_ptr(src_entry->paddr + src_entry_offset), max_copy);

		dest_entry_offset += max_copy;
		src_entry_offset += max_copy;

		left -= max_copy;

		if (dest_entry_offset == dest_entry->count) {
			if (dest_entry->next == NULL) {
				return(count - left);
			}
			dest_entry = dest_entry->next;
			dest_entry_offset = 0;
		}
		
		if (src_entry_offset == src_entry->count) {
			if (src_entry->next == NULL) {
				return(count - left);
			}
			src_entry = src_entry->next;
			src_entry_offset = 0;
		}

	}

	return(count);

}

