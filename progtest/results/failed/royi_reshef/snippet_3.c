/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
    packet **i = &pending_list;
    //we need to stop in the element before p in order to change its next variable
    //this function stops when p variable is found
    for (;(*i) != NULL && ((*i) != p); *i = ((*i)->next)) {
        /* do nothing */;
    }

    if (*i != NULL) {
        //instead of changing the the correct pointers this will change nothing
        (*i) = (*i)->next;
    }

    if (p != NULL) {
        packetfree(p);
    }
}

void correct_remove_from_pending_list(packet *p)
{
    packet **i = &pending_list;

    if (!p || !(*i)) {
        return;
    }
    
    for (;((*i)->next != NULL) && ((*i)->next != p); *i = ((*i)->next)) {
        /* do nothing */;
    }

    if ((*i)->next != NULL) {
        (*i)->next = (*i)->next->next;
    }
    
    packetfree(p);
}
