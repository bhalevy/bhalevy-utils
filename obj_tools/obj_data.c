#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

typedef long long pan_int64_t;
typedef unsigned long long pan_uint64_t;

pan_uint64_t
kmg(char *s)
{
  pan_uint64_t n = strtoll(s, &s, 0);

  switch (*s) {
  case 'g': case 'G': n <<= 10;  /*FALLTHROUGH*/
  case 'm': case 'M': n <<= 10;  /*FALLTHROUGH*/
  case 'k': case 'K': n <<= 10;
  }

  return n;
}

void
usage(char *progname)
{
  fprintf(stderr,
	  "Usage: %s [-xn -o offset -l length -b blocksz] file ...\n",
	  progname);
  exit(EXIT_FAILURE);
}

void
whereis(int ncomps,
	pan_uint64_t offset,
	int blocksz,
	int *comp,
	pan_uint64_t *comp_offset)
{
  pan_uint64_t nblock = offset / blocksz;
  int blocks_per_stripe = ncomps - 1;
  pan_uint64_t nstripe = nblock / blocks_per_stripe;
  int stripe_unit = nblock % blocks_per_stripe;
  int stripe_idx = nstripe % ncomps;

  *comp_offset = nstripe * blocksz + offset % blocksz;
  *comp = (stripe_unit + ncomps - stripe_idx) % ncomps;
}

int
main(int argc, char **argv)
{
  int ch;
  int comp;
  pan_uint64_t comp_offset;
  int ncomps;
  pan_uint64_t offset = 0, end;
  pan_int64_t length = 0;
  int blocksz = 64<<10;
  int nflag = 0;
  int n;
  int *fds;
  char **fnames;
  char *buf;
  int vflag = 0;
  int xflag = 0;

  while ((ch = getopt(argc, argv, ":hxnvo:l:b:")) > 0) {
    switch (ch) {
    case 'o':
      offset = kmg(optarg);
      break;
    case 'l':
      length = kmg(optarg);
      break;
    case 'b':
      blocksz = kmg(optarg);
      break;
    case 'n':
      nflag++;
      break;
    case 'v':
      vflag++;
      break;
    case 'x':
      xflag++;
      break;
    case 'h':
    case '?':
    default:
      usage(argv[0]);
    }
  }

  fnames = argv + optind;
  ncomps = argc - optind;

  if (ncomps <= 0) {
    usage(argv[0]);
  }

  fds = calloc(ncomps, sizeof(*fds));
  buf = malloc(blocksz);

  if (fds == NULL || buf == NULL) {
    fprintf(stderr, "%s: malloc: %s\n", argv[0], strerror(errno));
    exit(EXIT_FAILURE);
  }

  for (comp = 0; !nflag && comp < ncomps; comp++) {
    fds[comp] = open(fnames[comp], O_RDONLY);
    if (fds[comp] < 0) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], fnames[comp], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  for (end = offset + length; offset < end; offset += blocksz) {
    whereis(ncomps, offset, blocksz, &comp, &comp_offset);
    if (nflag) {
      printf(xflag ? 
	     "%12llx %s %llx\n" :
	     "%12lld %s %lld\n",
	     offset, fnames[comp], comp_offset);
      continue;
    }

    if (lseek(fds[comp], comp_offset, SEEK_SET) != comp_offset) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], fnames[comp], strerror(errno));
      exit(EXIT_FAILURE);
    }

    n = read(fds[comp], buf, blocksz);
    if (vflag) {
      fprintf(stderr, "offset %llu comp %d comp_offset %llu read %d\n",
              offset, comp, comp_offset, n);
    }
    if (n < 0) {
      fprintf(stderr, "%s: %s: %s\n", argv[0], fnames[comp], strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (n < blocksz) {
      if (end - offset <= blocksz) {
        memset(buf + n, 0, (end - offset) - n);
        n = end - offset;
      }
    }
    if (write(1, buf, n) != n) {
      fprintf(stderr, "%s: stdout: %s\n", argv[0], strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}
