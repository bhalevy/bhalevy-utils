/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

    /*
     * @IU: Here we have an issue at *i = ((*i)->next)
     * Must be i = &((*i)->next), otherwise we're corrupting the list.
     */
	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

    /*
     * @IU: Here it's OK, we're folding in the list
     */
	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}
