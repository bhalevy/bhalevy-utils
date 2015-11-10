/*******************************************/
/* quick and dirty test program for sg_map */
/*******************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sg_copy.h"

void print_map(sg_entry_t *map)
{
	sg_entry_t *m = map;
	int i = 1;
	
	while (m != NULL)
	{
		printf ("entry %d: @0x%08x paddr = 0x08%x   count = %d   next = 0x%08x\n", i, (unsigned long)(m),(unsigned long)(m->paddr), m->count, (unsigned long)(m->next));
		m = m->next;
		i++;
	}
}


int main(int argc, char **argv)
{

	void *buffer;
	int length;
	sg_entry_t *map;

	buffer = (void*)0x1000003;
	length = 127;

	/******************************** 
		test sg_map allocation 
	*********************************/
	
	printf ("allocate %d bytes starting at 0x%08x\n", length, buffer);
	map = sg_map(buffer, length);

	printf ("Allocated sg_map\n");
	
	print_map(map);
	
	/******************************** 
		test sg_destroy 
	*********************************/
	
	printf ("\n\nDestroy map @0x%08x\n", map);
	sg_destroy(map);
	printf ("Destroyed map\n");
	
	/********************************
		test sg_copy 
	*********************************/

	printf ("\n\nCopy test:\n");
	
	int offset = 0;
	int size;
	
	sg_entry_t *src = sg_map(buffer, 100);
	sg_entry_t *dest = sg_map((sg_entry_t *)(buffer+0x2000001), 100 + 128);
		
	printf("\nSrc map allocated:\n");
	print_map(src);
	printf("\nDest map allocated:\n");
	print_map(dest);
	
	
	offset = 0; length = 100; 
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);

	offset = 1; length = 100;
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	offset = 33; length = 100;
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	/* src offset out of bound */
	offset = 101; length = 100;
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	/* length longer then src */

	offset = 1; length = 200;
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	offset = 0; length = 50; 
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);

	offset = 10; length = 50; 
	printf ("\nCopy Src to Dest offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	/* dest shorter then src */
	
	sg_entry_t *dest2 = sg_map((sg_entry_t *)(buffer+0x3000000), 100 - 20);
	
	printf("\nSrc map allocated:\n");
	print_map(src);
	printf("\nDest2 map allocated:\n");
	print_map(dest2);

	offset = 1; length = 50;
	printf ("\nCopy Src to Dest2 offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest2, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	offset = 1; length = 80;
	printf ("\nCopy Src to Dest2 offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest2, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	offset = 1; length = 90;
	printf ("\nCopy Src to Dest2 offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest2, offset, length);
	printf ("Copied %d bytes\n\n", size);
	
	offset = 1; length = 110;
	printf ("\nCopy Src to Dest2 offset = %d length = %d\n", offset, length);
	size = sg_copy(src, dest2, offset, length);
	printf ("Copied %d bytes\n\n", size);
}

