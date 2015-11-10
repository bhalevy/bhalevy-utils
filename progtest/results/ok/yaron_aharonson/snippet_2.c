typedef struct {        // so it could be used in the way the function uses it
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
} rtp_packet;           // modified

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

    /* NOTE 1! standard C runtime library defines malloc as:
     * void * malloc(size_t size)
     * which means that the first parameter should be ommited. however, in case the
     * the snippet uses a special version of malloc it should be left as is.
     * NOTE 2! sizeof part should be sizeof(rtp_packet) (in our modified file), or
     * sizeof(struct rtp_packet) in the original file
     */
    //dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	//free(dummy); we can't free dummy - the referenced data is the function's output

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
