/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet *i = pending_list;

    if (p == NULL)
    {
        return; // TODO: trace an error
    }

	for (;(i->next) != NULL && ((i->next) != p); i = (i->next)) {
		/* do nothing */;
	}

	if (i->next == p) {
		i->next = p->next;
        packetfree(p);
	}
}