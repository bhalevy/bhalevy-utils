/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list; <- First of all, if "pending_list" is a pointer to the head of the list, this function will change it (the value of the "pending_list")
	                            <- and it's unlikely the expected behaviour.

	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) { <- If this true, then ((*i) == p)
		(*i) = (*i)->next; <- This will change "pending_list" to be equal to p->next, which could in particular be NULL . Hmmm...
				   <- I don't see how it would delete the element at all. Removing one level of indirection would improve the situation but then we
				   <- have to add a proper handling the last element handling case.
	}

	if (p != NULL) {
		packetfree(p);
	}
}

/* This is a proper implementation of the above */
void remove_from_pending_list(packet *p)
{
	packet *i = pending_list;

	/* Sanity */
	if (!p)
		return;

	/* If we need to delete an element at the head of the list */
	if (i == p) {
		/*
		 * Here we assume that pending list is variable that is used to
		 * store the head of the list
		 */
		pending_list = p->next;
		packetfree(p);
		return;
	}


	for (;(i->next != NULL) && (i->next != p); i = i->next);

	/* If we found it */
	if (i->next  != NULL) {
		i->next = p->next;
		packetfree(p);
	}
}