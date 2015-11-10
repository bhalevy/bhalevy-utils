
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sg_copy.h"

// for debugging output
int debug_flag = 1;

int main(int argc, char **argv) {
  //physaddr_t p = (physaddr_t)&main;
  char *buf1, *buf2;
  sg_entry_t *sg1, *sg2;
  int copy_count;

  //fprintf(stderr,"p = %lx, %x, %lx\n",p,~(PAGE_SIZE-1),(p & ~(PAGE_SIZE-1)));
  //fprintf(stderr,"p = %lx --> %lx\n",p,(unsigned long)phys_to_ptr(p));

  buf1 = strdup("calloc() allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory.  The memory is set to zero.  malloc() allocates size bytes and returns a pointer to the allocated memory.  The memory is not cleared.  free() frees the memory space pointed to by ptr, which must have been returned by a previous call to malloc(), calloc() or realloc().  Otherwise, or if free(ptr) has already been called before, undefined behaviour occurs.  If ptr is NULL, no operation is performed.  realloc() changes the size of the memory block pointed to by ptr to size bytes.  The contents will be unchanged to the minimum of the old and new sizes; newly allocated memory will be uninitialized.  If ptr is NULL, the call is equivalent to malloc(size); if size is equal to zero, the call is equivalent to free(ptr).  Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or realloc().  If the area pointed to was moved, a free(ptr) is done.");
  buf2 = strdup("The strcpy() function copies the string pointed to by src (including the terminating \\0 character) to the array pointed to by dest.  The strings may not overlap, and the destination string dest must be large enough to receive the copy.  The strncpy() function is similar, except that not more than n bytes of src are copied. Thus, if there is no null byte among the first n bytes of src, the result will not be null-terminated.  In the case where the length of src is less than that of n, the remainder of dest will be padded with null bytes.");

  sg1 = sg_map(buf1,strlen(buf1));

  if (debug_flag) {
	fprintf(stderr,"\nsg1:\n");
	sg_print(sg1);
	sg_print_contents(sg1);
  }

  sg2 = sg_map(buf2,strlen(buf2));

  if (debug_flag) {
	fprintf(stderr,"\nsg2:\n");
	sg_print(sg2);
	fprintf(stderr,"\nsg2 contents:\n");  
	sg_print_contents(sg2);
  }

  if (debug_flag) {
	fprintf(stderr,"Going to sg_copy(sg1,sg2,481,90)\n");
  }

  copy_count = sg_copy(sg1,sg2,481,90);

  if (debug_flag) {
	fprintf(stderr,"\nsg2 after copying %d bytes:\n",copy_count);
	sg_print(sg2);
	fprintf(stderr,"\nsg2 contents:\n");  
	sg_print_contents(sg2);
  }

  return 0;
}
