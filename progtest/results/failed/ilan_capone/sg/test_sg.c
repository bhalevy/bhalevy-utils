#include "sg_copy.h"
#include "stdio.h"
#include "string.h"

static char long_string[] =
	"Quoting the comment in the beginning of the implementation, just to serve as a buffer long enough to play with...\n"
	"\n"
	"/*\n"
	" * NOTES:\n"
	" *\n"
	" * -1-\n"
	" * I am relying on an assumption, that all included in the header file is\n"
	" * part of this API, and can be relied on.\n"
	" * Specifically - the inline functions translating between pointers and\n"
	" * physaddr_t show that page boundary checks for either are the same.\n"
	" * With this justification, I am not translating back and forth to check\n"
	" * the validity of a given entry, or to calculate the proper parameters\n"
	" * for a new or copied entry.\n"
	" *\n"
	" * -2-\n"
	" * The specification of sg_copy() implies, that the dest list is already\n"
	" * allocated (\"sg_entry_t **dest_p\" would imply it is an output parameter\n"
	" * carrying a pointer to data our function should allocate).\n"
	" * I assumed copy semantics, and not duplicate (not allocating new entries\n"
	" * for the dest list, and using as bounds all 3 elements: src list existing\n"
	" * entries, dest list allocated entries, and given count.\n"
	" * Were the semantics those of duplicate, I could use the given entry as\n"
	" * a head with zero count, and allocate new entries in a manner similar\n"
	" * to that done in sg_map().\n"
	" */\n";


static char *
get_aligned_buffer()
{
	return (char *)((unsigned long)(long_string + PAGE_SIZE - 1) & ~(PAGE_SIZE-1));
}

static char *
get_unaligned_buffer(int by_how_much)
{
	return get_aligned_buffer() + by_how_much;
}

static int
compare_entry_buf(sg_entry_t *list, void *buf, int buf_len)
{
	while (list && (buf_len > 0)) {
		void* entry_mem  = phys_to_ptr(list->paddr);
		int   entry_len  = list->count;
		int   memcmp_ret;

		if (entry_len > buf_len) {
			return 1;
		}
		if (memcmp(entry_mem, buf, entry_len)) {
			return 1;
		}

		list     = list->next;
		buf     += entry_len;
		buf_len -= entry_len;
	}
	if (list || buf_len) {
		return 1;
	}

	return 0;
}

static int s_tests_run    = 0;
static int s_tests_failed = 0;

#define RUN_TEST(test)										\
	do {													\
		++s_tests_run;										\
		if ((test)()) {										\
			++s_tests_failed;								\
			fprintf(stderr, "Failed test: %s\n", #test);	\
		}													\
	} while (0)

static void
print_summary()
{
	fprintf(stderr, "Total %d tests; %d failed, %d passed.\n", s_tests_run, s_tests_failed, s_tests_run-s_tests_failed);
}


static int test_map_aligned1();
static int test_map_unaligned2();
static int test_map_unaligned3();
static int test_copy1();


int
main()
{
	int rc = 0;

	RUN_TEST(test_map_aligned1);
	RUN_TEST(test_map_unaligned2);
	RUN_TEST(test_map_unaligned3);
	RUN_TEST(test_copy1);

	print_summary();

	return s_tests_failed;
}



static int
test_map_aligned1()
{
	char       *buf   = get_aligned_buffer();
	int         len   = strlen(buf);
	sg_entry_t *entry = sg_map(buf, len);
	int         rc    = compare_entry_buf(entry, buf, len);
	sg_destroy(entry);
	return rc;
}


static int
test_map_unaligned2()
{
	char       *buf   = get_unaligned_buffer(5);
	int         len   = strlen(buf);
	sg_entry_t *entry = sg_map(buf, len);
	int         rc    = compare_entry_buf(entry, buf, len);
	sg_destroy(entry);
	return rc;
}


static int
test_map_unaligned3()
{
	char       *buf   = get_unaligned_buffer(16);
	int         len   = strlen(buf);
	sg_entry_t *entry = sg_map(buf, len);
	int         rc    = compare_entry_buf(entry, buf, len);
	sg_destroy(entry);
	return rc;
}


static int
test_copy1()
{
	sg_entry_t  a1, a2, a3, a4;
	char       *buf      = get_aligned_buffer();
	int         len      = strlen(buf);
	int         offset1  = 17;
	int         offset2  = 20;
	sg_entry_t *copy_in  = sg_map(buf+offset1, len-offset1);
	sg_entry_t *copy_out = &a1;
	int         copied;
	a1.next = &a2; a2.next = &a3; a3.next = &a4; a4.next = NULL;

	copied = sg_copy(copy_in, copy_out, offset2, len);

	sg_destroy(copy_in);

	/*
	 * First entry of copy_in should contain 15 bytes.
	 * Second and on - 32 bytes each, until the last, which will contain less.
	 *
	 * copy_out copies from byte 20 of copy_in. Since its 1st entry contains 15 bytes,
	 * this is byte 5 of its second, and therefore will contain 27 bytes.
	 * The rest of copy_out will contain 32 bytes each.
	 * This gives us (27 + (3 * 32)) = 123.
	 */

	if (copied != 123) {
		return 1;
	}
	return compare_entry_buf(copy_out, buf+offset1+offset2, copied);	
}

