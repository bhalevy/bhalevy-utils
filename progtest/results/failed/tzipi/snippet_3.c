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
		/* Tzipi: the problem here is that the i element, which currently is the requested p,
			will instead point to the next element in the list. 
			but it won't update the previous element, i-1 - its "next" field should point to (*i)->next. 
			So now, when p is freed later on, the i-1 element will still point with its "next" field to an invalid memory address, p.*/
	}

	if (p != NULL) {
		packetfree(p);
	}
}
