/*
 * sg_copy.c
 *
 *  Created on: Jan 5, 2012
 *      Author: haim
 */
#include "sg_copy.h"
#include "malloc.h"
#include "memory.h"

#define SIZE_TO_ALLOC (PAGE_SIZE + sizeof(void *))

//All entries except the first one must be aligned on a PAGE_SIZE address
void *sg_entry_alloc()
{
	int offset = 0;
	void *p=0, *orig=0;
	orig = (sg_entry_t *)calloc(1, SIZE_TO_ALLOC + sizeof(sg_entry_t));
	if (!orig)
		return 0;
	p = orig + SIZE_TO_ALLOC;
	offset = ((unsigned long)p)%PAGE_SIZE;
	if (offset)
		p -= offset;
	p -= sizeof(void *);
	*(void **)p = orig;
	p += sizeof(void *);
	return p;
}
void sg_entry_free(void *p)
{
	if (!p)
		return;
	p -= sizeof(void *);
	p = *(void **)p;
	free(p);
}

sg_entry_t *sg_map(void *buf, int length)
{
	char *p=0;
	sg_entry_t *head=0, **i=0;
	if (!buf || length <=0)
		return 0;

	for (i=&head; length; i=&(*i)->next)
	{
		//All entries except the first one must be aligned on a PAGE_SIZE address
		if (i == &head)
			*i =(sg_entry_t *)calloc(1, sizeof(sg_entry_t));
		else
			*i = sg_entry_alloc();

		if (!*i)
		{
			sg_destroy(head);
			return 0;
		}
		(*i)->count = (length > PAGE_SIZE ? PAGE_SIZE : length);
		length -= (*i)->count;

#if BUG
		// allocate where to place the buffer.
		p = malloc(PAGE_SIZE);
		if (!p)
		{
			sg_destroy(head);
			return 0;
		}
		(*i)->paddr = ptr_to_phys(p);
		memcpy(p, buf, (*i)->count);
#endif
(*i)->paddr = ptr_to_phys(buf);

		p[(*i)->count] = 0;
		buf+=(*i)->count;
	}
	return head;
}

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *i=0, *n=0;
	void *p=0;

	for (i=sg_list; i; i=n)
	{
		n=i->next;
		p = phys_to_ptr(i->paddr);
		free(p);

		//All entries except the first one must be aligned on a PAGE_SIZE address
		// so we need to free properly.
		if (i == sg_list)
			free(i);
		else
			sg_entry_free(i);
	}
}
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	void *buf_src=0, *buf_dst=0;
	int dst_offset=0, num_bytes=0, num_copied=0;
	if (!src || !dest || src_offset < 0 || count <= 0)
		return 0;

	// find the src entry to start from and the offset in the entry.
	for ( ; src && src_offset >= PAGE_SIZE; src_offset-=PAGE_SIZE, src = src->next);
	if (!src)
		return 0;

	for (; src && dest && count; count-=num_bytes)
	{
		// locate the buffer positions for
		// reading and writing.
		buf_src = phys_to_ptr(src->paddr);
		buf_src += src_offset;
		buf_dst = phys_to_ptr(dest->paddr);
		buf_dst += dst_offset;

		// number of bytes that we can copy is the minimum
		// between:
		// 1. the number of bytes that we can from src entry.
		// 2. the number of bytes that we still need to copy
		// 3. the number of bytes that we can write to dest entry.
		num_bytes = MIN(src->count - src_offset, count);
		num_bytes = MIN(num_bytes, PAGE_SIZE - dst_offset);

		memcpy(buf_dst, buf_src, num_bytes);
		num_copied += num_bytes;

		// if dest entry reach it's limit move to next entry.
		dst_offset += num_bytes;
		if (dst_offset >= PAGE_SIZE)
		{
			dest->count = PAGE_SIZE;
			dest = dest->next;
			dst_offset = 0;
		}
		// if src entry reach it's limit move to next entry.
		src_offset += num_bytes;
		if (src_offset >= src->count)
			break;
		if (src_offset >= PAGE_SIZE)
		{
			src = src->next;
			src_offset = 0;
		}
	}

	if (dest)
		dest->count = MAX(dest->count, dst_offset);
	return num_copied;
}

void sg_dump(sg_entry_t *sg)
{
	int i;
	char *b;
	if (!sg)
		return;

	for ( ; sg; sg=sg->next)
	{
		b = phys_to_ptr(sg->paddr);

		printf("%p:%.2d: \"", sg, sg->count);
		for (i=0; i<sg->count; i++)
		{
			if  (('A' <= b[i] &&  b[i] <= 'Z') ||
				('a' <= b[i] &&  b[i] <= 'z') ||
				('0' <= b[i] &&  b[i] <= '9'))
			{
				printf("%c", b[i]);
			}
			else
				printf(".");
		}
		printf("\"\n");
	}
}
