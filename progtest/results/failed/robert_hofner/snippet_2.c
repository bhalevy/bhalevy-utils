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
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));
	/*
	 * The above is the syntax of calloc() not malloc()
	 * Beside that it only allocates a POINTER to structure and
	 * NOT the size of the STRUCTURE itself
	 * it should have been:
	 * dummy = (rtp_packet *)calloc(1, sizeof(rtp_packet));
	 */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	free(dummy);

	/*
	 * Once the structure dummy point to is freed,
	 * ptr points to an unallocated space. That means that
	 * it might be a piece of memory that might have been used
	 * for other purposes. It's not what this function was intent to do
	 */
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
