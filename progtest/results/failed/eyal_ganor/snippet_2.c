struct rtp_packet { 
	int version:2,
	    padding:1, // Removed the "int" in each line and add , instead of ;
	    extension:1,
	    ccount:4,
	    marker:1,
	    payloadtype:7,
	    sequence:16;
	int ssrc;
	int timestamp;
	int payload_length;

	char *payload;
};

void *create_packet(void *p)
{
	struct rtp_packet *ptr; /* Add struct */
	struct rtp_packet *dummy;  /* Add struct */

	ptr = (struct rtp_packet *) p;  /* Add struct */
	dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet)); /* Remove 1, and sizeof should be struct rtp_packet and not pointer of it */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; 

	free(ptr);
	ptr = dummy;  // Note that in this case - payload will stay the same as the original. If you want a new payload - a new payload should be malloced and the old one should be freed - but i'm not sure from the question if that was the purpose */
/*	free(dummy); -> should be removed */

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
