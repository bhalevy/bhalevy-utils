#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sg_copy.h"

#define _MIN(a, b)	((a) <= (b) ? (a) : (b))

static unsigned int sg_len(sg_entry_t *p);


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
sg_entry_t *sg_map(void *buf, int length)
{
	unsigned int buf_size;
	unsigned int num_entries;
	size_t alignNeeded = 0;
	unsigned int first_chunk_size;

	sg_entry_t *entry = NULL;
	sg_entry_t *first_entry = NULL;
	sg_entry_t *former_entry = NULL;


	if (length <= 0) {
//		printf("Error - invalid buf length\n");
		return NULL;
	}
	buf_size = (unsigned int)length;

	/* Check the need for alignment: */

	/* A.M. note:
	** Where y=2^n, X%y = X&(~(y-1)) ;
	** but I'll stay with the general case ((y!=2^n), which doesn't make much sense ...).
	** Anyway, the compiler optimization will do this code substitution when y=2^n.
	*/
	//alignNeeded = ((size_t)ptr_to_phys(buf)) & (~(PAGE_SIZE - 1));
	alignNeeded = ((size_t)ptr_to_phys(buf)) % PAGE_SIZE;

	first_chunk_size = (alignNeeded) ? ((size_t)PAGE_SIZE - alignNeeded) : 0;
	/* Adjust for very small buf_size, less than alignment */
	first_chunk_size = _MIN(buf_size, first_chunk_size);  

	/* A.M. note:
	** Allocating the memory for the list in advance, as one chunk,
	** is prefered over separate allocation for each node:
	**   1. No need to handle malloc() failure for each node.
	**   2. Destroying the list it is simpler - a single free() rather
	**      then freeing each node in sequence.
	**/

	
	/* Calculate number of entries (number of pages) */
	num_entries = (!alignNeeded) ?
			((buf_size + PAGE_SIZE - 1) / PAGE_SIZE) :
			(1 + ((buf_size - first_chunk_size + PAGE_SIZE - 1) / PAGE_SIZE));

//	printf("Final num_entries: %u\n", num_entries);

	/* Memory allocation for the whole list */
	first_entry = entry = 
		(sg_entry_t *)malloc(num_entries * sizeof(sg_entry_t));
	if (entry == NULL) {
//		printf("Error - no memory\n");
// BH: BUG: need sg_destroy
		return NULL;
	}

	if (alignNeeded) {
		/* First chunk is a non-whole-page */
		entry->count = (int)first_chunk_size;
		entry->paddr = ptr_to_phys(buf);
		entry->next = NULL;
		
		/* Advance buf */
		buf = (void *)((size_t)buf + first_chunk_size);
		buf_size -= first_chunk_size;

		/* Advance list */
		former_entry = first_entry;
		++entry;
		--num_entries;
	} else {
		/* First chunk is a whole-page - no special treatment */
		former_entry = NULL;
	}

	/* Other chunks are whole-page each */
	while (num_entries--) {
		entry->count = (int)(_MIN(PAGE_SIZE, buf_size));
		entry->paddr = ptr_to_phys(buf);
		entry->next = NULL;

		/* Chain the list */
		if (former_entry != NULL) {
			former_entry->next = entry;
		}

		/* Advance buf */
		buf = (void *)((size_t)buf + (size_t)(entry->count));
		buf_size -= (size_t)(entry->count);

		/* Advance list */
		former_entry = entry;
		++entry;
	}

	/* Debug: the whole buf should be exhausted here */
	assert(buf_size == 0);

	return first_entry;
}


/*
 * sg_destroy    Destroy a scatter-gather list
 *
 * @in sg_list   A scatter-gather list
 */
void sg_destroy(sg_entry_t *sg_list)
{
	if (sg_list != NULL) {
		/* Note: the whole list was allocated as 
		 * a single memory chunk
		 */
// BH: BUG, need recursive
		free(sg_list);
	} else {
//		printf("Error - invalid sg-list pointer\n");
	}
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
 */
int sg_copy(sg_entry_t *src, sg_entry_t *dest, int src_offset, int count)
{
	int copied;
	int src_offset_in_page, dst_offset_in_page;
	unsigned int src_len, dst_len, copy_len;
	unsigned char *src_page_virt_addr, *dst_page_virt_addr;

	/* Check parameters */
	if ((src == NULL) || (dest == NULL) ||
		(count < 0) || (src_offset < 0)) {
//		printf("Error - invalid param(s)\n");
		return 0;
	}
	
	if (count == 0) {
		return 0;
	}

	/* Determine possible number of bytes to copy: */
	src_len = sg_len(src);
	if ((unsigned int)src_offset >= src_len) {
//		printf("Error - src offset exceeds src buf length\n");
		return 0;
	}
	src_len -= (unsigned int)src_offset;
	dst_len = sg_len(dest);

	copy_len = _MIN(src_len, dst_len);
	copy_len = _MIN(copy_len, ((unsigned int)count));
	copied = (int)copy_len;

	/* Skip offset bytes at src sg map */
	while (src_offset >= src->count) {
		/* Advance to next page */
		src_offset -= src->count;
		src = src->next;
		assert(src);
	}

	/* Copy src to dst: */
	src_page_virt_addr = (unsigned char *)phys_to_ptr(src->paddr);
	dst_page_virt_addr = (unsigned char *)phys_to_ptr(dest->paddr);
	src_offset_in_page = src_offset;
	dst_offset_in_page = 0;

	while (copy_len--) {
		*(dst_page_virt_addr + dst_offset_in_page) =
				*(src_page_virt_addr + src_offset_in_page);

		++src_offset_in_page;
		assert(src);
		if (src_offset_in_page >= src->count) {
			src = src->next;
			src_offset_in_page = 0;
			if (src != NULL) {
				/* End of src list not reached */ 
				src_page_virt_addr = (unsigned char *)phys_to_ptr(src->paddr);
			}
		}

		++dst_offset_in_page;
		assert(dest);
		if (dst_offset_in_page >= dest->count) {
			dest = dest->next;
			dst_offset_in_page = 0;
			if (dest != NULL) {
				/* End of dst list not reached */ 
				dst_page_virt_addr = (unsigned char *)phys_to_ptr(dest->paddr);
			}
		}
	}

	return copied;
}

/* ------------------------------------------------------------------------- */

static unsigned int sg_len(sg_entry_t *p)
{
	unsigned int len = 0;

	while (p != NULL) {
		len += p->count;
		p = p->next;
	}

	return len;
}

#if 0
/* ------------------------------------------------------------------------- */

/**************** Test Section ******************/

static void prtSGlist(sg_entry_t * p)
{
	unsigned int i = 0;
	unsigned int tot_size = 0;

	while (p != NULL) {
// BH
int j;

		++i;
		tot_size += p->count;
		printf("SG Node %u at 0x%p : count: %2.2i, phys_addr: 0x%p, next entry at: 0x%p\n",
			i, p, p->count, p->paddr, p->next);

#if 1 //0
		for (j = 0; j < p->count; ++j) {
			printf("%hu ", ((unsigned char *)(phys_to_ptr(p->paddr)))[j] ); 
		}
		printf("\n");
#endif

		p = p->next;
	}

	printf("SG data size: %lu\n\n", tot_size);
}

//#define TST_BUF_SIZE     0
//#define TST_BUF_SIZE     1
//#define TST_BUF_SIZE     2
//#define TST_BUF_SIZE    15
//#define TST_BUF_SIZE    16
//#define TST_BUF_SIZE    17
//#define TST_BUF_SIZE    31
//#define TST_BUF_SIZE    32
//#define TST_BUF_SIZE    33
//#define TST_BUF_SIZE    46
//#define TST_BUF_SIZE    48
//#define TST_BUF_SIZE    49
//#define TST_BUF_SIZE    57
//#define TST_BUF_SIZE    63
//#define TST_BUF_SIZE    64
//#define TST_BUF_SIZE    65
//#define TST_BUF_SIZE    80
//#define TST_BUF_SIZE    81
//#define TST_BUF_SIZE   113
//#define TST_BUF_SIZE   525
//#define TST_BUF_SIZE   528
//#define TST_BUF_SIZE   529
//#define TST_BUF_SIZE   544
//#define TST_BUF_SIZE   600
//#define TST_BUF_SIZE  2049
#define TST_BUF_SIZE  13

void main(void)
{
	/* Test map creation */
	unsigned int i;
	unsigned char *tstBuf = (unsigned char *)malloc(TST_BUF_SIZE);
	for (i = 0; i < TST_BUF_SIZE; i++) {
		tstBuf[i] = (unsigned char)i;
	}

	sg_entry_t *sg = sg_map(tstBuf, TST_BUF_SIZE);
	if (sg != NULL) {
		prtSGlist(sg);
		i = sg_len(sg);
		printf("SG buf length = %u\n", i);

		sg_destroy(sg);
	}

	free(tstBuf);

	printf("\n\n\n");

	/* Test map copy */
#define SRC_LEN	74
#define DST_LEN	69
	int num_copied;
	unsigned char * srcBuf = (unsigned char *)malloc(SRC_LEN);
	unsigned char * dstBuf = (unsigned char *)calloc(1, DST_LEN);

	for (i = 0; i < SRC_LEN; i++) {
		srcBuf[i] = (unsigned char)i;
	}

	sg_entry_t * srcMap = sg_map(srcBuf, SRC_LEN);
	prtSGlist(srcMap);

	sg_entry_t * dstMap = sg_map(dstBuf, DST_LEN);
	prtSGlist(dstMap);

	num_copied = sg_copy(srcMap, dstMap, 0, 7);
	printf("%i bytes copied from src map, at offset 0, to dst map:\n", num_copied);
	prtSGlist(dstMap);

	num_copied = sg_copy(srcMap, dstMap, 5, 69);
	printf("%i bytes copied from src map, at offset 5, to dst map:\n",num_copied);
	prtSGlist(dstMap);

	num_copied = sg_copy(srcMap, dstMap, 17, 80);
	printf("%i bytes copied from src map, at offset 17, to dst map:\n",num_copied);
	prtSGlist(dstMap);

	sg_destroy(srcMap);
	sg_destroy(dstMap);
	free(srcBuf);
	free(dstBuf);
}
#endif
