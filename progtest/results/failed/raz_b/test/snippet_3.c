/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */


void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	/* we exit the bellow loop when at least one of the two happens :
		1.  *i = 0
		2.  *i = p 
	*/
	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	/* 
		i1 -> i2 -> i3 -> i4 -> NULL
		if p = i4 then i is NULL
	 */
	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
	/* we released i4 from list without nullifying i3->next , list is broken */
}
