struct rtp_packet {
	int version:2;
	int padding:1;
	int extension:1;
	int ccount:4;
	int marker:1;
	int payloadtype:7;
	int sequence:16;
	//timestamp should be first, then ssrc
	int ssrc;
	int timestamp;
	int payload_length;

	char *payload;
};//missing __attribute__ ((__packed__))

//the code assumes int is 32 bit, which might be untrue for some of the systems.
//should use u32 which is defined per system.


void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	//malloc gets only one parameter - size to allocate in bytes
	//also - should be sizeof(rtp_packet) 
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	//not sure ptr was allocated with malloc, and not sure it is this function responsibilty to free it anyway.
	free(ptr);
	ptr = dummy;
	free(dummy);

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
