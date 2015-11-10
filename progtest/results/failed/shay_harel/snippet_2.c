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

/*to start with, the function should take and return rtp_packet pointer, this will make sure we do type checking*/
void *create_packet(void *p)
{
	/*non initialized locals*/
	rtp_packet *ptr = NULL;
	rtp_packet *dummy = NULL;

	ptr = (rtp_packet *) p;

	/*fixed malloc syntax. Any chance the user wanted to call 'calloc' to allocate physically contig. memory ?*/
	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr;/*ok and legal but a cleaner interface will be memcpy*/

	/*Freeing here is very bad design, but it will work. What if someone sent you a pointer from
	a memory that is allocated on the stack ????*/
	free(ptr); 
	ptr = dummy;
	/*free(dummy);*//*if you do that, you lost the memory ptr now points to, so I removed it*/

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}