#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

static inline max(int x, int y)
{
	return x < y ? y : x;
}

static inline min(int x, int y)
{
	return x > y ? y : x;
}

sg_entry_t* map_partial_page(void* buf, int length)
{
	sg_entry_t* sg_entry = malloc(sizeof(sg_entry_t));
//	length = min(length, PAGE_SIZE);
// BH: BUG: first page alignment
	length = min(length, PAGE_SIZE - ((unsigned)buf & (PAGE_SIZE-1)));
// BH	printf("mapping %p at length %d\n", buf, length);
	if (sg_entry == NULL)
		return NULL;
	sg_entry->paddr = ptr_to_phys(buf);
	sg_entry->count = length;
	sg_entry->next = NULL;
	return sg_entry;

}

sg_entry_t* map_page(void* buf)
{
	if (buf != buf & ~(PAGE_SIZE-1))
		return NULL;
	return map_partial_page(buf, PAGE_SIZE);
}

sg_entry_t* sg_map(void* buf, int length)
{
	int i;
	sg_entry_t* sg_list = NULL;
	sg_entry_t* last = NULL;

	if (length == 0)
		return NULL;

	sg_list = map_partial_page(buf, length);
	if (sg_list == NULL)
		return NULL;
	last = sg_list;
	buf += last->count;
	length -= last->count;

	while (length > PAGE_SIZE)
	{
		last->next = map_page(buf);
		if (last->next == NULL)
		{
			sg_destroy(sg_list);
			return NULL;
		}
		last = last->next;
		buf +=  last->count;
		length -= last->count;
	}

	if (length > 0)
	{
		last->next = map_partial_page(buf, length);
		if (last->next == NULL)
		{
			sg_destroy(sg_list);
			return NULL;
		}
	}

	return sg_list;
}

void sg_destroy(sg_entry_t* sg_list)
{
	sg_entry_t* temp = sg_list;
	while (temp)
	{
		sg_list = temp->next;
// BH		printf("freeing %p\n", temp);
		free(temp);
		temp = sg_list;
	}
}

int copy_entry(sg_entry_t* src, int src_offset, sg_entry_t* dst, int dst_offset, int count)
{
	void* src_buf;
	void* dst_buf;

	if (!src || !dst || !count)
	{
// BH		printf("nothing to do: src = %p, dst = %p, count = %d\n", src, dst, count);
		return 0;
	}

	src_buf = phys_to_ptr(src->paddr) + src_offset;
	dst_buf = phys_to_ptr(dst->paddr) + dst_offset;
	
	count = min(count, dst->count - dst_offset);
// BH	printf("copy %d bytes from %p to %p\n", count, src_buf, dst_buf);
	memcpy(dst_buf, src_buf, count);
	return count;
}

int sg_copy(sg_entry_t* src, sg_entry_t* dest, int src_offset, int count)
{
	int dst_offset = 0;
	int left = count, copied = 0, total_copied = 0, nbytes = 0;

	while (src && src_offset > src->count)
	{
// BH		printf("src_offset %d, skipping entry %p of %d bytes\n", src_offset, src, src->count);
		src_offset -= src->count;
		src = src->next;
	}
	if (!src)
		return 0;
// BH	printf("starting with src entry %p (vaddr %p)\n", src, phys_to_ptr(src->paddr));

	while (src && dest && left)
	{
		nbytes = min(left, src->count - src_offset);
		copied = copy_entry(src, src_offset, dest, dst_offset, nbytes);
		left -= copied;
// BH		printf("copied %d bytes from %p (%lx) at offset %d\n", copied, src, src->paddr, src_offset);
		dst_offset += copied;
		if (dst_offset == dest->count)
		{
			dst_offset = 0;
			dest = dest->next;
		}
		src_offset += copied;
		if (src_offset == src->count)
		{
			src_offset = 0;
			src = src->next;
		}
	}
// BH	printf("stoped: src = %p, dest = %p, left = %d\n", src, dest, left);
	return count - left;
}

#if 0
void sg_print(sg_entry_t* sg_list)
{
	while (sg_list)
	{
		printf("entry: %p, paddr: %lx, count: %d, next: %p\n", sg_list, sg_list->paddr, sg_list->count, sg_list->next);
		sg_list = sg_list->next;
	}
}

void dump_buffer(char* buf, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		printf("%x ", buf[i]);
	}
	printf("\n");
}

int main(int argc, char** argv)
{
	char* buffer = NULL;
	char* buffer2 = NULL;
	int i, copied;
	sg_entry_t* sg_list = NULL;
	sg_entry_t* sg_list2 = NULL;
	int equal = 0;
	int size1, size2, copy_size, copy_offset;
	int should_copy = 0;

	if (argc != 5)
	{
		printf("bad args!\n");
		exit(EXIT_FAILURE);
	}

	size1 = atoi(argv[1]);
	size2 = atoi(argv[2]);
	copy_size = atoi(argv[3]);
	copy_offset = atoi(argv[4]);

	buffer = malloc(size1);
	buffer2 = malloc(size2);

	should_copy = min(copy_size, min(size1 - copy_offset, size2));
	printf("buffer: %p[%d], buffer2: %p[%d], copy %d bytes (should copy %d) at offset %d\n", buffer, size1, buffer2, size2, copy_size, should_copy, copy_offset);

	for (i = 0 ; i < size1 ; i++)
		buffer[i] = i + 32;

	sg_list = sg_map(buffer, size1);
	sg_list2 = sg_map(buffer2, size2);
	sg_print(sg_list);
	copied = sg_copy(sg_list, sg_list2, copy_offset, copy_size);
	equal = memcmp(buffer + copy_offset, buffer2, copied) == 0;
	printf("copied %s%d bytes, buffers %sequal\n", copied < should_copy ? "only " : "", copied, equal ? "" : "not ");
	if (!equal)
	{
		dump_buffer(buffer + copy_offset, max(size1 - copy_offset, 0));
		dump_buffer(buffer2, size2);
	}
	sg_destroy(sg_list);
	sg_destroy(sg_list2);
	free(buffer);
	free(buffer2);
}

#endif
