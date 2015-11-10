/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
    /* i must point to the same location as pending_list,
       otherwise i changes pending_list's location*/
	packet *i = pending_list;
	
	/* if list is empty do nothing */
	if (pending_list == NULL){
       return;
    }

    /* locate p or the end of the list */
    /* by the time i is NULL we've already violated our increment */
	for (;i->next != NULL && (i->next != p) && (i != p); i = (i->next)) {
		/* do nothing */;
	}
	
	/* i now points to p, what points to p, or is null */
	/* if p is at starting location */
    if (pending_list == p) {
       pending_list = i->next;
    }
    /* if we're not at the end */
	if (i->next != NULL) {
		i->next = p->next;
	}
	
	// p is no longer an element of the pending_list
	if (p != NULL) {
		packetfree(p);
	}
}
