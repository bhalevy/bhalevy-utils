#include <stdio.h>

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
/*	rtp_packet *ptr;
	rtp_packet *dummy;
*/
	struct rtp_packet *ptr;
	struct rtp_packet *dummy;

/*	ptr = (rtp_packet *) p;
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));
*/
	ptr = (struct rtp_packet *) p;
	dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet)); // Using Correct malloc syntax, the size should be of the structure, not its pointer

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
/*	free(dummy); Cannot free this memory, ptr is pointing to it */

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
