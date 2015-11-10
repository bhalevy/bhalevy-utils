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
    rtp_packet *ptr; //need to add struct keyword----- struct rtp_packet *ptr;
    rtp_packet *dummy; // struct rtp_packet *dummy;

    ptr = (rtp_packet *) p; // ptr = (struct rtp_packet *) p;
    dummy = (rtp_packet *)malloc(1, sizeof(rtp_packet *)); //1. malloc only take one varible (calloc takes 2)
                                                           //2. there should be no * (otherwise the current allocated size will be size_t instead of the struct size)
                                                           //3. need to add struct keyword inside the sizeof, the correct line is:
                                                           // dummy = (struct rtp_packet *)malloc(sizeof(struct rtp_packet));

    if (dummy == NULL) {
        return NULL;
    }

    *dummy = *ptr; 

    free(ptr); //this will free the original packet, in fact there is no need to use free anywhere in this functions
               //because we are going to return the duplicated packet as well
    ptr = dummy; //we don't really need this, we can use the dummy variable as well
    free(dummy); //this line will cause the next line to seg fault because ptr and dummy are pointing to the same memory

    ptr->sequence ++; //this will seg fault because of the previous free
    ptr->timestamp += 160;

    return ptr;
}
