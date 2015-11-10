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

    /* 
     * @IU: Possible alignment problem:
     * p is a void *, so it may be not aligned to integer boundary.
     * ptr must be aligned at an integer boundary on some platforms.
     * The casting will align the p (i.e. (void*)ptr may be != p)
     */
	ptr = (rtp_packet *) p;
    /*
     * @IU: Is it a calloc with a typo or a malloc with an extra argument?
     * calloc is an overkill here anyway since we'll assign the value right away,
     * so no point zeroing out the memory.
     */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

    /*
     * @IU: What about payload?
     * If all we are doing is playing with the header, it may be ok though.
     */
	*dummy = *ptr;

    /*
     * @IU: Because of possible misalignment (see above), free may either fail
     * or leave several bytes behind, depending on implementation of the
     * heap allocator.
     */
	free(ptr);
	ptr = dummy;

    /*
     * @IU: What? Now ptr will be a dangling pointer.
     */
	free(dummy);

    /*
     * @IU: Here we have a possible memory corruption because of dereferencing 
     * of a dangling pointer
     */
	ptr->sequence ++;
	ptr->timestamp += 160;

    /*
     * @IU: Here we are returning a dangling pointer to the caller.
     * We are asking for trouble here.
     */
	return ptr;
}

