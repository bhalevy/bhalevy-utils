/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{

	//no mutual access exclusion. one thread may change the list while removing is in action

	packet **i = &pending_list;

	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) {
		//does nothing- do not change the list
		(*i) = (*i)->next;
	}
	//else? - should handle removing p if it is the last element

	if (p != NULL) {
		packetfree(p);
	}
}
