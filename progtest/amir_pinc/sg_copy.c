#include <stdlib.h>
#include "sg_copy.h"

// BH missing typedefs
typedef unsigned char byte_t;

/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elements mapping the input buffer
 * 				 On failure or empty buffer, will return NULL
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */
sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *head = NULL;
	sg_entry_t *tail = NULL;
	int			bufferOffset = 0;

	/* Variables check */
	if ((length <= 0) || (buf == NULL))
		return NULL;	/* Invalid buffer size\pointer \ Empty buffer (TODO: trace) */

	/* Allocate list head */
	if ((head = (sg_entry_t*)malloc(sizeof(sg_entry_t))) == NULL)
		return NULL;	/* Failed (TODO: trace) */

	/* Set list first node */
	head->paddr = ptr_to_phys(buf);
	head->count = PAGE_SIZE - (((unsigned long)buf) & (PAGE_SIZE - 1));	/* Better performance than (buf % PAGE_SIZE), but can only work since PAGE_SIZE is power of 2 */

// BH: style: tabs vs. spaces
    /* Do we need more blocks? */
    if (head->count >= length)
    {
        head->count = length;
        head->next = NULL;
    }
    else   /* Yes, we do need more blocks */
    {
        bufferOffset = head->count;	/* Update new buffer offset */

        /* Continue with list nodes creation */
        tail = head;
        while (bufferOffset < length)
        {
            /* Allocate next node */
            if ((tail->next = (sg_entry_t*)malloc(sizeof(sg_entry_t))) == NULL)
            {
                sg_destroy(head);	/* Failed, free resources */
                return NULL;		/* Failed (TODO: trace) */
            }
            tail = tail->next;

            tail->paddr = ptr_to_phys(&(((byte_t*)buf)[bufferOffset]));
            tail->count = PAGE_SIZE;
            bufferOffset += PAGE_SIZE;
        }

        /* Update last node */
        tail->next = NULL;
        tail->count = PAGE_SIZE - (bufferOffset - length);
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
	sg_entry_t *temp;

	/* Free the list */
	while (sg_list != NULL)
	{
		temp = sg_list;
		sg_list = sg_list->next;
		free(temp);
	}
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
	byte_t *srcBuffer;
	byte_t *destBuffer;
	int		dest_offset;
	int		originalCount = count;

	/* Variables check */
	if ((src_offset < 0) || (count <= 0) || (src == NULL) || (dest == NULL))
		return 0;	/* Invalid attribute \ Nothing to copy (TODO: trace) */

	/* Jump to the src offset */
	while (src->count <= src_offset)
	{
		src_offset -= src->count;
		src = src->next;
		if (src == NULL)
			return 0;	/* Nothing to copy (TODO: trace) */
	}

	/* Copy bytes */
	srcBuffer = (byte_t*)phys_to_ptr(src->paddr);
	destBuffer = (byte_t*)phys_to_ptr(dest->paddr);
	dest_offset = 0;

// BH: highly inefficient
	while (count--)
	{
		if ((src_offset == src->count) || (dest_offset == dest->count))
			return originalCount - count;	/* Cannot copy anymore */

		destBuffer[dest_offset++] = srcBuffer[src_offset++]; /* Copy byte */

		/* Jump to next page if needed */
		if (src_offset == src->count)
		{
			src = src->next;
			if (src == NULL)
				return originalCount - count;	/* Cannot copy anymore */

// BH: style - tabs vs. spaces
            srcBuffer = (byte_t*)phys_to_ptr(src->paddr);
			src_offset = 0;
		}
		if (dest_offset == dest->count)
		{
			dest = dest->next;
			if (dest == NULL)
				return originalCount - count;	/* Cannot copy anymore */
            destBuffer = (byte_t*)phys_to_ptr(dest->paddr);
			dest_offset = 0;
		}
	}

	return originalCount;
}

