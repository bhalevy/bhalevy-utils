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
 * The code is buggy, it whould only work in case p is the last packet in the list
 * (and even in this case the packet before p would still have new pointer to p which was already freed)
 * otherwise, we would lose the packet coming after p.
 * to avoid this we would need check:
 * if ((*i)->next == p) {
 * 	(*i)->next = p->next;
 *	packetfree(p);
 * }
 * considering the end cases (first packet, Null, etc)
 * 
 * More elegant solution (there is an assumption here p is not last, but complexity is O(1)):
 */
void remove_from_pending_list2(packet *p)
{
	packet *tmp;

	if (p == NULL)
		return; 

	tmp = p->next;
	memcpy(p, tmp, sizeof(packet));
	free(tmp);
}
