/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
    /* create a pointer to the next member of a node which is a pointer
       to a node, initaly it points to list's first node 
     */
    packet **i = &pending_list;

    /* iterate through the list by dereferencing the next member pointer 
       until it dereferences the next pointer of the node that points to
       p or to list's end
     */
	for (;(*i) != NULL && ((*i) != p); i = &((*i)->next)) {
		/* do nothing */;
	}

    /* now check that it is not list end, so it must be the next pointer of the node that
       precedes p. rewitite the content of this pointer with the the next pointer, such that the next pointer of
       the current node is pointing to p's next node.
     */
	if (*i != NULL) {
		(*i) = (*i)->next;
	}

    /* This will free the memory used by p */
	if (p != NULL) {
		packetfree(p);
	}
}

