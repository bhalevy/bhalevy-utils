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
/* The type is 'struct rtp_packet' and not 'rtp_packet' -- should be changed through all the function */

	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;

/* The function 'malloc' receives only one parameter -- number of bytes to allocate
   sizeof(struct rtp_packet *) is the size of pointer -- 4 bytes on 32 bits system.
   The line should be changed to 'dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet));'
 */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

/* It is safer to use 'memcpy(dummy, ptr, sizeof(struct rtp_packet))' in this case */
	*dummy = *ptr;

/* The follownig line will free the memory of the original packet that
   was provided to the function -- shouldn't be done by "create" function.
 */
	free(ptr);

	ptr = dummy;
/* At this step 'ptr' and 'dummy' point to the same memory
   After the call to free both pointers will point to an unallocated memory
 */
	free(dummy);

/* Access to an unallocated memory */
	ptr->sequence ++;
/* Access to an unallocated memory */
	ptr->timestamp += 160;

	return ptr;
}
