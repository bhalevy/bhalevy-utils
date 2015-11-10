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

/* The function will not work.

   In each iteration of the loop '*i = ((*i)->next' makes the 'pending_list' to point to the next packet.
   And in the end of the function 'pending_list' will point to the packet that came after the
   packet that was deleted in list. And the previous packet's 'next' will point to garbage instead of the
   packet that came after the packet that was deleted.

   When the linked list has initially 4 elements, and the last element is supposed to be deleted,
   after the loop '(*i)' and 'pending_list' will point to the last element.
   After '(*i) = (*i)->next;' in the 'if' statement, '(*i)' and 'pending_list' will point to NULL.

   In case no other pointer points to the begining of the original pending_list, the first 3 elements will be lost.
   In any case 3th element's 'next' will point to garbage and not to NULL.
 */

