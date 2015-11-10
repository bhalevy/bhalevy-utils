/*
 * main.c
 *
 *  Created on: Jan 5, 2012
 *      Author: haim
 */
#include "tonian.h"
#include "sg_copy.h"
#include "stdio.h"
#include "malloc.h"

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

static void test_sg_copy()
{
	char msg1[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int msg1_len=sizeof(msg1)-1;
	char msg2[]="abcdefghijk";
	int msg2_len=sizeof(msg2)-1;
	int copied;

	sg_entry_t *h1 = sg_map(msg1, msg1_len);
	sg_entry_t *h2 = sg_map(msg2, msg2_len);

	SG_DUMP(h1);
	SG_DUMP(h2);

	copied = sg_copy(h1, h2, 3, 5);
	printf("\n\nsg_copy(h1, h2, 3, 5); copied=%d\n", copied);

	SG_DUMP(h1);
	SG_DUMP(h2);

	copied = sg_copy(h1, h2, 3, msg1_len);
	printf("\n\nsg_copy(h1, h2, 3, msg1_len); copied=%d\n", copied);

	SG_DUMP(h1);
	SG_DUMP(h2);

	copied = sg_copy(h2, h1, 3, msg1_len);
	printf("\n\nsg_copy(h2, h1, 3, msg1_len); copied=%d\n", copied);

	SG_DUMP(h1);
	SG_DUMP(h2);


	sg_destroy(h1);
	sg_destroy(h2);
}

static void test_snippet_2()
{
	struct rtp_packet *p = malloc(sizeof(struct rtp_packet));
	p->timestamp = 1;
	p->sequence = 1;
	p = create_packet(p);
	if (p->timestamp != 161)
		printf("ERROR: p->timestamp != 161\n");
	if (p->sequence != 2)
			printf("ERROR: p->sequence != 2\n");
	free(p);
}

static void test_snippet_3()
{
	int i;
	packet *p[15] = {0};
	int p_len = sizeof(p)/sizeof(packet *);

	for (i=0; i<p_len; i++)
		p[i] = add_to_list(i);

	dump_list();
	remove_from_pending_list(p[0]);
	remove_from_pending_list(p[6]);
	remove_from_pending_list(p[14]);
	remove_from_pending_list(p[1]);

	dump_list();

	for (i=0; i<p_len; i++)
		remove_from_pending_list(p[i]);
	dump_list();
}

int main()
{
	test_sg_copy();
	test_snippet_2();
	test_snippet_3();
	return 0;
}
