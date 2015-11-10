/*
 * (1) struct should be defined as typdef struct rtp_packet for future use
 * (2) bit-fields should be defined as unsigned
 */
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
}; /* Should be "} rtp_packet;" */


/*
 * The function should be of type rtp_packet * insted of void *
 */
void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

	/*
	 * Two mistakes here.
	 * (1) Wrong usage of malloc. Should be: void *malloc(size_t size);
	 * (2) Wrong size in malloc.
	 * The correct statement should be:
	 * dummy = (rtp_packet *)malloc(sizeof(rtp_packet));
	 */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	/*
	 * I assume that the meaning of this statement was to copy content of
	 * ptr to dummy. So the correct statement should be:
	 * memcpy(dummy,ptr,sizeof(rtp_packet);
	 */
	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
// BH: NOOO never free dummy
	free(dummy);

	/*
	 * Usage of ptr after free
	 */
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
