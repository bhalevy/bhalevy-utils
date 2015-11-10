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
	/* here you probably meant calloc instead of malloc
		 and also instead of sizeof(rtp_packet *) you should use sizeof(rtp_packet)
		 to allocate the structure and not 4 bytes (8 bytes in 64bit architecture)
	 */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); 

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;

	/* here you free the original packet which is not what you want to
		 do since you don't know if it was allocated on the stack and generally
		 copy function should not play with the input data */
	free(ptr);
	ptr = dummy;
	/* here you free the allocated packet copy which means you lost all the data */
	free(dummy);

	/* all pointers were released previously so modifying this dangling pointer
		 will lead to modification of some unknown data or segmentation fault
	*/
	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
