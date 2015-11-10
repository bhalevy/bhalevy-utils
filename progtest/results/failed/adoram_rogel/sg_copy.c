
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

// comes from ex1, for debugging
// BH
// extern int debug_flag;
int debug_flag;

#define min(a, b) ((a) <= (b) ? (a) : (b))

/*
 * sg_map        Map a memory buffer using a scatter-gather list
 *
 * @in buf       Pointer to buffer
 * @in length    Buffer length in bytes
 *
 * @ret          A list of sg_entry elements mapping the input buffer
 *
 * @note         Make a scatter-gather list mapping a buffer into
 *               a list of chunks mapping up to PAGE_SIZE bytes each.
 *               All entries except the first one must be aligned on a
 *               PAGE_SIZE address;
 */
sg_entry_t *sg_map(void *buf, int length) {
  sg_entry_t *head, *cur, *newb;
  int blocks_malloced = 0;

  if ((buf == NULL) || (length <= 0))
	  return NULL;

  head = (sg_entry_t *)malloc(sizeof(sg_entry_t));
  if (head == NULL) {
	fprintf(stderr,"*** Error: malloc failed for head in sg_map");
	return NULL;
  }
  blocks_malloced++;

  // The head block does not start on the page boundary
// BH: did not use ptr_to_phys
//  head->paddr = (physaddr_t)buf;
head->paddr = ptr_to_phys(buf);

  // This is the head's page beginning, not where our data starts
//  physaddr_t head_block_begins = (physaddr_t)phys_to_ptr((physaddr_t)buf);
  // This is the beginning of the next block/
//  physaddr_t next_block_begins = head_block_begins + PAGE_SIZE;
char *head_block_begins = buf;
char *next_block_begins = (void *)(((unsigned long)buf + PAGE_SIZE) & ~(PAGE_SIZE - 1));
  
  head->count = (next_block_begins - head_block_begins);
if (head->count > length) head->count = length;
head->next = NULL;

  length -= head->count;

  cur = head;

  while (length > 0) {
	newb = (sg_entry_t *)malloc(sizeof(sg_entry_t));
	if (newb == NULL) {
	  fprintf(stderr,"*** Error: malloc failed for newb in sg_map after %d successful block mallocs",blocks_malloced);
	  return NULL;
	}
	blocks_malloced++;
	// Link the new record to the linked list and advance into the new record
	cur->next = newb;
	cur = newb;

//	cur->paddr = next_block_begins;
	cur->paddr = ptr_to_phys(next_block_begins);
	cur->next = NULL; // init it, this might be the last record
	if (length >= PAGE_SIZE) {
	  cur->count = PAGE_SIZE;
	  length -= PAGE_SIZE;
	}
	else {
	  cur->count = length;
	  length = 0;
	}
	next_block_begins += PAGE_SIZE;
  }

  return head;
}


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list) {
  sg_entry_t *p, *q;

  if (sg_list == NULL)
	return;

  p = sg_list; // first element
  q = p->next; // second element or NULL, if only one element

  while (q != NULL) {
	p = q;
	q = q->next;
	free(p);
  }

  // release the first element
  if (sg_list != NULL)
	free(sg_list);

  return;
}


/*
 * sg_copy       Copy bytes using scatter-gather lists
 *
 * @in src       Source sg list
 * @in dest      Destination sg list
 * @in src_offset Offset into source
 * @in count     Number of bytes to copy
 *
 * @ret          Actual number of bytes copied
 *
 * @note         The function copies "count" bytes from "src",
 *               starting from "src_offset" into the beginning of "dest".
 *               The scatter gather list can be of arbitrary length so it is
 *               possible that fewer bytes can be copied.
 *               The function returns the actual number of bytes copied
 *
 * @ASSUME       As the spec does not specify what to do if the destination
 *               block is too small to hold the copied source, I'm assuming
 *               it's large enough.
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count) {
  sg_entry_t *s, *d;
  int bytes_copied = 0, s_ofst, d_ofst;
  int skipped = 0;

  if ((src == NULL) || (dest == NULL) || (src_offset < 0) || (count <= 0))
	return 0;

  d = dest;
  d_ofst = 0;

  // skip src_offset bytes into src
  for (s = src; (s && (s != NULL)); s = s->next) {
	skipped++;
	if (s->count > src_offset)
	  break; // we're reached our desired record
	else
	  src_offset -= s->count;
  }
  s_ofst = src_offset;

  if (debug_flag) {
	fprintf(stderr,"skip src skipped %d records in the src list and points at offset %d\n",skipped,s_ofst);
  }

  // got the exact location in s + s_ofst
  if (s) {
//	while (count > 0) {
	while (s && d && count > 0) {
	  // Calculate how many bytes to copy from the source block
	  //  it's the minimum of the number of bytes remaining in the block
	  //  from s+src_ofst to that block end, the size of the destination block
	  //  and the total number of bytes that we need to copy.
	  int bytes_to_copy = min(min(((s->count)-s_ofst),count),((d->count)-d_ofst));
	  if (debug_flag) {
		fprintf(stderr,"min(%d,%d,%d) returned %d bytes to copy\n",((s->count)-s_ofst),count,((d->count)-d_ofst),bytes_to_copy);
	  }

// BUG	  memcpy((void *)((d->paddr)+d_ofst),(void *)((s->paddr)+s_ofst),bytes_to_copy);
memcpy(phys_to_ptr(d->paddr)+d_ofst, phys_to_ptr(s->paddr)+s_ofst, bytes_to_copy);
	  if (debug_flag) {
		fprintf(stderr,"Copied %d bytes from %p to %p\n",bytes_to_copy,(void *)((s->paddr)+s_ofst),(void *)((d->paddr)+d_ofst));
	  }
	  bytes_copied += bytes_to_copy;
	  count -= bytes_to_copy;
	  s_ofst += bytes_to_copy;
	  if (s_ofst >= s->count) {
		s = s->next;
		s_ofst = 0;
	  }
	  d_ofst += bytes_to_copy;
	  if (d_ofst >= d->count) {
		d = d->next;
		d_ofst = 0;
	  }
	}
  }

  return bytes_copied;
}


/*
 * sg_print      Prints a sg list to stdout for debugging
 *
 * @in src       Source sg list
 */
void sg_print(sg_entry_t *src) {
  sg_entry_t *p;
  int counter = 1;

  for (p = src; (p && (p != NULL)); p = p->next) {
	printf("%d: %lu %d\n",counter++,p->paddr,p->count);
	
  }
  printf("\n");
  return;
}


/*
 * sg_print_contents Prints the contents that a sg list points to, to stdout for debugging
 *
 * @in src       Source sg list
 */
extern void sg_print_contents(sg_entry_t *src) {
  sg_entry_t *p;

  for (p = src; (p && (p != NULL)); p = p->next) {
	printf("%.*s\n",p->count,(char *)(p->paddr));
	
  }
  printf("\n");
  return;
}

