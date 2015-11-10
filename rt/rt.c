#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static char *argv0;
static char *fname = "<stdout>";
static int fflag;
static int rflag;
static int vflag;
static int zflag;

typedef struct {
	int ch;
	int flag;
	char *desc;
} open_flag_t;

open_flag_t open_flags[] = {
	{ 'a', O_APPEND, "O_APPEND" },
	{ 't', O_TRUNC, "O_TRUNC" },
	{ 'x', O_EXCL, "O_EXCL" },
#ifdef O_DIRECT
	{ 'd', O_DIRECT, "O_DIRECT" },
#endif
#ifdef O_LARGEFILE
	{ 'l', O_LARGEFILE, "O_LARGEFILE" },
#endif
#ifdef O_SYNC
	{ 's', O_SYNC, "O_SYNC" },
#endif
	{ 0, 0, NULL },
};

void
usage(void)
{
	static char buf[512];
	char *s = buf;
	open_flag_t *p;

	s += sprintf(s, "Usage: %s [-fvrz] [-o [", argv0);
	for (p = open_flags; p->ch; p++) {
		*s++ = p->ch;
	}
	s += sprintf(s, "]] [-b blocksz] [-a align] [-s seed] [-B beginoffset] "
			"[-S stridesz] -l length [file]\n");
	s += sprintf(s, "Options:\n");
	s += sprintf(s, " -f: fsync before closing file\n");
	s += sprintf(s, " -v: be verbose\n");
	s += sprintf(s, " -r: write random data\n");
	s += sprintf(s, " -z: write zeroes\n");
	s += sprintf(s, "Open flags:\n");
	for (p = open_flags; p->ch; p++) {
		s += sprintf(s, " %c: %s\n", p->ch, p->desc);
	}
	fprintf(stderr, "%s", buf);
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
	fprintf(stderr, "wrote %s %12lld bytes %4ld.%03ld sec %5.1f MB/s\n",
		fname, bytes,
		dt.tv_sec, dt.tv_usec / 1000,
		(double)bytes / ((1<<20) * (dt.tv_sec + (double)dt.tv_usec/1000000)));
	fflush(stderr);
}

int
parse_oflags(char *s)
{
	int c;
	int flags = 0;
	open_flag_t *p;

	while ((c = *s++) != 0) {
		for (p = open_flags; p->ch && p->ch != c; p++)
			;
		if (!p->ch) {
			usage();
		}
		flags |= p->flag;
	}

	return flags;
}

int
main(int argc, char** argv)
{
	int ch;
	ssize_t n, bytes;
	long long offset, begin = 0;
	int fd = 1;
	int blocksz = 4<<10;
	int stridesz = 0;
	int align = 0;
	long long length = 0;
	int oflags = O_CREAT | O_RDWR;
	void *buf = NULL;
	long seed = 0;
	struct timeval time0;
	long long last_offs;

	argv0 = argv[0];

	while ((ch = getopt(argc, argv, ":fvrza:o:b:l:s:B:S:")) != -1)
		switch (ch) {
		case 'f':
			fflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'z':
			zflag++;
			break;
		case 's':
			seed = atol(optarg);
			break;
		case 'a':
			align = (int)kmg(optarg);
			break;
		case 'b':
			blocksz = (int)kmg(optarg);
			if (blocksz % sizeof(long long)) {
				fprintf(stderr, "%s: blocksz must be in multiples of %Zd\n",
					argv0, sizeof(long long));
				usage();
			}
			break;
		case 'l':
			length = kmg(optarg);
			break;
		case 'o':
			oflags |= parse_oflags(optarg);
			break;
		case 'B':
			begin = kmg(optarg);
			break;
		case 'S':
			stridesz = (int)kmg(optarg);
			break;
		case '?':
		default:
			usage();
		}

	if (!length) {
				fprintf(stderr, "%s: length must be specified\n", argv0);
				usage();
	}

	argc -= optind;
	argv += optind;

	if (*argv) {
		fname = *argv;
		if ((fd = open(fname, oflags, 0644)) < 0) {
			fprintf(stderr, "%s: creat %s: %s\n", argv0, fname, strerror(errno));
			exit(-1);
		}
	}

	if (!align) {
		align = blocksz;
	}

	if ((buf = malloc(blocksz + align - 1)) == NULL) {
		fprintf(stderr, "%s: malloc %d: %s\n", argv0, blocksz, strerror(errno));
		exit(-1);
	}

	buf = (void*)((char *)buf + align - 1);
	buf = (void*)((char *)buf - (unsigned long)buf % align);

	gettimeofday(&time0, NULL);
	if (rflag) {
		if (!seed) {
			seed = (time0.tv_sec ^ time0.tv_usec);
		}
		srandom(seed);
		fprintf(stderr, "%s: seed %ld\n", argv0, seed);
		prep(buf, 0, blocksz);
	} else if (zflag) {
		memset(buf, 0, blocksz);
	}

	if (begin) {
		if (lseek(fd, begin, SEEK_SET) != begin) {
			fprintf(stderr, "%s: initial seek %s to offset %lld: %s\n", argv0, fname, begin, strerror(errno));
			exit(-1);
		}
	}
	last_offs = begin;
	for (offset = begin; offset < length; ) {
		if (vflag && (offset - last_offs >= (100<<20))) {
			report(offset - begin, &time0);
			last_offs = offset;
		}
		if (!rflag && !zflag) {
			prep(buf, offset, blocksz);
		}
		n = blocksz;
		if (offset + n > length)
			n = length - offset;
		bytes = write(fd, buf, n);
		if (n != bytes) {
			fprintf(stderr, "%s: write %s %Zd bytes: returned %Zd: %s\n",
				argv0, fname, n, bytes, strerror(errno));
			exit(-1);
		}
		if (stridesz) {
			if (lseek(fd, offset + stridesz, SEEK_SET) != offset + stridesz) {
				fprintf(stderr, "%s: seek %s to offset %lld: %s\n",
					argv0, fname, offset + stridesz, strerror(errno));
				exit(-1);
			}
			offset = offset + stridesz;
		} else {
			offset += n;
		}
	}

	if (fflag) {
		if (fsync(fd) < 0) {
			fprintf(stderr, "%s: fsync %s: %s\n", argv0, fname, strerror(errno));
			exit(-1);
		}
	}

	if (close(fd) < 0) {
		fprintf(stderr, "%s: close %s: %s\n", argv0, fname, strerror(errno));
		exit(-1);
	}

	report(offset - begin, &time0);

	return 0;
}
