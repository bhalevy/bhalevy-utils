/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	/* for (;(*i) != NULL && ((*i) != p); i = &((*i)->next)) { */
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


In the for loop *i = ((*i)->next) is a mistake and should be i = &((*i)->next).
The way the code is written the global pending_list (the list head) will be modified all the time eventually pointing to the element after p or NULL.
