/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

    /* the following line had to be corrected in order for the function to work */
	//for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
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
