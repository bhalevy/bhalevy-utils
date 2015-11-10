// the function only works correctly if trying to remove first element
// of the list.
// otherwise it will set pending_list to be the next element after the searched one
// - that is remove all elements before p in addition to p.
// in the particular case of removing the last element, pending_list will be set to NULL

/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
	packet **i = &pending_list;

// this changes pending_list - (*i) - to NULL or p
	for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
		/* do nothing */;
	}

	if (*i != NULL) {
// if p last element, this changes pending_list to NULL
// otherwise, this change pending_list to be next packet after p
		(*i) = (*i)->next;
	}

	if (p != NULL) {
		packetfree(p);
	}
}


// BH: did not provide corrected  code