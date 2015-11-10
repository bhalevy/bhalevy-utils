
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

	ptr = (struct rtp_packet *) p;
	dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;   // copy packet contents

//	free(ptr);      it'll be better not to free the packet which is not allocated here
	ptr = dummy;
//	free(dummy);   if we release dummy, the packet copy will be released

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
