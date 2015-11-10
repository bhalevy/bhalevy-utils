#include "sg_copy.h"

#define printk printf

sg_entry_t *sg_map(void *buf, int length)
{
// BH: in user land
//	long memflags =
//	    (GFP_KERNEL * !in_atomic()) | (GFP_ATOMIC * in_atomic());
	sg_entry_t *sghead;
	sg_entry_t *sge;
	int arrsize;
	int pages;

	pages = (((unsigned long)buf & ~PAGE_MASK) + length + ~PAGE_MASK) >> PAGE_SHIFT;
	arrsize = sizeof(sg_entry_t) * pages;
	/* better allocate an array to reduce memory fragmentation */
//	sghead = (sg_entry_t *) kmalloc(arrsize, memflags);
	sghead = (sg_entry_t *) malloc(arrsize);
	if (sghead == 0) {
		printk("%s %s:%d failed to allocate memory \n",
		       __FILE__, __FUNCTION__, __LINE__);
		return 0;
	}
	memset(sghead, 0, arrsize);

	for (sge = sghead; length > 0; sge++) {
		int offset = (unsigned long)buf & ~PAGE_MASK;
		sge->count = PAGE_SIZE - offset;
// BH: bug, min(count, length)
		/* 
		 * BUG: it is not enough to do virt to phys, we should also get_page here
		 *     and put it back when releasing
		 */
// BH: bug + offset ?
//		sge->paddr =
//		    ptr_to_phys((void *)(((unsigned long)buf) + offset));
		sge->paddr =
		    ptr_to_phys(buf);
printf("sge=%p paddr=%lx count=%d buf=%p length=%d\n", sge, sge->paddr, sge->count, buf, length);
		buf += sge->count;
		length -= sge->count;
// BH: BUG: pre increment
//		sge->next = ++sge;
		sge->next = sge + 1;
// BH: what about last?
	}
	return sghead;
}

void sg_destroy(sg_entry_t * sg_sist)
{
	sg_entry_t *temp = sg_sist;
	sg_entry_t *next;

	if (temp == 0) {
		printk("%s %s:%d failed to allocate memory \n",
		       __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	/* must save next pointer before release current ...
	 * aiee why not list_for_each_safe 
	*/
	for (next = temp; next; temp = next) {
		next = temp->next;
//		kfree(temp);
		free(temp);
	}
}

int sg_copy(sg_entry_t * src, sg_entry_t * dest, int src_offset, int count)
{
	sg_entry_t *head_src = src;
	sg_entry_t *head_dest = dest;
	int len = 0;

	if (!src || count < 0 || src_offset < 0 || !dest) {
		printk("%s %s:%d ilegal parm %p %d %d %p\n",
		       __FILE__, __FUNCTION__, __LINE__, src, count, src_offset,
		       dest);
// BH		dump_stack();
		return -1;
	}

	for (; src && dest;) {

		void *dest_addr = phys_to_ptr(dest->paddr);
		void *src_addr = phys_to_ptr(src->paddr + src_offset);

		// no point to check memcpy ret here
		memcpy(dest_addr, src_addr, src->count);
		dest->count = src->count;
		len += src->count;
		dest = dest->next;
		src = src->next;
		src_offset = 0;
	}
	return len;
}
