/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

    /*
     * This advances the pending_list pointer itself, so
     * effectively we are removing ALL packets until p
     * from the list (and leaving some dangling pointers
     * on the way).
     * Also, the loop should stop just BEFORE p (and not AT
     * p), so we could update the next pointer of the packet
     * proceeding p to point to the packet following p.
     */
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
