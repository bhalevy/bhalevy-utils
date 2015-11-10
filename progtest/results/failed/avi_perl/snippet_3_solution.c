/* fixed version of snippet_3.c */

/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */

void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list; 
	
	/* scan the list for p (or list end) */
	for (; ((*i) != NULL) && ((*i) != p) ; i = &((*i)->next))
	{
		/* do nothing */
	}
	
	/* if p was indeed found - remove it from the list by skipping it*/
	if (*i != NULL) 
	{
		(*i) = p->next; /* the next pointer pointing to p now points to the next element in the list 
						(or NULL if p is last in list) 
						For the sake of clarity, I prefer use of p->next over (*i)->next 
						*/
	}
		
	/* 
		remove p from memory - even if p was not found
	 	If packetfree() uses regular free(), this call will requires that p be a valid pointer to an element allocated by malloc
		p is always freed even if it wasn't found on the list
		it may be better leave it to the caller to delete p -or- to free p only if it was found.
	*/
	if (p != NULL) {
		packetfree(p);
	}
	
}


