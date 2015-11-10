struct rtp_packet {
	int version:2;
	int padding:1;
	int extension:1;
	int ccount:4;
	int marker:1;
	int payloadtype:7;
	int sequence:16;
	int ssrc;
	int timestamp;
	int payload_length;

	char *payload; // pointer to payload
};

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	/* cast the original void *p.
	   although legal, it's bad practice to use void *
	   since it's not type safe and the caller can make a mistake              
	   passing the wrong pointer type very easily.  			  
	   In addition no validity check is performed to see if p is not NULL */
	ptr = (rtp_packet *) p;


	/* tries to allocate memory for a new packet.
	   it's wrong. sizeof(rtp_packet *) is actually 4 bytes since it's the size of a pointer.
	   we are trying to allocate space for the entire packet header.
	   the correct code should be dummy = (rtp_packet *)malloc(sizeof(char), sizeof(struct rtp_packet)); */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	/* C struct copy.
	   will copy the packet header including the pointer to the payload */
	*dummy = *ptr;

	/* destroys the original packet.
	   although legal, it's bad practice since the caller needs to know that his memory has been released
	   and in the next line (ptr = dummy) replaced. can cause caller to have danging pointers. */
	free(ptr);

	/* point to the newly created packet. */
	ptr = dummy;

	/* simply a mistake. the new packet is now destroyed leaving ptr as a dangling pointer. */
	free(dummy);

	/* since sequence was defined as int, when it wrappes around it will have a negative value.
	   Usually not what you want in a sequence number.*/
	ptr->sequence ++;
	/* since timestamp was defined as int, when it wrappes around it will have a negative value.
	   Usually not what you want in a timestamp.*/
	ptr->timestamp += 160;

	/* need to casted back to (void *) */
	return ptr;
}
