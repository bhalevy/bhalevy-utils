typedef struct rtp_packet {
	unsigned int version:2;
	unsigned int padding:1;
	unsigned int extension:1;
	unsigned int ccount:4;
	unsigned int marker:1;
	unsigned int payloadtype:7;
	unsigned int sequence:16;
	unsigned int timestamp;
	unsigned int ssrc;
	unsigned int payload_length;

	char *payload;
} rtp_packet;

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
