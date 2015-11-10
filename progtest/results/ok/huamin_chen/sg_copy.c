#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// BH: missing definition fo assert
static void assert(int cond)
{
	if (!cond) {
		fprintf(stderr, "ASSERT failed\n");
		exit(1);
	}
}

#include "sg_copy.h"

#if 0
sg_entry_t *sg_map(void *buf, int length)
{
    int num_bufs = length / PAGE_SIZE;
    sg_entry_t *sg_list = NULL, *last_sg = NULL;
    int total = 0, i;

    if (!buf || !length <= 0)
	return NULL;

    /* allocate sg_list */
// BH: BUG: what about first buf alignment?
    if (length % PAGE_SIZE)
	num_bufs ++;
    
    sg_list = (sg_entry_t *)malloc(num_bufs * sizeof(sg_entry_t));
    /* exit on OOM */
    if (!sg_list){
	perror("malloc");
	exit(1);
    }
    /* initialize vectors from last to 2nd */
// BH: BUG: off by one
//    for (i=num_bufs; i > 0; i--){
for (i=num_bufs-1; i > 0; i--){
	sg_list[i].paddr = ptr_to_phys(buf-total);
// BH: BUG: last buffer count
	sg_list[i].count = PAGE_SIZE;
	sg_list[i].next = last_sg;
	last_sg = &sg_list[i];
	total += PAGE_SIZE;
    }
    
    /* residual goes to the first vector */
    sg_list[0].paddr = ptr_to_phys(buf);
    sg_list[0].count = length - total;
    assert(sg_list[0].count <= PAGE_SIZE);
    sg_list[0].next = last_sg;
    return sg_list;
}
#endif

static sg_entry_t *
sg_map_entry(void *p, int length)
{
	int count;
	sg_entry_t *sg;

	sg = malloc(sizeof(sg_entry_t));
	sg->paddr = ptr_to_phys(p);
	count = length;
	if (count + (sg->paddr & (PAGE_SIZE-1)) > PAGE_SIZE)
		count = PAGE_SIZE - (sg->paddr & (PAGE_SIZE-1));
	sg->count = count;
	sg->next = NULL;

	return sg;
}

sg_entry_t *
sg_map(void *buf, int length)
{
	char *p = buf;
	sg_entry_t *sg, *sg_list = NULL, **next;

	if (!buf || length <= 0)
		return NULL;

	for (next = &sg_list;
	     length;
	     p += sg->count, length -= sg->count, next = &(*next)->next)
		sg = *next = sg_map_entry(p, length);

	return sg_list;
}


void sg_destroy(sg_entry_t *sg_list)
{
    sg_entry_t *sg = sg_list, *next;

    /* handle special case */
// BH: why?
    if (!sg_list)
	return;
    
    while(sg){
	next = sg->next;
	/* FIXME: should we unmap user buffer? */
	free(sg);
	sg = next;
    }
}

/*
 * copy data between two non-overlapping scatter-gather list.
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
    int start = 0, num = 0;
    sg_entry_t *src_it = src, *dest_it = dest;

    /* handle special case */
// BH: test src_offset
//    if (!src || !dest || count <= 0)
    if (!src || !dest || count <= 0 || src_offset < 0)
	return 0;

    while (src_it){
	if (start + src_it->count > src_offset){
	    /* locate the start offset, start copying */
	    int begin_off = start + src_it->count - src_offset;
	    int bytes = src_it->count - begin_off;
	    if (bytes > count)
		bytes = count;
	    assert(begin_off <= PAGE_SIZE && bytes <= PAGE_SIZE);
// BH: BUG: copying into phys addr
//	    memcpy(dest->paddr,src_it->paddr + begin_off, bytes);
memcpy(phys_to_ptr(dest->paddr), phys_to_ptr(src_it->paddr + begin_off), bytes);

  	    count -= bytes;
	    num += bytes;
	    while (count > 0 && src_it && dest_it){
		src_it = src_it->next;
		dest_it = dest_it->next;
		bytes = src_it->count;
		if (bytes > count)
		    bytes = count;
// BH: BUG: copying into phys addr
//		memcpy(dest->paddr,src_it->paddr, bytes);
memcpy(phys_to_ptr(dest->paddr), phys_to_ptr(src_it->paddr), bytes);
		count -= bytes;
		num += bytes;
	    }
	    return num;
	}else{
	    start += src_it->count;
	    src_it = src_it->next;
	}
    }
    return num;
}
