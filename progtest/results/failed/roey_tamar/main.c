
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sg_copy.h"

#define byte char
#define BUFFER_SIZE 6

extern void buffer_dump(void* buf, int length);

extern void sg_list_dump(sg_entry_t* list);

int main(){

	int err = 0;
	sg_entry_t* list = NULL;
	void* buf = NULL;

	buf = malloc(sizeof(byte) * BUFFER_SIZE);
	if (!buf) {
		printf("Failed to allocate memory\n");
		return 0;
	}

	memset(buf, 'd', BUFFER_SIZE);

	*(((byte*)buf) + 0) = 'a';
	*(((byte*)buf) + 1) = 'b';
	*(((byte*)buf) + 2) = 'c';
	*(((byte*)buf) + 3) = 'e';

	list = sg_map(buf, BUFFER_SIZE);
	if (!list) {
		printf("Failed to create scatter-gather list\n");
		return err;
	}

	sg_list_dump(list);

	printf("number of copied bytes = %u\n", sg_copy(list, list, 4, 2));

	sg_list_dump(list);

	sg_destroy(list);

	free(buf);

	return err;
}
