
// if is used as-is to create the RTP packet sent, may be not portable between compilers
// (bit packing order is undefined)
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
// assuming C++ code (otherwise should be prepended with 'struct')
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
// 1. malloc sizeof(rtp_packet *), should be sizeof(rtp_packet)
// 2. POSIX malloc(size_t size) accepts one argument - this malloc
//    is non-standard (or error)
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}
    
// ptr may be NULL (not checked)
	*dummy = *ptr;

// assumes ptr was malloc'ed - which may or may not be right in that framework
// (if packet p was created with create_packet, then this is double free)
	free(ptr);
	ptr = dummy;
	free(dummy);

// ptr points to freed memory
	ptr->sequence ++;
// integer overflow possible (also in sequence number) - not sure if a problem
	ptr->timestamp += 160;

	return ptr;
}
