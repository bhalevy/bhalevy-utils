/*
 * bfind
 *
 * find a string in a file, block by block
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define BLOCKSZ 512

char *progname;

static ssize_t
kmgt(char *s)
{
  char *es;
  long ln = strtol(s, &es, 0);
  ssize_t n = (ssize_t)ln;

  if ((long)n != ln) {
    fprintf(stderr, "%s: %s: value too big\n", progname, s);
    exit(1);
  }

  switch (*es) {
  case 't': case 'T': n <<= 10;
  case 'g': case 'G': n <<= 10;
  case 'm': case 'M': n <<= 10;
  case 'k': case 'K': n <<= 10;
  }

  return n;
}
 
void
usage(void)
{
  fprintf(stderr, "Usage: %s [-b blocksz] string [file ...]\n", progname);
  exit(1);
}

static ssize_t
readblock(int fd, char *buf, int blocksz)
{
  ssize_t n, count;

  for (count = 0; count < blocksz; count += n) {
    n = read(fd, buf + count, blocksz - count);
    if (n > 0) {
      continue;
    }
    if (n == 0) {
      break;
    }
    return -1;
  }

  return count;
}

void
bfind(char *fname, int fd, char *str, char *buf, int blocksz)
{
  int slen = strlen(str);
  ssize_t nbytes;
  char *match;
  long long bnum;

  for (bnum = 0; (nbytes = readblock(fd, buf, blocksz)) > 0; bnum++) {
    match = buf;
    while (nbytes - (match - buf) >= slen &&
           (match =
            memchr(match, *str, nbytes - slen - (match - buf) + 1)) != NULL) {
      if (!memcmp(match, str, slen)) {
        printf("%s%s%5lld %10lld\n",
               fname ? fname : "",
               fname ? ": " : "",
               bnum,
               bnum * blocksz + (long long)(match - buf));
        match += slen;
      } else {
        match++;
      }
    }
    /* deal with console oddities */
    if (nbytes < blocksz) {
      break;
    }
  }

  if (nbytes < 0) {
    fprintf(stderr, "%s: read %s: %s\n",
            progname,
            fname ? fname : "<stdin>",
            strerror(errno));
    exit(1);
  }
}
 
int
main(int argc, char **argv)
{
  ssize_t blocksz = BLOCKSZ;
  char *s, *fname;
  char *buf;

  progname = *argv++;
  if ((s = strrchr(progname, '/')) != NULL) {
    progname = s - 1;
  }

  while (*argv && (*argv)[0] == '-') {
    if (!strcmp(*argv, "--")) {
      argv++;
      break;
    }

    switch ((*argv)[1]) {
    case 'b':
      argv++;
      if (!*argv) {
        usage();
      }
      blocksz = kmgt(*argv);
      break;
    default:
      usage();
    }
    argv++;
  }

  if (!*argv) {
    usage();
  }

  s = *argv++;

  buf = malloc(blocksz);
  if (!buf) {
    fprintf(stderr, "%s: memory allocation failure\n", progname);
    exit(1);
  }

  if (!*argv) {
    bfind("<stdin>", 0, s, buf, blocksz);
  } else while ((fname = *argv++) != NULL) {
    int fd = open(fname, O_RDONLY);

    if (fd < 0) {
      fprintf(stderr, "%s: %s: %s\n", progname, fname, strerror(errno));
      exit(1);
    }

    bfind(fname, fd, s, buf, blocksz);

    close(fd);
  }

  return 0;
}
