#include <stdlib.h>
#include <stdio.h>
#include "sg_copy.h"

void dump_sg_list(sg_entry_t *sg)
{
	int i,j=0;
	while (sg) {
		printf("entry: %d | count: %d\n", j, sg->count);
		for (i = 0; i < sg->count; i++) 
			printf("0x%x ", *(unsigned char*)(phys_to_ptr(sg->paddr)+i));
		sg = sg->next;
		printf("\n");
		j++;
	}
}

int main(int argc, char *atgv)
{
	unsigned char buf1[PAGE_SIZE];
	unsigned char buf2[PAGE_SIZE*2];
	unsigned char buf3[PAGE_SIZE*3+PAGE_SIZE/2];

	sg_entry_t *sg1 = NULL;
	sg_entry_t *sg2 = NULL;
	sg_entry_t *sg3 = NULL;

	int i, rt;
	int j = 0;

	/* Init buffers */
	for (i=0; i<PAGE_SIZE; i++)
		buf1[i] = j++;

	for (i=0; i<PAGE_SIZE*2; i++)
		buf2[i] = j++;

	for (i=0; i<PAGE_SIZE*3+PAGE_SIZE/2; i++)
		buf3[i] = j++;

	/* Map buffers to sg's */
	printf("Mapping buffers...\n");
	sg1 = sg_map(&buf1, PAGE_SIZE);
	if (!sg1) {
		printf("sg_map sg1 failed\n");
		return -1;
	}
	sg2 = sg_map(&buf2, PAGE_SIZE*2);
	if (!sg2) {
		printf("sg_map sg2 failed\n");
		return -1;
	}

	sg3 = sg_map(&buf3, PAGE_SIZE*3+PAGE_SIZE/2);
	if (!sg3) {
		printf("sg_map sg3 failed\n");
		return -1;
	}
	/* Test sg_map() */
	printf("\nDump sg1:\n");
	dump_sg_list(sg1);
	printf("\nDump sg2:\n");
	dump_sg_list(sg2);
	printf("\nDump sg3:\n");
	dump_sg_list(sg3);

	/* Run some sg_copy's */
	printf("Copying buffers...\n");
	rt = sg_copy(sg2, sg1, 0, sizeof(buf1));
	printf("Coppied %d bytes from sg2 to sg1\n", rt);
	rt = sg_copy(sg3, sg2, PAGE_SIZE/2, PAGE_SIZE*3+PAGE_SIZE/2);
	printf("Coppied %d bytes from sg3 to sg2\n", rt);

	/* Test sg_copy() */
	printf("sg1:\n");
	dump_sg_list(sg1);
	printf("\nsg2:\n");
	dump_sg_list(sg2);
	printf("\nsg3:\n");
	dump_sg_list(sg3);

	/* Destroy sg's */
	sg_destroy(sg1);
	sg_destroy(sg2);
	sg_destroy(sg3);

	return 0;
}
