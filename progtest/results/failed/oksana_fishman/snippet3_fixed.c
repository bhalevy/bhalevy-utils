void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	for (;(*i) != NULL && ((*i)->next != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) {
		if (*p != NULL) {
			(*i)->next = (*p)->next; /*remove the element*/
		}
		else
		{
			(*i)->next =NULL;
		}
	}

	if (p != NULL) {
		packetfree(p);
	}
}
