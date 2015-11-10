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
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	free(dummy); 

	ptr->sequence ++; 
	ptr->timestamp += 160; 

	return ptr; 
}

/*
 * The above code has the following errors:
 * 
 * 1. line 18: rtp_packet is a structure, we must declare it as such 
 * 2. line 19: same as #1
 * 3. line 21:  cast is not needed but if used the same error as #1
 * 4. line 22:
 * 		a. malloc doesn't take 2 arguments - void *malloc(size_t size)
 * 		b. a structure allocation is needed, not a ptr (e.g malloc(sizeof(*dummy)))
 * 		c. cast is not needed but if used the same error as #1
 * 5. line 28: crash if p == NULL, not necessarily a bug - depends on function usage
 * 6. line 30: possible crash - p might not be heap alloced
 * 7. line 32: freeing the memory we just allocated and filled
 * 8. line 34: writting on freed memory - big no no - might crash if different thread alloced this region
 * 9. line 35: same as #8
 * 10 line 37: returning a ptr to freed memory
 */
