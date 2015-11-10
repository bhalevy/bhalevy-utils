/* this is the corrected version of snippet_2.c */

#include <stdlib.h> /* to get NULL   */
#include <string.h> /* to get memcpy */

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

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	if (p == NULL)
	{
		return (NULL);
	}
	
	ptr = (rtp_packet *) p;
	dummy = (rtp_packet*) malloc (sizeof(rtp_packet));
	
	if (dummy == NULL) {
		return NULL;
	}

	memcpy(dummy, ptr, sizeof (rtp_packet));

	/* leave it for the user to decide whether to perform free(ptr) */
	
	dummy->sequence ++;
	dummy->timestamp += 160;

	return ((void*)dummy);
}
