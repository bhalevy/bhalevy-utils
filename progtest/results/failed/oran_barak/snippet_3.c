/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

	/* iterate on list until we find p.
	   if p is NULL (*i) will be NULL */
	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	/* if (*i) is NULL (p not found or NULL) do nothing
	   otherwise, this looks like a mistake. the code will make the iterator go forward one more step.
	   this does nothing to the list and will cause the next step (packetfree) to destroy a linked node.
	   probably what was meant here was this
	   if (*i != NULL) {
		(**i) = (**i)->next;
	} 
	this is an old trick for deleting a node from the middle of a linked list without having to manipulte 
	the pointer of the previous node. 
	this code will cause a full copy of the next node to the current node. 
	if the linked list looked like this A->B->C->D 
	and we are deleting B than the list will look like this after this code A->C->D 
	since B was overwritten with the contents of C. 
	The original C is now dangling but will be destroyed with packetfree. 
	Note that this trick cannot be used for the last element in the list since there is content to NULL.
	Nor can you simply delete the last node as the previous node->next will be left dangling. 
	In fact, the only way to use this trick with the last node is to use a dummy last node. 
	    																								 */
	if (*i != NULL) {
		(*i) = (*i)->next;
	}


	/* if p is NULL do nothing.
	   else probably just perform free() on the packet mem */
	if (p != NULL) {
		packetfree(p);
	}
}
