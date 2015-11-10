#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
} rtp_packet;

rtp_packet *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	memcpy(dummy, ptr, sizeof(rtp_packet));

	dummy->sequence ++;
	dummy->timestamp += 160;

	return dummy;
}

void print_packet(rtp_packet *packet)
{
	printf("Dump\n====\n");
	printf("version: 0x%x\n", packet->version);
	printf("padding: 0x%x\n", packet->padding);
	printf("extension: 0x%x\n", packet->extension);
	printf("ccount: 0x%x\n", packet->ccount);
	printf("marker: 0x%x\n", packet->marker);
	printf("payloadtype: 0x%x\n", packet->payloadtype);
	printf("sequence: 0x%x\n", packet->sequence);
	printf("ssrc: 0x%x\n", packet->ssrc);
	printf("timestamp: 0x%x\n", packet->timestamp);
	printf("payload_length: 0x%x\n\n", packet->payload_length);
}

int main(int argc, char* argv)
{

	rtp_packet *first_packet, *second_packet;

	first_packet = (rtp_packet *)calloc(0,sizeof(rtp_packet));

	first_packet->version = 0x1;
	first_packet->padding = 0x1;
	first_packet->extension = 0x1;
	first_packet->ccount = 0x1;
	first_packet->marker = 0x1;
	first_packet->payloadtype = 0x1;
	first_packet->sequence = 0x1;
	first_packet->ssrc = 0x1;
	first_packet->timestamp = 0x1;
	first_packet->payload_length = 0x1;

	second_packet = create_packet(first_packet);
	printf("First packet\n");
	print_packet(first_packet);
	printf("Second packet\n");
	print_packet(second_packet);
}
