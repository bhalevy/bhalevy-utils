
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
	
	/* shimon: size of memory allocated is incorrect, need to allocate a new structure
	           and not a new pointer */
	/* dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); */
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	/* shimon: I guess original packet is not needed anymore */
	free(ptr);
	ptr = dummy;
	
	/* shimon: dummy is pointing on the same packet as ptr, and ptr is still needed */
	/* free(dummy); */

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
