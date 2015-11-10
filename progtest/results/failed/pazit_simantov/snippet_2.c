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

	/* first - it's not malloc signature (but calloc) - it's:
	   "void* malloc(size_t) and not as it's written..
	   unless the writer override "malloc", and then my next comment is not correct
	   second - mistake - malloc rtp_packet pointer instead of rtp_packet,
	   so will get rtp_packet** and you cast it to rtp_packet*... not good... */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	/* secondly (assuming malloc is written as it should)
	   you can't copy pointers by asignment. you need to use memcpy\bcopy */
	this way it copy the first 2 bits (version field)
// BH: why?
	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	free(dummy);
	/* if the writer didn't override free - now we lost both addresses. since you free dummy - you free prt as well
	   so we shouldn't update any of its fields
	   it's memory smear and might cause segmentation fault */

	/* ++ operator can by used on int\long\short.. but not 16bits field (although 16 bits is short, so I'm not sure,
	   it may work... */
// BH: why?
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
