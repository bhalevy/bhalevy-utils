#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
typedef struct rtp_packet rtp_packet;

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

        /* Itay: need to check p is not NULL, can not copy from NULL */
        if (!p) {
            return NULL;
        }

	ptr = (rtp_packet *) p;
	/* dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); 
         * Itay: wronng. should be: */
        dummy = (rtp_packet *)malloc(sizeof(rtp_packet)); 

	if (dummy == NULL) {
		return NULL;
	}

	/* *dummy = *ptr; 
         * Itay: wrong. should be: */
        memcpy(dummy, ptr, sizeof(rtp_packet));
        dummy->payload = (char *)malloc(ptr->payload_length);
        if (! dummy->payload) {
                free(dummy);
                return NULL;
        }
        memcpy(dummy->payload, ptr->payload, ptr->payload_length);

        /* Itay: wrong. not supposed to free the input 
	free(ptr);
	ptr = dummy;
	free(dummy);
        should be: */
        ptr = dummy;

        /* Itay: missing overload checks below */
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}

void print_packet(const rtp_packet * p)
{
        if (!p) {
            printf("p is NULL\n");
            return;
        }
        printf("version = %d, ccount = %d, sequence = %d, timestamp = %d, payload = %s\n",
               p->version, p->ccount, p->sequence, p->timestamp, p->payload);
}

#define PAYLOAD "12345"
#define MAX_PAYLOAD_LEN (256)

int main()
{
    rtp_packet p1;
    rtp_packet *p2;
    memset(&p1, 0, sizeof(rtp_packet));
    p1.version = 1;
    p1.ccount = 3;
    p1.sequence = 7;
    p1.timestamp = 100;
    p1.payload = PAYLOAD;
    p1.payload_length = strnlen(PAYLOAD, MAX_PAYLOAD_LEN);

    print_packet(&p1);

    p2 = create_packet(&p1);

    print_packet(p2);

    free(p2->payload);
    free(p2);

    return EXIT_SUCCESS;
}
