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
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); // ITAI - should be (rtp_packet *)malloc(sizeof(rtp_packet))

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; //ITAI - better to do with memcpy(dummy, ptr, sizeof(rtp_packet))
	// ITAI - The payload itself is not copied, the new packet will point to the same payload memory as the original packet (might be ok, depends on the usage)

	free(ptr);	//ITAI - should not release source packet memory. delete line
	ptr = dummy;
	free(dummy);  //ITAI - should not release dest packet memory. delete line

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr; //ITAI - should be return (void*)ptr or (even better) change the function return type to rtp_packet*
}
