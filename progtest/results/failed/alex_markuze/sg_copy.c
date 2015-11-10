/**
Some leagal disclamer...
**/
#include <errno.h>
#include <stdlib.h>
#include "sg_copy.h"

// BH: required
#include <string.h>

static inline void *ERR_PTR(int err)
{
	return NULL;
}

#define min(x,y) (((x)<(y))?(x):(y))

void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t * tmp;

	while (sg_list) { 
		tmp = sg_list->next;
		free(sg_list);
		sg_list = tmp;
	}
	//also works if sg_list == NULL
}

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t	*next, *prev;
	physaddr_t	tmp;
	int			tmp_sz;
	sg_entry_t	*map;
void *tmpbuf;

	if ( !buf || length <= 0) {
		return ERR_PTR(-EINVAL);
	}

	map = (sg_entry_t *)malloc(sizeof(sg_entry_t));	

	if ( !map ) {
		return ERR_PTR(-ENOMEM);
	}

	tmp			= ptr_to_phys(buf);
	map->paddr	= tmp;
	map->next	= NULL;

// BH: BUG: use buf, not paddr
//	tmp = (tmp + min(PAGE_SIZE,length)) & ~(PAGE_SIZE - 1);//calculate nearest PAGE_SIZE boundry
//	tmp_sz		= tmp - map->paddr;
tmpbuf = (void *)(((unsigned long)buf + min(PAGE_SIZE,length)) & ~(PAGE_SIZE - 1));
tmp_sz		= tmpbuf - buf;
	map->count	= (tmp_sz) ? tmp_sz: length;

	if ( !tmp_sz ) 
		return map;
	/*tmp_sz is zero if buffer_ptr + length was inside one page size 
		_____________________________________________________
		|			|///////////////////|				|
		-----------------------------------------------------
					^beffer start		^ + length		^PAGE_SIZE boundry 
	*/
	buf		+= tmp_sz;
	length	-= tmp_sz;
	prev	= map;

	for ( ; length >= 0; length -= PAGE_SIZE ) {
		next	= (sg_entry_t *)malloc(sizeof(sg_entry_t));
		if( !next ){
			sg_destroy(map);
			return ERR_PTR(-ENOMEM);
		}
		prev->next	= next;
		prev		= next;
		next->paddr = ptr_to_phys(buf);
//		map->count	= min(PAGE_SIZE,length);//length is decreased each time in the for header.
next->count	= min(PAGE_SIZE,length);//length is decreased each time in the for header.
		buf			+= PAGE_SIZE;			//if length is less then PAGE_SIZE we will get out of the loop
		next->next	= NULL;
	}
	return map;
}

int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	size_t size;
	int copied = 0, src_cnt, dst_cnt;
	void * psrc, * pdest;

	if ( ! src || !dest )
		return 0;

	if (src_offset < 0 || count <= 0)
		return 0;

	while ( src && src_offset >= src->count) { //offset may be larger then src list so we check for src == NULL
		src_offset -= src->count;
		src			= src->next;
	}

	if ( !src )
		return 0;//offset was the size of the src list

	size	= min( min( dest->count, src->count - src_offset), count);//determinig first copy size
	pdest	= phys_to_ptr(dest->paddr);
	psrc	= phys_to_ptr(src->paddr) + src_offset;
	dst_cnt = dest->count;
	src_cnt = src->count - src_offset;
//1'st simple case: first sg segment is the same size in src and dest if count is smaller then both it doesnt matter.
	if (dst_cnt == src_cnt || size == count) {
		while ( size ) {
//			memcpy(pdset, psrc, size);
			memcpy(pdest, psrc, size);
			copied	+= size;
			count	-= size;

			dest	= dest->next;
			src		= src->next;

			if (!dest || !src)
				return copied;
	//computing the source destination and size for next segment copy
			size	= min( min( dest->count, src->count), count);
			pdest	= phys_to_ptr(dest->paddr);
			psrc	= phys_to_ptr(src->paddr);
		}
		return copied;
	}
//2'nd case: first sg segment in dest is smaller then the src->count - src_offset
//3'd case : first sg segment in src->count - src_offset is smaller then the dest
//each step cases interchange
	while (size) {
		if (dst_cnt < src_cnt) {
//				memcpy(pdset, psrc, size);
				memcpy(pdest, psrc, size);
				copied	+= size;
				count	-= size;
				dest	= dest->next;
				// we move to next sg elem in dest, but we have not finnished copying from source
				if ( !dest )
					return copied;

				src_cnt -= size;	// calculate how much of sg elem still needs to be copied
				dst_cnt = dest->count;// last field may be smaller then PG_SIZE
				pdest	= phys_to_ptr(dest->paddr);
				psrc	+= size;	//this virtaul address + size are still mapped to the same physical page
				size	= min(min(src_cnt, count),dst_cnt);//last sg elem count may be small
		}
// 3'd case comment are symetrical to the comments for the second case
		if ( size && dst_cnt > src_cnt) {
//				memcpy(pdset, psrc, size);
				memcpy(pdest, psrc, size);
				copied	+= size;
				count	-= size;
				src		= src->next;
				if ( !src )
					return copied;

				src_cnt = src->count;
				dst_cnt -= size;
				pdest	+= size; 
				psrc	= phys_to_ptr(src->paddr);
				size	= min(min(dst_cnt, count),src_cnt);
		}
	}
return copied;
}
