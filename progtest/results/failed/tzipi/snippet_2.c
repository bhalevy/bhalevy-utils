struct rtp_packet {
	/* Tzipi: assigning default value to a field in a struct is an invalid syntax, 
		and anyway, using ":2" for assigning is illegal as well */

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
	/* Tzipi: rtp_packet is a struct label. in order to use it as a pointer, 
		either use typedef on the struct definition, or use (struct rtp_packet *) */
	
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); 
	/* Tzipi: 1. malloc gets only one parameter - size_t size 
			  2. the sizeof call should be called with "struct rtp_packet" 
			  - that's the actual size we want to malloc - only pointer size is wrong
			  3. same issue with "rtp_packet*" as before (no typedef) */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; /* Tzipi: derefencing is wrong on l side and r side. */

	free(ptr); /* Tzipi: what happens if p is NULL? */
	ptr = dummy;
	free(dummy); /* Tzipi: ptr is now a dangling pointer - the memory malloced and pointed by dummy is now free */

	/* Tzipi: ptr is pointing to invalid memory - derefencing can crash the program.*/
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr; /* Tzipi: as said - ptr is a dangling pointer and therefore invalid memory. 
			returning it to calling function might lead to inconsistence behavior in the calling function.
}
