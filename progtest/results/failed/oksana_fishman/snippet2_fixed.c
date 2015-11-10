void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

	dummy = (rtp_packet *)malloc(sizeof(rtp_packet));
	
	if (dummy == NULL) {
		return NULL;
	}
	
	/*copy the packet*/
	memcpy(dummy,ptr,sizeof(rtp_packet));
	/*allocate payload*/
	dummy->payload=(char*)malloc((ptr->payload_length)*sizeof(char));
	/*copy payload*/
	memcpy(dummy->payload,ptr->payload,ptr->payload_length*sizeof(char));

	free(ptr->payload);
	free(ptr);

	ptr = dummy;

	ptr->sequence ++;
	ptr->timestamp += 160;

	return ptr;
}
