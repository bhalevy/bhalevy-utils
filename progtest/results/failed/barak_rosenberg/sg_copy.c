#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"



// BH: absolutely messy code (tab vs. spaces?)

sg_entry_t *sg_map(void *buf, int length)
{
	sg_entry_t *chunk,*last_chunk;
    int size;
    void *current_place;

// BH: input limits not verified
	size=length;
	current_place=buf;

	last_chunk=NULL;

	while(size>0)
	{
	  chunk=malloc(sizeof(sg_entry_t));

	  if(!chunk)
		  {
		  			printf("error in malloc in sg_map\n");
		  			return 0;
		  }


// BH: BUG: first page alignment
	  chunk->paddr = ptr_to_phys(current_place);
	  if (size>PAGE_SIZE)
	   chunk->count = PAGE_SIZE;
	  else
	   chunk->count = size;

      size=size-PAGE_SIZE;                              // reduce number of bytes by PAGE_SIZE
      current_place=current_place+PAGE_SIZE;            // move current_place to next PAGE
	  chunk->next=last_chunk;                          // move to next chunk
	  last_chunk=chunk;
	}

// BH: BUG: what the hack?  who asked for a reverse list?
    return last_chunk;
}


void sg_destroy(sg_entry_t *sg_list)
{
	sg_entry_t *tmp;
	while (sg_list)
	{
	 tmp=sg_list->next;
	 free(sg_list);
	 sg_list=tmp;
	}
}


// BH: no no no, cannot copy onto physical addresses
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	if(src->count-src_offset < count)               // in a list, number of bytes minus offset is less then count bytes
	{												// then copy only number of bytes minus offset that exist in element
	  memcpy(&dest->paddr,&src->paddr+src_offset ,src->count-src_offset);
	  return (src->count-src_offset);
	}
	else
	{
	  memcpy(&dest->paddr,&src->paddr+src_offset ,count);
	  return count;
	}



}

#if 0
int main()
{
	sg_entry_t  *chunks, *current_chunk;

	char str[]="cdsbhjcbnhgnghngfngfbfgbfgkmlmgfbgfbgncdscsdfghnhdfgkjbnfgjkbngfjknkkj";

	chunks=sg_map(str,strlen(str) );

	current_chunk=chunks;

	printf("string is :%s\n\n",str);

	while(current_chunk)
	{
	 printf("address= %lu count=%d\n",current_chunk->paddr,current_chunk->count);

	 current_chunk=current_chunk->next;
	}


    //sg_copy(chunks,chunks->next,0,3);



	sg_destroy(chunks);


	return 0;
}


#endif
