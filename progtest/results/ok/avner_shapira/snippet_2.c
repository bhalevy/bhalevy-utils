/* ok. this is not a raw RTP packet struct:
 * - real RTP header definition should not assume BIG ENDIAN (host order is net order) but #ifdef it.
 * - real RTP header does not contain length member - it is derived from the UDP header.
 * - real RTP header should allow ccount > 0, in which case the payload does not follow the base header immediately but is found at (char*)hdr + sizeof(hdr) + hdr->ccount * sizeof(struct csrc_hdr).
 * - even if we know that ccount is 0, payload should not occupy a pointer space, but be defined as char payload[0], and the actual malloc should be for sizeof(hdr) + payload_length.
 * BUT it may be a data struct that stores the data _from_ the RTP packet after processing it. if that's the idea, everything is ok here of course.
 */
struct rtp_packet {
	/* ERROR: bitfield members must be unsigned, if you want sane behavior ahead. fixed */
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
};
/* COMPILATION BUG: added as per the usage below */
typedef struct rtp_packet rtp_packet;

void *create_packet(void *p)
{
	rtp_packet *ptr;
	rtp_packet *dummy;

	ptr = (rtp_packet *) p; /* IMHO casting is not needed, but does not harm */
	dummy = (rtp_packet *)calloc(1, sizeof(rtp_packet *)); /* COMPILATION BUG: fixed malloc to calloc due to prototype. however it's redundant since you immediately initialize all fields */

	if (dummy == NULL) {
		return NULL;
	}

	*dummy = *ptr; /* NOTE: this is a shallow copy - payload is shared. that's not so bad since you immediately free the original packet, so payload just passed to the new one without the need to memcpy it */

	free(ptr);
	ptr = dummy;
	free(dummy); /* FATAL: you free your data and then use it _and_ return a pointer to it as a result. immediate segfault if you are lucky, but most probably you'll waste hours in order to find where you did this... */

	ptr->sequence ++;
	ptr->timestamp += 160; /* I assume you know what you're doing here, and that the packets are indeed that long exactly (e.g. 20ms audio) */

	return ptr;
}
