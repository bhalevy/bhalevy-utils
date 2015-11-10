/*
 * remove_from_pending_list: removes the pkt from the list when
 * it is no longer needed. pending_list is a global variable.
 */
void remove_from_pending_list(packet *p)
{
   if (p != NULL) {
      packet *i = pending_list;

      for (;i != NULL && (i->next != p); i = i->next) {
         /* do nothing */;
      }

      if (i != NULL) {
         i->next = p->next;
         packetfree(p);
      }
   }

}
