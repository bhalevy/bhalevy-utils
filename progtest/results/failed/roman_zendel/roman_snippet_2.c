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
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));
    /* RZ
     * Miistakes in malloc(1, sizeof(rtp_packet *))
     * it shuuld allocate actual memory enougf for packet and not for pointer to packet
     * should be  dummy = (rtp_packet *)calloc(1, sizeof(rtp_packet ))
     * or dummy =(rtp_packet *)malloc(sizeof(rtp_packet ));
     */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	free(ptr);
	ptr = dummy;
	free(dummy);
    /* RZ
     * Miistakes 
     * bith dummy and ptr  packets are freed , future access to the packet's
     * memory may cause GPF
     * Also the was no requirement to delete source packet, otherwize better change
     * source packet itself
     */

	ptr->sequence ++;
	ptr->timestamp += 160;
    /* RZ
     * Mistake - ptr is the pointer to the source packet
     * this code should make changes in dummy instaed, 
	dummy->sequence ++;
	dummy->timestamp += 160;
    */

    return ptr;
    /* RZ
     * Mistake - ptr is the pointer to the source packet, 
     * function should return dummy
     * reurn dummy
     */
}
