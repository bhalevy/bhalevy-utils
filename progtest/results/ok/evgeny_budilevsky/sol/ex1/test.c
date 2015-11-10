#include "sg_copy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#define STR1 "this is a long string with some random data which was created by me"

void test_copy_return_value()
{
  sg_entry_t *big_sg;
  sg_entry_t *small_sg;
  
  char big_buf[32*32+10];
  char small_buf[32*5+7];
  
  int ret;

  big_sg = sg_map(big_buf, ARRAY_SIZE(big_buf));
  assert(big_sg != NULL);

  small_sg = sg_map(small_buf, ARRAY_SIZE(small_buf));
  assert(small_sg != NULL);

  /* destination is small so don't copy more than small_buf
     size even when count > small_buf size */
  ret = sg_copy(big_sg, small_sg, 0, ARRAY_SIZE(big_buf));
  assert(ret == ARRAY_SIZE(small_buf));

  ret = sg_copy(big_sg, small_sg, 0, ARRAY_SIZE(small_buf));
  assert(ret == ARRAY_SIZE(small_buf));

  
  /* source is small so don't copy more than small_buf
     size even when count > small_buf size */
  ret = sg_copy(small_sg, big_sg, 0, ARRAY_SIZE(small_buf));
  assert(ret == ARRAY_SIZE(small_buf));

  ret = sg_copy(small_sg, big_sg, 0, ARRAY_SIZE(big_buf));
  assert(ret == ARRAY_SIZE(small_buf));

  /* make sure you skip the offset */
  ret = sg_copy(small_sg, big_sg, 3, ARRAY_SIZE(small_buf));
  assert(ret == ARRAY_SIZE(small_buf) - 3);

  /* count is less than src/dst sg size */
  ret = sg_copy(small_sg, big_sg, 3, 5);
  assert(ret == 5);

  sg_destroy(small_sg);
  sg_destroy(big_sg);
}

void test_copy_data()
{
  sg_entry_t *sg_src;
  sg_entry_t *sg_dst;

  char buf_src[] = STR1;
  char buf_dst[256];

  int ret;


  sg_src = sg_map(buf_src, ARRAY_SIZE(buf_src));
  assert(sg_src != NULL);

  sg_dst = sg_map(buf_dst, ARRAY_SIZE(buf_dst));
  assert(sg_dst != NULL);

  ret = sg_copy(sg_src, sg_dst, 0, ARRAY_SIZE(buf_src));
  assert(ret == (strlen(STR1) +1 ));
  assert(strcmp(buf_src, buf_dst) == 0);

  ret = sg_copy(sg_src, sg_dst, 7, ARRAY_SIZE(buf_src));
  assert(ret == (strlen(STR1) +1 -7 ));
  assert(strcmp(buf_src+7, buf_dst) == 0);

}

void test_only_first_element_not_aligned()
{
  sg_entry_t *sg;

  sg = sg_map((void*)7, 64);
  assert(sg != NULL);
  assert(sg->count == 25);
  assert(sg->next != NULL);
  assert(sg->next->count == 32);
  assert(sg->next->next != NULL);
  assert(sg->next->next->count == 7);
  assert(sg->next->next->next == NULL);

  sg_destroy(sg);
}

int main(int argc, char *argv[])
{
  test_copy_return_value();
  test_copy_data();
  test_only_first_element_not_aligned();
  return 0;
}
