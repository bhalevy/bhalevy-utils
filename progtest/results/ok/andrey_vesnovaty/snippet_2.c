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
	int payload_length

	char *payload;
};


void *create_packet(void *p)
{
	/* Andrey: no type rtp_packet - add "struct" before 
	 * 	or (better) use typedef 
	 */
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p;
	/* Andrey: once again no type rtp_packet */
	/* Andrey: malloc takes only one argument 
			remove "1, " or use calloc */
	dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *));

	if (dummy == NULL) {
		return NULL;
	}

	/* Andrey: before accessing memory pointed by input value p
	 *			1) check if it's not null first
	 *			2) do it before memory allocation
	 */
	*dummy = *ptr;

	/* Andrey: problematic block start */
	free(ptr);
	ptr = dummy;
	free(dummy);	
	/* Andrey: problematic block end */
	/* Andrey: no matter how "problematic block" should work it won't work anyway 
	 * 			since ptr == dummy && dummy is freed ptr->sequence access the memory
	 *			using invalid pointer.
	 *
	 *		Possible solution: create a copy of packet pointed by p,
	 *			modify its sequence & time stamp & returns the copy;
	 *			don't modify packet pointed by p.
	 *		
	 *			1) remove "free(ptr); ptr = dummy; free(dummy);"
	 *			2) since I don't see any reference counter for payload
	 *				memory of size payload_length should be allocated &
	 *				payload content should be copied from "ptr" to dummy 
	 *			3) function should return dummy
	 */

	/* Andrey: as far as I know bit field overflow behavior is implementation dependant
		IMHO it would be better to reserve an unsigned int for sequence numbers
	*/	
	ptr->sequence ++;
	ptr->timestamp += 160;
		
	return ptr;	/* Andrey: "3) function should return dummy" */
}
