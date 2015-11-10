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

/* MISTAKE: 
	type "struct rtp_packet" is referenced as "rtp_packet"
	in the code that follows;
	either use "struct rtp_packet" in the code that follows (better), 
	or define a new type here, like this:
*/
typedef struct rtp_packet rtp_packet;

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	
	/* MISTAKE: 
		malloc takes 1 parameter as it is defined like this:
			void *malloc(size_t size);
		the "array" version, is called "calloc":
			void *calloc(size_t nmemb, size_t size);
	*/
	/* MISTAKE
		sizeof(rtp_packet *) will give the size of the pointer,
		not of the structure. Either sizeof(*dummy) or 
		sizeof(rtp_packet) will do
	*/
	//dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));
	
	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	/* MISTAKE
		dummy should not be freed here, as it became the "active" packet
		and ptr points to it now; The next access like ptr->sequence ++
		will result in a fault
	*/
	//free(dummy);

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}

