/*
the rtp_packet struct has several problems:
1. The struct declaration does not declare a typedef - therefore we can't declare variables of this struct type
2. All 'int's should be 'unsigned int' - RTP protocol does not carry signed information in the packet (e.g. sequence number and timestamp)
	Using signed int will eventually break as sequence number and timestamp grow beyond signed int positive max size.
3. It would work on a 32 bit compiler/machine, however on a 64 bit compiler we may give us problems.
	May help lose this dependence by declaring all fields as bitfields. At least if no excessive field processing is required.

typedef struct {
	unsigned int version:2;
	unsigned int padding:1;
	unsigned int extension:1;
	unsigned int ccount:4;
	unsigned int marker:1;
	unsigned int payloadtype:7;
	unsigned int sequence:16;
	unsigned int ssrc:32;
	unsigned int timestamp:32;
	unsigned int payload_length:32;

	char *payload;
} rtp_packet;
*/

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
	Best add a check here that p is not NULL.
	p being NULL would crash the program when '*dummy = *ptr' is performed a few code lines ahead.
	You can of course trust the caller to make sure p is not NULL - which he probably will do. At least 90% of the time.
	
	if (p == NULL)
	{
		return (NULL);
	}
	
	the down side of adding this test is that the user will get NULL returned both if p is NULL and if allocation fails.
	If needed, the user may resolve this ambiguity by checking p upon return of NULL.
	*/
	
	ptr = (rtp_packet *) p;
	
	/*
		bad call to malloc:
		two parameters are passed to malloc - which only acceps a single parameter (allocated size in bytes)
		the second parameter is the size of a POINTER rather then size of rtp_packet - probably not what we expect...
		correct call would be:
		dummy = (rtp_packet*) malloc (sizeof(rtp_packet));
	*/
	
	/* allocate a new rtp_packet */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	/* NULL is not defined - need to #include <stdlib.h> */
	if (dummy == NULL) {
		return NULL;
	}

	/* copy the original rtp_packet to the newly allocated space
	the use of the '=' to copy the pointed structure is not standard C 
	It would be better to use:
	memcpy(dummy, ptr, sizeof (rtp_packet));
	*/

	*dummy = *ptr;

	/*
	free the memory space consumed by the original rtp_packet
	Now this has two caveats:
	1. Does the user expect the original area to be freed and then replaced with the new packet?
		Is the user willing to lose the original pointer in case memory allocation failed?
	2. The function's pointer parameter, p,	may not have been allocated by malloc in the calling program
	   - therefore should not be freed like this (p may be just a pointer to a larger allocated buffer that encompasses the packet).
	*/
	free(ptr);
	
	/* assuming we are willing to live with the caveats above - now ptr points to dummy */
	ptr = dummy;
	
	/*
	now we kill our hard earned *dummy, along with our new *ptr. 
	ptr now points to a freed area, most likely not what we want
	*/
	free(dummy);

	/*
	Perform the required manipulation on the new packet.
	Note: juat make sure sequence and timestamp are declared 'unsigned int' in the rtp_packet struct above, otherwise this will produce bad results.
	*/
	ptr->sequence ++;
	ptr->timestamp += 160;

	/*
	function was declared to return void* while we return rtp_packet*
	may compile OK but best use:
	return (void*)ptr;
	*/
	return ptr;
}
