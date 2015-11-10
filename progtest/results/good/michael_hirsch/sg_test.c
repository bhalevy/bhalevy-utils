/*
 * sg_copy.c
 *
 * Tutorial implementation of scatter/gather with mapped memory
 *
 * Michael Hirsch
 * 2011-10-04
 */

#include <stdio.h>
#include <stdlib.h>		/* for malloc and free */
#include <string.h>		/* for memcpy */
#include "sg_base.h"
#include "sg_copy.h"

#define BUF_LEN	256
#define CHECK_CHAR	'd'

/*
 * random_in_range	return a random number in the range [low,high)
 */

int random_in_range(int low, int high)
{
	int val;

	if (low == high) {
		return low;
	}
	if (low > high) {
		int temp = low;
		low = high;
		high = temp;
	}

	val = (random() % (high - low)) + low;

	if ((val < low) || (val >= high)) {
		printf("random val %d out of range [%d, %d)\n",
		       val, low, high);
		return (low + high) / 2;
	}
	return val;
}

int main(int argc, char *argv[])
{
	byte	src_buf[BUF_LEN];
	byte	dest_buf[BUF_LEN];
	int	src_start;
	int	dest_start;
	int	src_len;
	int	dest_len;
	int	copy_start;
	int	to_copy;
	int	copied;
	int	expect_copied;
	int	n_reps;
	int	i;
	int	j;

	sg_entry_t	*src_sg;
	sg_entry_t	*dest_sg;

	if (argc != 2) {
		printf("Usage: %s n_reps\n\n"
		       "Test the scatter/gather implementation",
		       argv[0]);
		exit(1);
	}

	n_reps = atoi(argv[1]);

	/* initialize the source */
	for (i = 0; i < BUF_LEN; i++) {
		src_buf[i] = (byte) i;
	}

	for (i = 0; i < n_reps; i++) {
		memset(dest_buf, CHECK_CHAR, BUF_LEN);

		/* choose random copy params */
		src_start = random_in_range(0, PAGE_SIZE);
		dest_start = random_in_range(0, PAGE_SIZE);
		src_len = random_in_range(0, BUF_LEN - src_start);
		dest_len = random_in_range(0, BUF_LEN - dest_start);
		copy_start = random_in_range(0, src_len);
		to_copy = random_in_range(copy_start, BUF_LEN);

		/* build the maps */
		src_sg = sg_map(src_buf + src_start, src_len);
		dest_sg = sg_map(dest_buf + dest_start, dest_len);

		/* copy */
		copied = sg_copy(src_sg, dest_sg, copy_start, to_copy);

		/* check the result length */
		expect_copied = to_copy;
		if (copy_start > src_len) {
			expect_copied = 0;
		}
		else if (expect_copied > (src_len - copy_start)) {
			expect_copied = src_len - copy_start;
		}
		if (expect_copied > dest_len) {
			expect_copied = dest_len;
		}

		if (copied != expect_copied) {
			printf("Rep %d: Copied to few bytes %d,"
			       " expected %d\n"
			       "\tto_copy %d\n"
			       "\tsrc_len %d\n"
			       "\tdest_len %d\n"
			       "\tcopy_start %d\n",
			       i,
			       copied,
			       expect_copied,
			       to_copy,
			       src_len,
			       dest_len,
			       copy_start);
			exit(1);
		}

		/* check the result data */
		for (j = 0; j < dest_start; j++) {
			if (dest_buf[j] != CHECK_CHAR) {
				printf("Rep %d: corrupt dest[%d]\n",
				       i, j);
			}
		}
		for (j = dest_start + copied; j < BUF_LEN; j++) {
			if (dest_buf[j] != CHECK_CHAR) {
				printf("Rep %d: corrupt dest[%d]\n",
				       i, j);
			}
		}
		if ((copied > 0) &&
		    (memcmp(src_buf + src_start + copy_start,
			    dest_buf + dest_start,
			    copied) != 0)) {
			printf("Rep %d: data mismatch\n", i);
			exit(1);
		}

		/* clean up */
		sg_destroy(src_sg);
		sg_destroy(dest_sg);
	}

	exit(0);
}
