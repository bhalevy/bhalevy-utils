/* 
	Main problem here is the bad assignment to *i in the 'for' loop.
	*i should be assigned THE ADDRESS of (*i)->next rather then the pointer held by (*i)->next
	
	If used as shown above - the result is a corrpution of the global pending_list variable,
	followed by a messup of the rest of the linked list ('next' pointers are shifted.)
	In case the packet does gets found, it is then not skipped correctly.

	Another potential issue is that packetfree() will be called even if p was not found, 
	This may or may not be a desirable behavior.
	
	Again, if not just a snippet, NULL needs to be defined and pending_list, packet, and packetfree() needs to be declared.

*/

/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;
	
	for (;(*i) != NULL && ((*i) != p) ; *i = &((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) {
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}

