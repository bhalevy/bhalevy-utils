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
	struct rtp_packet *ptr;
	struct rtp_packet *dummy;

	if (p == NULL){
		return NULL;
	}
	ptr = (struct rtp_packet *) p;
	dummy = (struct rtp_packet *)malloc(sizeof(*ptr));
	
	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	/* never just increment , overflown integer value is not defined in c specifications */
	dummy->sequence = ( ++dummy->sequence ) & 65535;
	dummy->timestamp += 160;

	/* free(ptr);  i do not see in question thar src is to be freed  */
	return dummy;
}
