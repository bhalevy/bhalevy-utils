/* 
 * Andrey: Error Handling
 *	Since error handling not defined it's marked as
 *		/ * ERROR: < error reason> * /
 * 	The intention is to keep data structures as consistent as possible    
 */
#include <stdlib.h>
#include <string.h>

#include "sg_copy.h"

/*
 * Helper function for sg_map
 *
 * @note	count should be less or equal to count
 */
static void sg_entry_create(sg_entry_t **entry, void *buf, int count, int *reminder)
{
	sg_entry_t *p;

	p = calloc(1, sizeof(sg_entry_t));
	*entry = p;
	if (!p)	
		return; /* ERROR: memory allocation failure */
		
	p->paddr = ptr_to_phys(buf);
	p->count = (*reminder < count) ? *reminder : count;
	*reminder -= p->count;
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
	sg_entry_t *cur_entry, *ret_entry;
	int length_reminder;
	unsigned long first_buf, cur_buf;
	
	if (!buf)
		return NULL; /* ERROR: invalid input */
		
	if (length < 1)
		return NULL;

	length_reminder = length;
	
	first_buf = (unsigned long)buf;
#if ((PAGE_SIZE & (PAGE_SIZE - 1)) == 0)
	/* PAGE_SIZE is a power of 2 */
	cur_buf = ((first_buf & ~ (PAGE_SIZE -1)) + PAGE_SIZE);
#else
	cur_buf = first_buf + PAGE_SIZE - (first_buf % PAGE_SIZE);
#endif	
	sg_entry_create(&ret_entry, buf, cur_buf - first_buf, &length_reminder);
	if (!ret_entry)
		return NULL;	/* ERROR: memory allocation failure */
	
	cur_entry = ret_entry;
	while (length_reminder > 0)
	{
		sg_entry_create(&(cur_entry->next), (void*)cur_buf, PAGE_SIZE, &length_reminder);
		if (!cur_entry->next)
		{
			/* ERROR: memory allocation failure */
			sg_destroy(ret_entry);
			return NULL;
		}
		
		cur_entry = cur_entry->next;
		cur_buf += PAGE_SIZE;		
	}

	return ret_entry;
}

/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *cur, *next;
	
	for (cur = sg_list; cur; cur = next)
	{
		next = cur->next;
		free(cur);
	}
}

/*
 * Helper data structure for sg_copy
 */
struct sg_ptr_entry_s {
	void *buf;               /* physical address */
	int count;               /* number of bytes: */
	sg_entry_t *next;
};

typedef struct sg_ptr_entry_s sg_ptr_entry_t;

/* 
 * helper fiction for  sg_copy
 */
static sg_ptr_entry_t *sg_ptr_entry2ptr(sg_entry_t *entry, sg_ptr_entry_t *p)
{
	if (!entry)
		return NULL;
		
	if (entry->count < 1)
		return sg_ptr_entry2ptr(entry->next, p);

	p->buf = phys_to_ptr(entry->paddr);	
	if (!p->buf)
		return NULL; /* ERROR: data structure inconstant  */
		
	p->count = entry->count;
	p->next = entry->next;
	return p;
}

/*
 * helper fiction for  sg_copy
 *
 * @note	p should not be null;
 *		cp_size should be grater or equal to 0, 
 *		should be less or equal than p->count
 */
static sg_ptr_entry_t *sg_ptr_update_entry(sg_ptr_entry_t *p, int cp_size)
{
	p->count -= cp_size;
	if (p->count > 0)
	{
		p->buf += cp_size;
		return p;
	}
	else
		return sg_ptr_entry2ptr(p->next, p);
}

/*
 * helper fiction for  sg_copy
 *
 * @note:	count should be positive
 */
static int sg_ptr_copy(sg_ptr_entry_t *src, sg_ptr_entry_t *dest, int count)
{
	sg_ptr_entry_t *cur_src, *cur_dest;
	int reminder;
	
	cur_src = src;
	cur_dest = dest;
	reminder = count;
	while (reminder > 0 && cur_src  && cur_dest)
	{	
		int cp_size;
		
		cp_size = (cur_src->count > reminder) ? reminder : cur_src->count;
		cp_size = (cp_size > cur_dest->count) ? cur_dest->count : cp_size;
		memcpy(cur_dest->buf, cur_src->buf, cp_size);
		reminder -= cp_size;
		cur_src = sg_ptr_update_entry(cur_src, cp_size);
		cur_dest = sg_ptr_update_entry(cur_dest, cp_size);
	}
	
	return count - reminder;
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
	sg_entry_t *cur;
	int reminder;
	sg_ptr_entry_t src_ptr, dest_ptr;
	sg_ptr_entry_t *src_ptr_p;
	
	if (!src || !dest || src_offset < 0)
		return 0;	/* ERROR: invalid input */

	if (count < 1)
		return 0;
		
	cur = src;
	reminder = src_offset;
	while(cur && cur->count < reminder)
	{
		reminder -= cur->count;
		cur = cur->next;
	}
	
	if (!cur)
		return 0;
	
	src_ptr_p = sg_ptr_entry2ptr(cur, &src_ptr);
	if (!src_ptr_p)
		return 0;
	
	return sg_ptr_copy(sg_ptr_update_entry(src_ptr_p, reminder),
			sg_ptr_entry2ptr(dest, &dest_ptr), count);
}

