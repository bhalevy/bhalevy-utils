/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */

//#define __WITH_BUGS__
#ifdef __WITH_BUGS__
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	// 1. by iterate this way we actually changing the address in
	//		the global variable pending_list and there for losing the
	//		head of the list.
	// 2. we are not keeping reference to the previous packet in order to
	//		connect it with the packet following the
	//		one we would like to delete.
	// 3. no validity test on p. if p is NULL then we will iterate on the
	//		list for no reason.
	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	// 4. if p is NULL we need to return.
	if (*i != NULL) {
		// 5. pending_list will point to the packet followed th packet we want
		// 		to delete.
		(*i) = (*i)->next;
	}

	// 6. this validity test should be in the beginning in order to save
	//		meaning less iterations.
	if (p != NULL) {
		packetfree(p);
	}
}
#else

#include "tonian.h"
#include "malloc.h"

static packet *pending_list = 0;

static void packetfree(void *p)
{
	if (!p)
		return;
	free(p);
}

void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;
	if (!p)
		return;
	for (; *i && *i != p; i=&(*i)->next);

	if (!*i)
		return;
	*i=p->next;
	packetfree(p);
}

packet *add_to_list(int a)
{
	packet **i = &pending_list, *n=0;
	n = malloc(sizeof(packet));
	if (!n)
		return 0;
	n->val = a;

	for (; *i && (*i)->val < a; i=&(*i)->next);
	n->next = *i;
	*i = n;
	return n;
}

void dump_list()
{
	packet *i=pending_list;
	for (; i; i=i->next)
		printf("%d, ", i->val);
	printf("\n");
}
#endif
