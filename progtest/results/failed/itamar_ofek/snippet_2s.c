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
	struct rtp_packet *ptr = NULL; /* missing struct keyword */
	struct rtp_packet *dummy = NULL;
	if (p == NULL) { /* check should be before allocating memory*/
      return NULL;
	}

	ptr = (struct rtp_packet *) p;
	dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet)); /* malloc should take size of struct*/
    if (dummy != NULL)
    {    

        *dummy = *ptr;

        ptr = dummy;
        /* free(dummy);  ptr and dummy now point to same memory freeing dummy free also the returned rtp_packet pointed by ptr */


        ptr->sequence++;
        ptr->timestamp += 160;
    }
    free (p); /* p must be free'd */
	return ptr;
}


