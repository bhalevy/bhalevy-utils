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
	rtp_packet *ptr;// 1 mistake, should be defined as "struct rtp_packet* " rtp_packet * is not defined
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));	
	// 2 mistakes,	1 malloc accepts one size_t input variable
	//				2 sizof should be: sizeof(struct rtp_packet) - we need struckt size not ptr size
	if (dummy == NULL) {
		return NULL; //concern: caller function may expect ptr to be freed any way, mem leack risk on bad flow.
	}

	*dummy = *ptr;

	free(ptr); // error prone: p may not have been malloced - can be a static field of the calling function
	ptr = dummy;
	free(dummy);
	// 1 mistake - freed allocated memory , the next lines manipulate memory that was freed.(may get seg/page fault)

	ptr->sequence ++;		//concern: may become negative and even overflow on to the payload bit field
							//arrithmetic op on bit field - expensive in machine ops.

	ptr->timestamp += 160;	//concern: timestamp may become negative if overflows, may be problematic

	return ptr;
}
