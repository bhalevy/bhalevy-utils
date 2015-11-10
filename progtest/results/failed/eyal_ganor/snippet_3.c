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

	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}

The problem is that In a linked list - the specific pkt will be removed - but the previous one on the list
will still point on it Instead of pointing on the p->next.
In order to solve we need additional parameter (prev) that will point on one item before *i, and after the deletion of p,
prev->next will point on (*i).
