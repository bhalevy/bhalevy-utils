
#include <stdio.h>
#include <stdlib.h>

typedef struct rtp_packet {
	unsigned int version:2;
	unsigned int padding:1;
	unsigned int extension:1;
	unsigned int ccount:4;
	unsigned int marker:1;
	unsigned int payloadtype:7;
	unsigned int sequence:16;
	unsigned int ssrc;
	unsigned int timestamp;
	unsigned int payload_length;

	char *payload;
} rtp_packet_t;

void *create_packet(void *p)
{
	rtp_packet_t *ptr;
	rtp_packet_t *dummy;

	ptr = (rtp_packet_t *)p;
	dummy = (rtp_packet_t *)malloc(sizeof(rtp_packet_t *));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	//free(ptr);
	ptr = dummy;
	//free(dummy);

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}


void print_packet(rtp_packet_t *p, char *name) {
  printf("print_packet %s: %u: %u: %u: %u: %u: %u: %u: %u: %u: %u:%s:\n",name,p->version,p->padding,p->extension,p->ccount,p->marker,p->payloadtype,p->sequence,p->ssrc,p->timestamp,p->payload_length,p->payload);
  return;
}

int main(int argc, char **argv) {

  rtp_packet_t *y;
  rtp_packet_t x = {2,1,0,6,1,5,145,123,162,7,"Adoram"};
  
  print_packet(&x,"x");
  y = create_packet(&x);

  print_packet(y,"y");

  return 0;
}
