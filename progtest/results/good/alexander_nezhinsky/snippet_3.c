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

/*
    The logical mistake is in changing "next" of the previous packet
    to point to the next after "p", after advancing past "p"
    
    I also did not understand the necessity of using double pointers.
    
    The type of pending_list was not specified. There are 2 possible
    implementations. For both of them a fixed version is provided
*/


/* Implementation 1 (the list header is a dummy packet, 
   its "next" refers to the 1st list entry) */
packet pending_list;

void remove_from_pending_list(packet *p)
{
	packet *i;

	if (p == NULL)
		return;

	for (i = &pending_list; i != NULL && i->next != p; i = i->next);

	if (i != NULL) 
		i->next = p->next;

	packetfree(p);
}

/* Implementation 2 (the list header is a pointer to the 1st list entry) */
packet *pending_list;

void remove_from_pending_list(packet *p)
{
	packet *i;

	if (p == NULL)
		return;

	if (pending_list == p)
		pending_list = p->next;
	else {
		for (i = pending_list; i != NULL; i = i->next) {
			if (i->next == p) {
				i->next = p->next;
				break;
			}
		}
	}
	packetfree(p);
}
