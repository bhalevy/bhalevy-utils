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
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); <-- This would not compile, I guess "calloc" was meant instead of "malloc"

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; <- I'd use a memcpy() here but I guess compiler will use it anyway... ;)

	free(ptr); <- It's not safe - what if the original packet (p) was not allocated separately and is a part of big buffer that was allocated and then cut into packets.
	           <- In any case it end good: for instance if the packet that starts from the beginning of such buffer is given to this function
	           <- it will free the memory of all other packets...
	           <- What is "p" was not even dynamicaly allocated?
	ptr = dummy;
	free(dummy); <- Hmmm... We are freeing the allocated above memory here. All folloing is going to access the illegal virtual address.

	ptr->sequence ++; <- Starting from here all access to the address ptr points at and the one returned from the function will garbadge the host memory and
	                  <- most probably will cause a memory corruption.
	ptr->timestamp += 160;

	return ptr;
}
