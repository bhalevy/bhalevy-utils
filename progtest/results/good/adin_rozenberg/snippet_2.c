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

	char *payload;
};

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); /* should be malloc(sizeof(rtp_packet)) */
	                                                       /* malloc() accepts a single paramter (unlike calloc()), 
	                                                          plus we want to allocate the size of the struct, not a pointer */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; /* payload is neither allocated nor copied, so both packets point to the same payload.
	                  A function for freeing a packet would probably free the payload as well, invalidating
	                  the other packet's payload. */

	free(ptr); /* not necessarily a mistake, but a little odd - 
	              if the point is to *replace* the given packet,
	              one can manipluate its field directly, rather
	              than allocate a new one and free the old one */
	ptr = dummy;
	free(dummy); /* this is definitely wrong! */

    /* potential overflow (especially sequence, since it is 16 bits wide), but this may be acceptable */
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
