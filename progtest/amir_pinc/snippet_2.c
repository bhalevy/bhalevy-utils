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
	rtp_packet *ptr;	// [AP] struct rtp_packet *ptr;
	rtp_packet *dummy;	// [AP] struct rtp_packet *dummy;

	ptr = (rtp_packet *) p;	// [AP] ptr = (struct rtp_packet *) p;
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); // [AP] dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	// [AP] Need to verify p\ptr is not NULL { if (ptr == NULL) return NULL; }
	*dummy = *ptr;

	free(ptr);	// [AP] Do we want to free user pointer?! What if on stack?
	ptr = dummy;
	free(dummy);	// [AP] We must not free dummy because we wish to edit it in the following lines and return it to caller

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
