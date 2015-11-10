/*
 * Scatter/gather list - Unit Test
 *
 * January 30, 2012 Dave Stav <dave@stav.org.il>
 *
 * Tonian Systems Inc.
 */

#include <stdio.h>
#include <string.h>
#include "sg_copy.h"

static void sg_print(sg_entry_t *sg)
{
	for (; sg; sg = sg->next)
		printf("[%2d] \"%.*s\"\n", sg->count, sg->count,
				(char *)phys_to_ptr(sg->paddr));
	printf("\n");
}

int main(int argc, char **argv)
{
	sg_entry_t *sg1, *sg2, *sg3;
	char buf1[] =
		"Luke Luck likes lakes. "
		"Luke's duck likes lakes. "
		"Luke Luck licks lakes. "
		"Luck's duck licks lakes.";
	char buf2[] =
		"Duck takes licks in lakes Luke Luck likes. "
		"Luke Luck takes licks in lakes duck likes.";
	char buf3[] = "012345678901234567890123456789";
	int ret;

	sg1 = sg_map(buf1, strlen(buf1));
	sg_print(sg1);

	sg2 = sg_map(buf2, strlen(buf2));
	sg_print(sg2);

	sg3 = sg_map(buf3, strlen(buf3));
	sg_print(sg3);

	ret = sg_copy(sg3, sg2, 5, 100);
	printf("Copied %d bytes\n", ret);
	sg_print(sg2);

	sg_destroy(sg1);
	sg_destroy(sg2);
	sg_destroy(sg3);

	return 0;
}
