#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

typedef long long dex_off_t;

typedef ssize_t io_fun(int, dex_off_t, size_t);

static io_fun rio, wio, wtio, randio;

char *argv0;
long proc_seed;
int align = 32;
int blocksz = 4096;
dex_off_t def_end_offset;
dex_off_t start_offset, end_offset;
int timeout;
struct timeval start_time;
int sync_pipe_fd[2];
long seed;
char *rbuf, *wbuf;
char **fname_p;
dex_off_t *length_p;
int opcount;
int64_t bytes;
io_fun *io = rio;
int trunc_pct;
int fsync_pct;
int verbose;
int Rflag;
int verify;

void
errexit(char *fmt, ...)
{
  va_list ap;
  char buf[256];

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  fprintf(stderr, "%s: %s: %s\n", argv0, buf, strerror(errno));
  exit(-1);
}

void
sig_handler(int sig)
{
  struct timeval end_time;
  double t;

  gettimeofday(&end_time, NULL);
  t = (end_time.tv_sec - start_time.tv_sec) +
    (double)(end_time.tv_usec - start_time.tv_usec) / 1000000;
  printf("%s %s ops %d bytes %lld time %.3f iops %.0f throughput %.0f start %lld end %lld blocksz %d seed %ld\n",
	 *fname_p,
         io == rio ? "read" :
	 io == wio ? "write" :
	 io == wtio ? "write/trunc" :
	   "randio",
	 opcount,
	 (long long)bytes,
	 t,
	 t > 0. ? opcount / t : 0.,
	 t > 0. ? bytes / t : 0.,
	 start_offset,
	 end_offset,
	 blocksz,
	 proc_seed);
  exit(0);
}

long
nrand(long n)
{
  u_int64_t r = (u_int64_t)rand();

  return (n * r) / ((u_int64_t)RAND_MAX + 1);
}

static ssize_t
rio(int fd, dex_off_t offs, size_t count)
{
  long n;

  if (verbose) {
    printf(" read: fd %d offset %lld count %zd\n", fd, offs, count);
  }
  n = read(fd, rbuf, count);
  if (verbose > 1) {
    printf(" read: fd %d offset %lld count %zd ret %ld\n", fd, offs, count, n);
  }
  if (n > 0) {
    bytes += n;
  }
  if (verify) {
    long expected;

    if (offs + count <= *length_p) {
      expected = count;
    } else if (offs <= *length_p) {
      expected = *length_p - offs;
    } else {
      expected = 0;
    }

    if (n != expected) {
      errexit(" read: %s: fd %d offset %lld count %d expected_length %lld ret %ld expected %ld",
              *fname_p, fd, offs, count, *length_p, n, expected);
    }
  }
  return n;
}

static ssize_t
wio(int fd, dex_off_t offs, size_t count)
{
  long n;

  if (verbose) {
    printf("write: fd %d offset %lld count %zd\n", fd, offs, count);
  }
  if (Rflag) {
    memset(wbuf, nrand(256), count);
  }
  n = write(fd, wbuf, count);
  if (verbose > 1) {
    printf("write: fd %d offset %lld count %zd ret %ld\n", fd, offs, count, n);
  }
  if (n >= 0) {
    bytes += n;
    if (verify && offs + count > *length_p) {
      *length_p = offs + count;
    }
  }
  return n;
}

static ssize_t
wtio(int fd, dex_off_t offs, size_t count)
{
  int p;
  long n;

  if (!trunc_pct && !fsync_pct) {
    return wio(fd, offs, count);
  }

  p = nrand(100);
  if (trunc_pct && p < trunc_pct) {
    if (verbose) {
      printf("trunc: fd %d offset %lld\n", fd, (long long)(offs + count));
    }
    n = ftruncate(fd, offs + count);

    if (verify && n >= 0) {
      *length_p = offs + count;
    }

    return n;
  }

  if (fsync_pct && p < trunc_pct + fsync_pct) {
    if (verbose) {
      printf("fsync: fd %d\n", fd);
    }
    return fsync(fd);
  }

  return wio(fd, offs, count);
}

static ssize_t
randio(int fd, dex_off_t offs, size_t count)
{
  int p;
  long n;

  if (!trunc_pct && !fsync_pct) {
      return nrand(2) ? wio(fd, offs, count) : rio(fd, offs, count);
  }

  p = nrand(100);
  if (trunc_pct && p < trunc_pct) {
    if (verbose) {
      printf("trunc: fd %d offset %lld\n", fd, (long long)(offs + count));
    }
    n = ftruncate(fd, offs + count);

    if (verify && n >= 0) {
      *length_p = offs + count;
    }

    return n;
  }

  if (fsync_pct && p < trunc_pct + fsync_pct) {
    if (verbose) {
      printf("fsync: fd %d\n", fd);
    }
    return fsync(fd);
  }

  return nrand(2) ? wio(fd, offs, count) : rio(fd, offs, count);
}

void
dex(int fd, int count)
{
  int nblocks = (end_offset - start_offset) / blocksz;
  dex_off_t offset;

  while (count <= 0 || opcount < count) {
    offset = start_offset + nrand(nblocks) * (dex_off_t)blocksz;
    if (lseek(fd, (off_t)offset, SEEK_SET) < 0)
      errexit("lseek %s %lld", *fname_p, offset);
    if (io(fd, offset, blocksz) < 0)
      errexit("%s", *fname_p);
    opcount++;
  }
}

int
kmg(char *s)
{
  int n = strtol(s, &s, 0);

  switch (*s) {
  case 'g':
  case 'G':
    n <<= 30;
    break;
  case 'm':
  case 'M':
    n <<= 20;
    break;
  case 'k':
  case 'K':
    n <<= 10;
    break;
  }

  return n;
}

int64_t
kmgb(char *s)
{
  int64_t n = strtoll(s, &s, 0);

  switch (*s) {
  case 'g':
  case 'G':
    n <<= 30;
    break;
  case 'm':
  case 'M':
    n <<= 20;
    break;
  case 'k':
  case 'K':
    n <<= 10;
    break;
  case 'b':
  case 'B':
    n *= blocksz;
    break;
  }

  return n;
}

void
usage(void)
{
  fprintf(stderr, "Usage: %s [-rwvVR] [-T trunc_pct] [-F fsync_pct] [-a align] [-b blocksz] [-s start_offset] [-e end_offset] [-t timeout || -n count] [-S seed] file ...\n", argv0);
  fprintf(stderr, "Options:\n"
          "  -r  rand io (reads, writes, [truncates, fsyncs])\n"
          "  -w  write io (writes, [truncates, fsyncs])\n"
          "  -T  truncates percent (applicable to -r and to -w)\n"
          "  -F  fsync percent (applicable to -r and to -w)\n"
          "  -v  be verbose (repeated, be more verbose)\n"
          "  -V  verify read count against expected length\n"
          "  -a  buffer alignment in bytes (default=%d)\n"
          "  -b  blocksz size in bytes (default=%d)\n"
          "  -s  start offset in bytes\n"
          "  -e  end offset in bytes\n"
          "  -t  timeout in seconds\n"
          "  -n  number of operations\n"
          "  -S  seed for pseudo random number generator\n"
          "  -R  randomize written data\n"
          "\n"
          "* If neither -r nor -w are specified dex will do random reads only\n"
          "* Byte sizes and offsets can be followed by [kmg] to denote kilo-, mega-, or giga- bytes\n"
          "* Dex forks for each file specified and performs I/O in parallel\n",
          align, blocksz);
  exit(-1);
}

int
main(int argc, char **argv)
{
  char *cp;
  int ch, i, nproc, fd;
  int status;
  int count = 0;
  char *private_fname = NULL;
  dex_off_t private_length = 0;

  argv0 = argv[0];
  while ((cp = strchr(argv0, '/')) != NULL)
    argv0 = cp + 1;
  while ((ch = getopt(argc, argv, ":?hvVRrwF:T:a:b:s:e:t:n:S:")) != -1) {
    switch (ch) {
    case 'a':
      align = kmg(optarg);
      break;
    case 'b':
      blocksz = kmg(optarg);
      break;
    case 's':
      start_offset = kmgb(optarg);
      break;
    case 'e':
      def_end_offset = kmgb(optarg);
      break;
    case 't':
      timeout = atoi(optarg);
      break;
    case 'n':
      count = atoi(optarg);
      break;
    case 'S':
      seed = atoi(optarg);
      break;
    case 'T':
      trunc_pct =  atoi(optarg);
      if (io != (io_fun *)randio) {
	io = (io_fun *)wtio;
      }
      break;
    case 'F':
      fsync_pct =  atoi(optarg);
      if (io != (io_fun *)randio) {
	io = (io_fun *)wtio;
      }
      break;
    case 'R':
      Rflag++;
      break;
    case 'w':
      io = (io_fun *)wio;
      break;
    case 'r':
      io = (io_fun *)randio;
      break;
    case 'v':
      verbose++;
      break;
    case 'V':
      verify++;
      break;
    case '?':
    case 'h':
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;
  nproc = argc;

  if (nproc <= 0 || (!count && !timeout)) {
    usage();
  }

  rbuf = malloc(blocksz + align - 1);
  rbuf = (char *)(((u_long)rbuf + align - 1) & ~(align - 1));
  wbuf = malloc(blocksz + align - 1);
  wbuf = (char *)(((u_long)wbuf + align - 1) & ~(align - 1));
  memset(wbuf, 0xff, blocksz);

  if (!seed)
    seed = time(0) + getpid();

  if (pipe(sync_pipe_fd) < 0)
    errexit("pipe");

  fname_p = &private_fname;
  length_p = &private_length;

  for (i = 0; i < nproc; i++) {
    private_fname = argv[i];

    fd = open(private_fname, io == (io_fun *)read ? O_RDONLY : (O_CREAT | O_RDWR), 0644);
    if (fd < 0)
      errexit("open %s", private_fname);
    private_length = lseek(fd, 0, SEEK_END);
    if (fd < 0)
      errexit("lseek %s", private_fname);
    end_offset = def_end_offset;
    if (end_offset <= 0)
      end_offset = private_length;
    if (end_offset <= 0)
      usage();

    switch (fork()) {
    case 0:
      proc_seed = seed + i;
      srand(proc_seed);
      signal(SIGALRM, sig_handler);
      read(sync_pipe_fd[1], &ch, 1);
      gettimeofday(&start_time, NULL);
      if (timeout) {
	alarm(timeout);
      }
      dex(fd, count);
      sig_handler(-1);
      break;
    case -1:
      errexit("fork");
      break;
    default:
      break;
    }
  }

  for (i = 0; i < nproc; i++)
    write(sync_pipe_fd[0], &ch, 1);

  status = 0;
  for (i = 0; i < nproc; i++) {
    int s;

    wait(&s);
    if (s)
      status = s;
  }

  return status;
}
