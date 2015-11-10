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
	/*
	 * If p is NOT part of the list (including the case where
	 * the list is empty),
	 * it will cause the release of p even
	 * if it's not part of the list (say by beeing a wrong argument)
	 * which is not a bahaviour one expects from the code.
	 * Moreover, if p not the first item in the list, the list
	 * will be broken (if p is not the last item on the list)
	 * or the new last item (in case p was the last one in the original list)
	 * will point to an undefined address instead of pointing to NULL
	 */

	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}
