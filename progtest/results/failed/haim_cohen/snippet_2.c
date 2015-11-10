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

#ifdef __WITH_BUGS__
void *create_packet(void *p)
{
	// the rtp_packet is not defined as type and this
	// variable declaration will not pass compilation.
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

	// 1. this is not the malloc function signature
	//		the signature is: void *malloc(size_t size);
	//     it is more resemble to the calloc.
	// 2. the allocation size is wrong. with this code
	//    we would allocate pointer size (which is 4/8 bytes
	//    depending on the OS 32/64 bit)
	//    when we really want to allocate sizeof(struct rtp_packet)
	// 3. this one is more like enhancement
	//		it will be better to reuse the packet received
	//		from user instead of allocating and freeing.
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	// 4. this could cause access violation if ptr is null
	//    we should check before using it.
	// 5. in the case that we have allocated sizeof(rtp_packet *) which is
	//		less then sizeof(rtp_packet) by doing this assignment
	//		we also corrupted the memory that follows the memory we allocated.
	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	// 6. here we are freeing the memory dummy is pointing to which will be
	//    referenced to with ptr and also return this address to the user.
	free(dummy);

	// 7. we are accessing memory that being free ( free(dummy); )
	// 	dangling pointer.
	ptr->sequence ++;
	ptr->timestamp += 160;

	// 8. same as 7 we returning to the user address that being free.
	return ptr;
}
#else

#include <malloc.h>
typedef struct rtp_packet rtp_packet;
void *create_packet(void *p)
{
	rtp_packet *ptr=0, *dummy=0;
	if (!p)
		return 0;
	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (!dummy)
		return 0;

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;

	ptr->sequence ++;
	ptr->timestamp += 160;
	return ptr;
}

#endif

