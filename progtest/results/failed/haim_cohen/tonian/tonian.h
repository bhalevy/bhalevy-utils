/*
 * tonian.h
 *
 *  Created on: Jan 5, 2012
 *      Author: haim
 */

#ifndef TONIAN_H_
#define TONIAN_H_

typedef struct packet packet;
struct packet
{
	int val;
	packet *next;
};

extern void *create_packet(void *p);
extern void remove_from_pending_list(packet *p);
packet *add_to_list(int a);
extern void dump_list();

#endif /* TONIAN_H_ */
