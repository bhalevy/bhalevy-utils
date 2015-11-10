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
 * The above code has the following errors:
 * 
 * 1. The pending_list should have a lock if it's a multithreaded application - must have a lock if it's kernel code
 * 2. line 10: *i = ((*i)->next) modifies the global pending_list ptr - the data will probably be leaked - should use i = &(*i)->next
 * 3. line 17: frees p regardless of the above search (i.e. p will be freed if it's on the list or not)
 */
