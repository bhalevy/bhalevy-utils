
#include <stdio.h>
#include <stdlib.h>

#define N_ENTRIES (4)

struct packet_s {
	int data;
	struct packet_s * next;
};
typedef struct packet_s packet;

packet * pending_list;

int init_pending_list();
void remove_from_pending_list(packet *p);
void packetfree(packet *p);
void showlist();
packet * get_last_packet();


/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

     /* for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {  Itay: bug - changing the list, should be: */
        for (;(*i) != NULL && ((*i) != p); i = &((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}

void packetfree(packet *p)
{
	free(p);
}

/* init the list with N_ENTRIES */
int init_pending_list()
{
	int i = 0;
        packet **p = &pending_list;

        printf("initializing list\n");

        for (i = 0; i < N_ENTRIES; i++) {
                *p = malloc(sizeof(packet));
                if (! *p) {
                    return -1;
                }
                (*p)->data = i;
                (*p)->next = NULL;
                p = &((*p)->next);
        }

	return 0;
}

void showlist()
{
	packet * p = pending_list;
        printf("list: [");
	while (p) {
		printf("%d", p->data);
		if (p->next) {
			printf (", ");
		}
		p = p->next;
	}
	printf("]\n");
}

packet * get_last_packet()
{
	packet * p = pending_list;

	for (;p != NULL && p->next != NULL; p = p->next) {
	}

	return p;
}

int main(void) 
{
        packet * p = NULL;

	if (init_pending_list() < 0) {
		printf("error allocating list\n");
		return EXIT_FAILURE;
	}
	showlist();

        printf("\nremove all from the beginning:\n");
        
        while (pending_list) {
                printf("removing packet: %d from list\n", pending_list->data);
                remove_from_pending_list(pending_list);
                showlist();

        }

        printf("\n");
        if (init_pending_list() < 0) {
		printf("error allocating list\n");
		return EXIT_FAILURE;
	}
	showlist();

        printf("\nremove second packet\n");
        
        printf("removing packet: %d from list\n", pending_list->next->data);
        remove_from_pending_list(pending_list->next);
        showlist();

        printf("\nremove the rest from the end\n");
        while ((p = get_last_packet()) != NULL) {
	        printf("removing packet: %d from list\n", p->data);
                remove_from_pending_list(p);
                showlist();
        }

        printf("removed all\n");
        showlist();

	return EXIT_SUCCESS;
}

