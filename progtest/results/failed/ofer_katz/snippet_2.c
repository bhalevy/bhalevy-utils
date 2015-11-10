/* typedef */struct rtp_packet {
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
} /* rtp_packet */;

void *create_packet(void *p)
{
	rtp_packet *ptr; /* sruct rtp_packet *ptr or rtp_packet needs to be defined via typedef */
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	/* dummy = (rtp_packet *)malloc( sizeof(rtp_packet) );
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));/* 2 problems in this line:
															 malloc is called wrongly like calloc. 
														     Allocation is for a pointer to rtp_packet and not the structure itself */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; /* payload data needs to be copied explicitly. A pointer assignment will not work in this case */

	free(ptr); /* payload needs to be freed before its container is deleted. It is unsafe to assume that the the input parameter was allocated via malloc */
	ptr = dummy;
	free(dummy);
	/* At this point we have no memory allocated for ptr.
	   The code below is causing a memory overrun. At best it is writing to available memory in the heap*/
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
