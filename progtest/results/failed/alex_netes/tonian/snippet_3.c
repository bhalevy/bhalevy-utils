/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	/* 
	 * Alex:
	 * faulty assignment. Pointer to pending_list is being overwritten and lost later
	 */
	packet **i = &pending_list;

// BH: i = &((*i)->next)
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
