typedef struct rtp_packet {
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
} rtp_packet; // added typedef for simplification

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;
	
	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(1 * sizeof(rtp_packet)); // fixed parameters, we want to hold a structure, not a pointer

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	// free(dummy); // removed, this frees the memory we're using

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}