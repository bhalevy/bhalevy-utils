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
/* previous line is right only when we look for the first element in the list (and that's only if the list is not b-directional,
or circular) and won't work for next elements
if it's b-directional link list: */
	if (*i != NULL) {
		if ((*i)->prev) {
			(*i)->prev->next = (*i)->next;
			(*i)->next->prev = (*i)->prev;
		else {
			(*i) = (*i)->next;
			(*i) = (*i)->prev = NULL;
		}
	}
/* if it's not b-directional, need to save the previous element in the for loop, and update it's next with p->next
/ see below */

	if (p != NULL) {
		packetfree(p);
	}
}

if it's not b-directional:
{
	packet **i = &pending_list;
	packet *prev = NULL;

	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		prev = *i;
	}

	if (*i != NULL) {
		if (prev) prev->next = (*i)->next
		else (*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}
