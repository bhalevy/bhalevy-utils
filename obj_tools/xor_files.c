#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE (64 << 10)

int
main(int argc, char **argv)
{
  char *fname[2];
  int fd[2];
  unsigned char *buf[2];
  unsigned char *p[2];
  unsigned char *ep;
  int count[2], mincount, maxcount;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s file1 file2\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  buf[0] = malloc(BUF_SIZE);
  buf[1] = malloc(BUF_SIZE);
  if (buf[0] == NULL || buf[1] == NULL) {
    if (buf[0] != NULL) {
      free(buf[0]);
    }
    if (buf[1] != NULL) {
      free(buf[1]);
    }
    fprintf(stderr, "%s: malloc failed: %s\n", argv[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  fname[0] = argv[1];
  fname[1] = argv[2];
  fd[0] = open(fname[0], O_RDONLY);
  if (fd[0] < 0) {
    fprintf(stderr, "%s: open %s: %s\n", argv[0], fname[0], strerror(errno));
    free(buf[0]);
    free(buf[1]);
    exit(EXIT_FAILURE);
  }
  fd[1] = open(fname[1], O_RDONLY);
  if (fd[1] < 0) {
    fprintf(stderr, "%s: open %s: %s\n", argv[0], fname[1], strerror(errno));
    free(buf[0]);
    free(buf[1]);
    exit(EXIT_FAILURE);
  }

  for (;;) {
    count[0] = read(fd[0], buf[0], BUF_SIZE);
    if (count[0] < 0) {
      fprintf(stderr, "%s: read %s: %s\n", argv[0], fname[0], strerror(errno));
      free(buf[0]);
      free(buf[1]);
      exit(EXIT_FAILURE);
    }
    count[1] = read(fd[1], buf[1], BUF_SIZE);
    if (count[1] < 0) {
      fprintf(stderr, "%s: read %s: %s\n", argv[0], fname[1], strerror(errno));
      free(buf[0]);
      free(buf[1]);
      exit(EXIT_FAILURE);
    }

    if (count[0] > count[1]) {
      maxcount = count[0];
      mincount = count[1];
      memset(buf[1] + mincount, 0, maxcount - mincount);
    } else if (count[1] > count[0]) {
      maxcount = count[1];
      mincount = count[0];
      memset(buf[0] + mincount, 0, maxcount - mincount);
    } else {
      maxcount = count[0];
      mincount = count[0];
      if (!maxcount) {
	break;
      }
    }

    for (p[0] = buf[0], p[1] = buf[1], ep = p[0] + maxcount;
	 p[0] < ep; p[0]++, p[1]++) {
      *p[0] ^= *p[1];
    }

    write(1, buf[0], maxcount);
  }

  return 0;
}
