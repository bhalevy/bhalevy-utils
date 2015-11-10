/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) { //1. Each iterration modifies the global pending_list
		/* do nothing */;
	}

	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		//2.previous ptr still pointing to p , linked list is broken
		packetfree(p);
	}
}

/*
Will NOT work. 
	reasons:
	1. Function not reusable  global ptr will be modified each time.
	2. Basic functionality of this function should be (prev->next == p) ==> prev->next = p->next; 
		this operation is missing.
	3. Not thread safe
*/
