typedef struct rtp_packet_s {
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
} rtp_packet_t;

rtp_packet_t *create_packet(rtp_packet_t *p)
{
	rtp_packet_t *dummy = NULL;

        if (p) {
            dummy = (rtp_packet_t *) malloc(sizeof(rtp_packet_t));

            if (dummy == NULL) {
                return NULL;
            }

            *dummy = *p;
            dummy->payload = (char*) malloc(p->payload_length);
            memcpy(dummy->payload, p->payload, p->payload_length);

            dummy->sequence ++;
            dummy->timestamp += 160;
        }

	return dummy;
}
