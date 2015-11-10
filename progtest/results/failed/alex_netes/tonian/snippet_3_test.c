#include <stdio.h>
#include <stdlib.h>

#define MAX_PACKET 4

typedef struct packet
{
	int id;
	struct packet *next;
} packet;

packet *pending_list;

void packetfree(void *p)
{
	free(p);
}

void print_list()
{
	packet *tmp_p;

	for (tmp_p = pending_list; tmp_p != NULL; tmp_p = tmp_p->next)
		printf("{%p}[%d]->", tmp_p, tmp_p->id);
	printf("\n...\n");
}


int fill_list()
{
	int i;
	packet *tmp_p;

	for (i=0; i < MAX_PACKET; i++)
	{
		tmp_p = (packet*)malloc(sizeof(packet));

		if (!tmp_p) {
			printf("malloc failed\n");
			return -1;
		}
		tmp_p->id = i;
		tmp_p->next = pending_list;
		pending_list = tmp_p;
	}
	return 0;
}


/********************  Original Code *************************/
 

/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	printf("Pointer to the packet that should be removed (p): %p\n"
		"Pointer to the head the list (i): %p\n"
		"Pointer to the next packet of the head of the list (i->next): %p\n"
		"Pointer to the head the list (pending_list): %p \n",
		p, (*i),(*i)->next, pending_list);
	printf("...\n");
	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}

/******************** END Original Code *************************/


int main(int argc, char* argv)
{
	packet *tmp_p;

	pending_list = NULL;
	if (fill_list()) {
		printf("fill_list failed\n");
		return -1;
	}

	printf("Test after fill: ");
	print_list();
	tmp_p = pending_list;
	while(tmp_p && tmp_p->next)
		tmp_p = tmp_p->next;

	remove_from_pending_list(tmp_p);
	printf("Test after remove: ");
	print_list();
}
