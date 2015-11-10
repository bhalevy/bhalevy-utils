/*
 * fops - an interactive file operations interpreter
 *
 * @author bhalevy
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef unsigned char pan_uint8_t;
typedef          char pan_int8_t;
typedef unsigned short pan_uint16_t;
typedef          short pan_int16_t;
typedef unsigned int pan_uint32_t;
typedef          int pan_int32_t;
typedef unsigned long long pan_uint64_t;
typedef          long long pan_int64_t;

#define PAN_ASSERT(x) \
  if (!(x)) { \
    fprintf(stderr, "assert(%s) failed\n", "## x ##"); \
    exit(EXIT_FAILURE); \
  }

static char *pan_progname;
static char *cmdline;

static off_t
kmg(char *s)
{
  char              *s_end;
  pan_uint64_t       n;
  off_t              n_off;

  n = strtoull(s, &s_end, 0);

  switch (*s_end) {
  case 't': case 'T': n <<= 40; break;
  case 'g': case 'G': n <<= 30; break;
  case 'm': case 'M': n <<= 20; break;
  case 'k': case 'K': n <<= 10; break;
  }

  n_off = (off_t)n;

  PAN_ASSERT(((pan_uint64_t)n_off) == n);

  return n_off;
}

static int
mygetc(char *ch)
{
  static char *cmdline_p;

  if (cmdline != NULL) {
    if (cmdline_p == NULL) {
      cmdline_p = cmdline;
    }
    *ch = *cmdline_p;
    if (*ch == '\0') {
      return 0;
    }
    cmdline_p++;
    return 1;
  }

  return read(0, ch, 1);
}

static int
mygetln(char *buf, int len)
{
  int n;
  char *p;
  char ch;

  for (p = buf; /*@-strictops@*/ p - buf /*@=strictops@*/ < len - 1; ) {
    n = mygetc(&ch);
    if (n < 0) {
      return n;
    }
    if (n == 0) {
      break;
    }
    if (ch == '\r' || ch == '\n' || ch == ';' || ch == 0) {
      if (p == buf) {
        continue;
      }
      break;
    }
    if (p == buf && (ch == ' ' || ch == '\t')) {
      continue;
    }
    *p++ = ch;
  }

  *p = '\0';

  return /*@-strictops@*/ p - buf /*@=strictops@*/;
}

static char *
nexttok(char *s, char *delim)
{
  while (*s != '\0' && !strchr(delim, *s)) {
    s++;
  }

  while (*s != '\0' && strchr(delim, *s)) {
    s++;
  }

  if (*s == '\0') {
    return NULL;
  }

  return s;
}

void
usage(void)
{
  fprintf(stderr, "Usage: %s [-c cmd] file\n", pan_progname);
  exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
  int len, bytes;
  int fd = 1;
  off_t offs;
  char *fname = NULL;
  char cmd[256], *tok, *buf;
  static char delim[] = " \t";

  pan_progname = *argv++;
  if ((tok = strrchr(pan_progname, '/')) != NULL) {
    pan_progname = tok + 1;
  }
  if (!argv[0]) {
    usage();
  }
  while (argv[0][0] == '-') {
    switch (argv[0][1]) {
    case 'c':
      cmdline = *++argv;
      break;

    default:
      usage();
    }
    argv++;
  }

  fname = argv[0];
  if (!fname) {
    usage();
  }

  fd = open(fname, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    fprintf(stderr, "%s: %s: open failed: %s\n",
            pan_progname, fname, strerror(errno));
    exit(EXIT_FAILURE);
  }

  while (1) {
    len = mygetln(cmd, sizeof(cmd));
    if (len < 0) {
      fprintf(stderr, "%s: <stdin>: read failed: %s\n",
              pan_progname, strerror(errno));
      exit(EXIT_FAILURE);
    }
    if (len == 0) {
      break;
    }
    switch (cmd[0]) {
    case 'h':
    case '?':
    help:
      printf("commands can be separated by new lines or ';'\n"
             "s offset   seek to offset\n"
             "w string   write string at current offset\n"
             "r count    read count bytes at current offset\n"
             "t length   truncate to length\n"
             "f          fsync\n"
             "c          close (and reopen) file\n"
             "h          print this help message\n"
             "q          quit\n\n");
      break;

    case 'q':
      goto out;

    case 'c':
      if (close(fd) < 0) {
        fprintf(stderr, "%s: %s: close failed: %s\n",
                pan_progname, fname, strerror(errno));
        exit(EXIT_FAILURE);
      }
      fd = open(fname, O_RDWR, 0644);
      if (fd < 0) {
        fprintf(stderr, "%s: %s: open failed: %s\n",
                pan_progname, fname, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;

    case 's':
      tok = nexttok(cmd, delim);
      if (tok == NULL) {
        goto help;
      }
      offs = kmg(tok);
      if (lseek(fd, offs, SEEK_SET) != offs) {
        fprintf(stderr, "%s: %s: seek to offset %lld failed: %s\n",
                pan_progname, fname, (long long)offs, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;

    case 't':
      tok = nexttok(cmd, delim);
      if (tok == NULL) {
        goto help;
      }
      offs = kmg(tok);
      if (ftruncate(fd, offs) < 0) {
        fprintf(stderr, "%s: %s: truncate to length %lld failed: %s\n",
                pan_progname, fname, (long long)offs, strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;

    case 'w':
      tok = nexttok(cmd, delim);
      if (tok == NULL) {
        goto help;
      }
      len = write(fd, tok, strlen(tok));
      if (len < 0) {
        fprintf(stderr, "%s: %s: write(%s, %d) failed: %s\n",
                pan_progname, fname, tok, (int) strlen(tok), strerror(errno));
        exit(EXIT_FAILURE);
      }
      break;

    case 'r':
      tok = nexttok(cmd, delim);
      if (tok == NULL) {
        goto help;
      }
      len = kmg(tok);
      buf = malloc(len+1);
      if (!buf) {
        fprintf(stderr, "%s: allocate %d bytes failed: %s\n",
                pan_progname, len, strerror(errno));
      } else {
        bytes = read(fd, buf, len);
        if (bytes < 0) {
          fprintf(stderr, "%s: %s: read(%s, %d) error: %s\n",
                  pan_progname, fname, tok, len, strerror(errno));
          exit(EXIT_FAILURE);
        }
        buf[bytes] = 0;
        if (bytes != len) {
          fprintf(stderr, "%s: %s: read(%s, %d) returned %d\n",
                  pan_progname, fname, tok, len, bytes);
        }
        printf("'%s'\n", buf);
        free(buf);
      }
      break;

    case 'f':
      if (fsync(fd) < 0) {
        fprintf(stderr, "%s: %s: fsync failed: %s\n",
                pan_progname, fname, strerror(errno));
        exit(EXIT_FAILURE);
      }
    }
  }

 out:

  if (close(fd) < 0) {
    fprintf(stderr, "%s: %s: close failed: %s\n",
            pan_progname, fname, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return 0;
}
