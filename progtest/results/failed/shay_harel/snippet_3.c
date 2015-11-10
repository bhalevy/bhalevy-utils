/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;/*point to the start of the linked list*/
	packet *tmp = pending_list;

	/*in this for loop we evaluate the address of the packet passed to the address of the 
	packet in the list, if they match, we go out, 

	HOWEVER, there is a bug here since we don't keep the previous packet in the list.
	We need to do so because after we go out we have to link it to the 'next' of the one we 
	are pulling out. To do so, I added another temp pointer and I keep it as we go
	see below, added 'tmp = *i'*/
	for (;(*i) != NULL && ((*i) != p); tmp = *i, *i = ((*i)->next)) {
		/* do nothing */;
	}

	/*if we removed the first element, we have to make sure pending_list points to the next one
	in line so I had to add this code here*/
	if (*i == pending_list){
		pending_list = (*i)->next;
	}

	/*over here, the itent was do keep th pointer for the next one but it was 
	done wrong, my fix is in*/
	if (*i != NULL) {
		tmp->next = (*i)->next;		
	}

	/*there is a bug here that if we passed in a completly bogus address
	which is not in the list, it will panic so we need to add a flag saying
	we found it somehow.

	Other than that, it will just delete the packet from memory since it's already out of the list
	*/
	if (p != NULL) {
		packetfree(p);
	}
}
