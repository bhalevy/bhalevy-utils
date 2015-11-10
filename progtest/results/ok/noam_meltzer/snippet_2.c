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
    /*
     * need to write "struct rtp_packet *ptr" (and every time else rtp_packet is referred to)
     */
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
    /*
     * 1. to use malloc, need to #include <stdlib.h>
     * 2. malloc syntax is wrong - malloc only takes one parameter: size_t
     * 3. sizeof will return the size of a pointer on the target architecture, not the size of struct rtp_packet.
     */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

    /*
     * this will copy the contents of the input packet. except for the payload.
     * the payload is a pointer...
     * (so what needs to be done is to allocate memory for new char* and then
     * assign it's pointer to dummy->payload
     */
	*dummy = *ptr;

    /*
     * this will free the memory of *p which might later be used somewhere
     * else in the code.
     * or, if *p wasn't dynamically allocated then this will cause a crash
     */
	free(ptr);

    /*
     * next two lines: ptr is now a pointer to the same memory as dummy
     * free()ing dummy will free ptr as well
     */
	ptr = dummy;
	free(dummy);

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
