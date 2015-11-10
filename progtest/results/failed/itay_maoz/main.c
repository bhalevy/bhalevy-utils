#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sg_copy.h"

#define BUFF_LENGTH (256)

int main()
{
    int i, size, offset, len, expected_size;
    sg_entry_t * list1, * list2;
    void * p;
    byte buf1[BUFF_LENGTH] = { 0 };
    byte buf2[BUFF_LENGTH] = { 0 };
    for (i = 0 ; i < BUFF_LENGTH; i++) {
        buf1[i] = i & 0xFF;
    }

    print_buf(buf1, BUFF_LENGTH);

    /* test create and destroy */

    printf("expecting 32:\n");
    list1 = sg_map(buf1, 32);
    print_list(list1);
    sg_destroy(list1);

    printf("expecting 32, 20:\n");
    list1 = sg_map(buf1, 52);
    print_list(list1);
    sg_destroy(list1);

    printf("expecting 20:\n");
    list1 = sg_map(buf1, 20);
    print_list(list1);
    sg_destroy(list1);

    printf("expecting 32, 32:\n");
    list1 = sg_map(buf1, 64);
    print_list(list1);
    sg_destroy(list1);

    printf("expecting 32,32,... :\n");
    list1 = sg_map(buf1, BUFF_LENGTH);
    print_list(list1);
    sg_destroy(list1);

    /* test copy */

    list1 = sg_map(buf1, BUFF_LENGTH);
    list2 = sg_map(buf2, BUFF_LENGTH);

    for (len = 0; len < BUFF_LENGTH + 10; len++) {
        for (offset = 0; offset < BUFF_LENGTH + 10; offset++) {
            memset(buf2, 0, BUFF_LENGTH);
            size = sg_copy(list1, list2, offset, len);

            /* validate the size is as expected */ 
            expected_size = len;
            if (offset >= BUFF_LENGTH) {
                expected_size = 0;
            } else if (offset + len > BUFF_LENGTH) {
                expected_size = BUFF_LENGTH - offset;
            }
            printf("progress: len = %d, offset = %d, size = %d, expected_size = %d\n", len, offset, size, expected_size);
            assert(size == expected_size);

            /* validate buffers are the same after the copy */
            if (memcmp(buf1 + offset, buf2, size) != 0) {
                 fprintf(stderr, "error!! buffers don't match\n");
                 printf("buf1:\n");
                 print_buf(buf1 + offset, size);
                 printf("buf2:\n");
                 print_buf(buf2, size);
                 return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}
