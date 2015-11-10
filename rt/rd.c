#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define TIMEVAL_SUB(tv1, tv2) { \
  (tv1)->tv_usec -= (tv2)->tv_usec; \
  while ((tv1)->tv_usec < 0) { \
   (tv1)->tv_sec--; \
   (tv1)->tv_usec += 1000000; \
  } \
  (tv1)->tv_sec -= (tv2)->tv_sec; \
}

static char *argv0;
static char *fname = "<stdout>";
static int rflag;
static int vflag;
static char *scrname;
static FILE *scrfile;

void
usage(void)
{
  fprintf(stderr, "Usage: %s [-rvL] [-b blocksz] [-B beginoffset] [-l length] [-t targetbw] "
                  "[-s seed] [-S scrfile] [file]\n", argv0);
  exit(1);
}

int
randomval(int n)
{
  long long r = random();

  return (int)((n * r) / ((long long)RAND_MAX+1));
}

void
prep(void *buf, long long offset, int count)
{
  unsigned long long *bp, *ep;

  for (bp = buf, ep = bp + count/sizeof(*bp); bp < ep; bp++) {
    if (!rflag) {
      *bp = (unsigned long long)offset;
      offset += sizeof(*bp);
    } else {
      unsigned short *sp;

      for (sp = (unsigned short*)bp;
           sp < (unsigned short*)(bp+1);
           sp++) {
        /*
         * not the ideally random but good enough
         * and faster then randomval()
         */
        *sp = (unsigned short)random();
      }
    }
  }
}

long long
kmg(char *s)
{
  long long n = strtoll(s, &s, 0);

  switch (*s) {
  case 'g': case 'G': n <<= 10; /*FALLTHROUGH*/
  case 'm': case 'M': n <<= 10; /*FALLTHROUGH*/
  case 'k': case 'K': n <<= 10; /*FALLTHROUGH*/
  }

  return n;
}

void
report(long long bytes, struct timeval *time0)
{
  struct timeval ct, dt;

  gettimeofday(&ct, NULL);
  dt.tv_sec = ct.tv_sec - time0->tv_sec;
  dt.tv_usec = ct.tv_usec - time0->tv_usec;
  while (dt.tv_usec < 0) {
    dt.tv_sec--;
    dt.tv_usec += 1000000;
  }
  while (dt.tv_usec >= 1000000) {
    dt.tv_sec++;
    dt.tv_usec -= 1000000;
  }
  fprintf(stderr, "read %s %12lld bytes %4ld.%03ld sec %5.1f MB/s\n",
    fname, bytes,
    dt.tv_sec, dt.tv_usec / 1000,
    (double)bytes / ((1<<20) * (dt.tv_sec + (double)dt.tv_usec/1000000)));
  fflush(stderr);
}

int
main(int argc, char** argv)
{
  int ch;
  ssize_t n, nread;
  long long offset, begin, bytes;
  int fd = 1;
  size_t blocksz = 4<<10;
  int targetbw = 0;
  long long length = 0;
  void *buf = NULL;
  long seed = 0;
  int lflag = 0;
  struct timeval time0;
  long long last_offs;

  argv0 = argv[0];

  while ((ch = getopt(argc, argv, ":vrLb:l:s:t:B:S:")) != -1)
    switch (ch) {
    case 'r':
      rflag++;
      break;
    case 'v':
      vflag++;
      break;
    case 'L':
      lflag++;
      break;
    case 's':
      seed = atol(optarg);
      break;
    case 'b':
      blocksz = (size_t)kmg(optarg);
      if (blocksz % sizeof(long long)) {
        fprintf(stderr, "%s: blocksz must be in multiples of %Zd\n",
          argv0, sizeof(long long));
        usage();
      }
      break;
    case 'l':
      length = kmg(optarg);
      break;
    case 't':
      targetbw = kmg(optarg);
      break;
    case 'B':
      begin = kmg(optarg);
      break;
    case 'S':
      scrname = optarg;
      scrfile = fopen(scrname, "r");
      if (!scrfile) {
        fprintf(stderr, "%s: open %s: %s\n",
          argv0, scrname, strerror(errno));
        usage();
      }
      break;
    case '?':
    default:
      usage();
    }

  argc -= optind;
  argv += optind;

  if (*argv) {
    fname = *argv;
    if ((fd = open(fname, O_RDONLY)) < 0) {
      fprintf(stderr, "%s: open %s: %s\n", argv0, fname, strerror(errno));
      exit(-1);
    }
  }

  if ((buf = malloc(blocksz)) == NULL) {
    fprintf(stderr, "%s: malloc %zd: %s\n", argv0, blocksz, strerror(errno));
    exit(-1);
  }

  gettimeofday(&time0, NULL);
  if (rflag) {
    if (!seed) {
      seed = (time0.tv_sec ^ time0.tv_usec);
    }
    srandom(seed);
    fprintf(stderr, "%s: seed %ld\n", argv0, seed);
    prep(buf, 0, blocksz);
  }

  bytes = 0;

  if (scrfile) {
    int ntokens, ln;

    for (ln = 1;
         (ntokens = fscanf(scrfile, "%lld %zd\n", &offset, &n)) > 0;
         ln++) {
      if (ntokens > 2) {
        fprintf(stderr, "%s: %s %d: too many tokens\n", argv0, scrname, ln);
        exit(-1);
      }
      if (ntokens == 1) {
        n = blocksz;
      }

      if (vflag) {
        fprintf(stdout, "%s: offset %lld n %zd\n",
                argv0, offset, n);
      }

      if ((offset != last_offs) && lseek(fd, offset, SEEK_SET) < 0) {
        fprintf(stderr, "%s: %s: failed seek to offset %lld: %s\n",
                argv0, fname, offset, strerror(errno));
        exit(-1);
      }

      nread = read(fd, buf, n);
      if (nread != n) {
        fprintf(stderr, "%s: read %s %zd bytes: %s\n",
                argv0, fname, nread, strerror(errno));
        if (nread < 0) {
          exit(-1);
        } else {
          goto done;
        }
      }
      bytes += nread;
      last_offs = offset + nread;
    }

    if (ntokens != EOF) {
      fprintf(stderr, "%s: %s: %s\n", argv0, scrname, strerror(errno));
      exit(-1);
    }

    goto done;
  }

 again:
  if (begin) {
    if (lseek(fd, begin, SEEK_SET) != begin) {
      fprintf(stderr, "%s: initial seek %s to offset %lld: %s\n", argv0, fname, begin, strerror(errno));
      exit(-1);
    }
  }
  last_offs = begin;
  for (offset = begin; !length || offset < length; offset += n, bytes += n) {
    if (targetbw > 0 && bytes) {
      struct timeval now, dt, iv;
      struct timespec interval;
      long long curbw;

      gettimeofday(&now, NULL);
      dt = now;
      TIMEVAL_SUB(&dt, &time0);
      curbw = (1000000LL * bytes) / (1000000LL * dt.tv_sec + dt.tv_usec);
      if (curbw > targetbw) {
        /* bytes / (now + interval - time0) = targetbw
         * now + interval - time0 = bytes / targetbw
         * interval = bytes / targetbw - (now - time0)
         */
        iv.tv_sec = bytes / targetbw;
        iv.tv_usec = ((1000000 * bytes) / targetbw) % 1000000;
        TIMEVAL_SUB(&iv, &dt);
        if (iv.tv_sec > 0 || iv.tv_usec > 0) {
          interval.tv_sec = iv.tv_sec;
          interval.tv_nsec = iv.tv_usec * 1000;
          nanosleep(&interval, NULL);
        }
      }
    }
    if (vflag && (offset - last_offs >= (100<<20))) {
      report(bytes, &time0);
      last_offs = offset;
    }
    n = blocksz;
    if (offset + n > length) {
      n = length - offset;
    }
    n = read(fd, buf, n);
    if (n < 0) {
      fprintf(stderr, "%s: read %s %zd bytes: %s\n", argv0, fname, n, strerror(errno));
      exit(-1);
    }
    if (n == 0)
      break;
  }

 done:
  report(bytes, &time0);

  if (lflag) {
    if (lseek(fd, 0, SEEK_SET)) {
      fprintf(stderr, "%s: %s: failed seek: %s\n", argv0, fname, strerror(errno));
      exit(-1);
    }
    goto again;
  }

  return 0;
}
